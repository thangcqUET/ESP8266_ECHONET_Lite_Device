#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>
#include <DHT.h>
WiFiUDP UDPSender;
WiFiUDP UDPReceiver;

ESP8266WebServer server(80);



int Relay_Pin=D1;
int DHT11_Pin=D2;
DHT dht(DHT11_Pin,DHT11);
// ESV value
const byte SetI = 0x60; // request write value (no response required)
const byte SetC = 0x61; // request write value (response required)
const byte Get = 0x62; // request read value
const byte Set_Res = 0x71; // response write value
const byte Get_Res = 0x72; // response read value
const byte INF = 0x73;
const byte INF_REQ = 0x63;

const byte GET_echo = 0xD6;


// EPC value
//Device Object Super Class Properties
const byte OperationStatus = 0x80; //0x30=ON 0x31=OFF
const byte Location = 0x81; //location
const byte PowerLimit = 0x99; //powerlimit
//Requirement for electric energy sensor class: 0x00 0x22 0x01-0x7F
const byte Cumulative = 0xE0; //Cumulative amounts of electric energy 4B 0,001Wh
const byte Capacity_med = 0xE1; //Medium-capacity sensor instantaneous electric energy 4B W
const byte Capacity_sma = 0xE2; //Small-capacity sensor instantaneous electric energy 2B 0.1W
const byte Capacity_lar = 0xE3; //Large-capacity sensor instantaneous electric energy 2B 0.1kW
const byte Cumulative_log = 0xE4; //Cumulative amounts of electric energy measurement log 192B 0,0001kWh
const byte Effective_voltage = 0xE5; //Effective voltage value 2B V

/*
 * EEPROM
 * 0-31 : ssid 
 * 32 - 96:pass 
 * quantity EOJ: 97
 * EOJ code: X1-X2-X3: 98 -130
 * future:131- 150 
 * Status: 151 
 * Power limit: 152- 153 
 * Location: 154 
 * Port ESP8266: 
 */



const IPAddress ipMulti = {224, 0, 23, 0};
const int portMulti = 3610;

//Access point
const char* ssid = "AP_ESP8266";
const char* passphrase = "12345678";

int sizeOfResponsePacket=0;
String st;
String content;
int statusCode;

int getMaxValue();
void setStatusDevice();
void sendEchoNode(IPAddress ipMulti, int portMulti );
void sendMessage(IPAddress ip, int port, byte* message);

union {
  float floatValue;
  unsigned char bytes[4];
} thing;


class property_content{
  public:
  byte EPC=0, PDC=0;
  byte EDT[20]={0};
};
class packet{
  public:
    byte TID[2]={0}, SEOJ[3]={0}, DEOJ[3]={0}, ESV=0, OPC=0;
    property_content properties[5];
    packet(){
      
    }
//     Create response by request
    packet(packet* req){
//      Serial.println("Da chay ham khoi tao111111111111111111111111111111111111111111111111111111111111111111111");
      for(int i=0;i<2;i++){
        this->TID[i]=req->TID[i];
      }
      for(int i=0;i<3;i++){
        this->DEOJ[i]=req->SEOJ[i];
      }
    }
    // byte[] -> packet
    void readPacket(byte packet[], int n){
      byte SEOJ_copy[] = {packet[4], packet[5], packet[6]};
      for(int i=0;i<3;i++){
        SEOJ[i]=SEOJ_copy[i];
      }
      byte DEOJ_copy[] = {packet[7], packet[8], packet[9]};
      for(int i=0;i<3;i++){
        DEOJ[i]=DEOJ_copy[i];
      }
      byte TID_copy[] = {packet[2], packet[3]};
      for(int i=0;i<3;i++){
        TID[i]=TID_copy[i];
      }
      ESV = packet[10];
      OPC = packet[11];
      int p=12;
      for(int i=0;i<OPC;i++){
        properties[i].EPC=packet[p++];
        properties[i].PDC=packet[p++];
        for(int j=0;j<properties[i].PDC;j++){
          properties[i].EDT[j]=packet[p++];
        }
      }
    }
    // packet -> byte[]
    byte* writePacket(){
      byte head_packet[12]={0x10, 0x81, TID[0], TID[1], SEOJ[0], SEOJ[1], SEOJ[2], DEOJ[0], DEOJ[1], DEOJ[2], ESV, OPC};
      int n=0;
      for(int i=0;i<OPC;i++){
        n+=(properties[i].PDC+2);
      }
      n+=12;
      byte *packet=(byte*)malloc(n*sizeof(byte));
      for(int i=0;i<12;i++){
        packet[i]=head_packet[i];
      }
      int i=12;
      int j=0;
      while(1){
        if(j>=OPC) break;
        packet[i++]=properties[j].EPC;
        packet[i++]=properties[j].PDC;
        for(int t=0;t<properties[j].PDC;t++){
          packet[i++]=properties[j].EDT[t];
        }
        j++;
      }
////      //fixbug
//      Serial.println("Write_Packet:");
//      for(int i=0;i<n;i++){
//        Serial.print(packet[i]);
//        Serial.print(" ");
//      }
//      Serial.println();
////      //
      sizeOfResponsePacket=n;
      return packet;
    }
    void printPacket(){
      Serial.print("TID: ");
      Serial.print(TID[0]);
      Serial.print(" ");
      Serial.print(TID[1]);
      Serial.print(" ");
      Serial.print("SEOJ: ");
      Serial.print(SEOJ[0]);
      Serial.print(" ");
      Serial.print(SEOJ[1]);
      Serial.print(" ");
      Serial.print(SEOJ[2]);
      Serial.print(" ");
      Serial.print("DEOJ: ");
      Serial.print(DEOJ[0]);
      Serial.print(" ");
      Serial.print(DEOJ[1]);
      Serial.print(" ");
      Serial.print(DEOJ[2]);
      Serial.print(" ");
      Serial.print("ESV: ");
      Serial.print(ESV);
      Serial.print(" ");
      Serial.print("OPC: ");
      Serial.print(OPC);
      Serial.print(" ");
      int j=0;
      while(1){
        if(j>=OPC) break;
        Serial.print("EPC: ");
        Serial.print(properties[j].EPC);
        Serial.print(" ");
        Serial.print("PDC: ");
        Serial.print(properties[j].PDC);
        Serial.print(" ");
        for(int t=0;t<properties[j].PDC;t++){
          Serial.print("EDT: ");
          Serial.print(properties[j].EDT[t]);
          Serial.print(" ");
        }
        j++;
      }
      Serial.println();
    }
};



void setup() {
  Serial.begin(115200);
  EEPROM.begin(512);
  pinMode(LED_BUILTIN, OUTPUT);
  //relay
  pinMode(Relay_Pin, OUTPUT);
  pinMode(A0, INPUT);
  //dht11 temperature humidity
  dht.begin();
  delay(10);
  Serial.println("**************************************");
  Serial.println("Startup");
  
  // read eeprom for ssid
  Serial.println("Reading EEPROM ssid");
  String esid="";
  for (int i = 0; i < 32; ++i)
  {
    esid += char(EEPROM.read(i));//
  }
  Serial.print("SSID: ");
  Serial.println(esid);

  // read eeprom for pass
  Serial.println("Reading EEPROM pass");
  String epass = "";
  for (int i = 32; i < 96; ++i)
  {
    epass += char(EEPROM.read(i));
  }
  Serial.print("PASS: ");
  Serial.println(epass);

  // Turn off auto connect with wifi before
  WiFi.setAutoConnect(false);

  // Connect wifi
  if ( esid.length() > 1 ) {
    WiFi.begin(esid.c_str(), epass.c_str());
    if (testWifi()) {
      launchWeb(0);

      UDPReceiver.beginMulticast(WiFi.localIP(),  ipMulti, portMulti);
      Serial.println("ECHO NODE");
      //Node Profile Class: 0xD5 Instance list notification
      sendEchoNode(ipMulti, portMulti);
      

      WiFi.softAPdisconnect(true);
      // check Status
      setStatusDevice();
      return;
    }
  } 
  
    setupAP();
  
}

bool testWifi(void) {
  int c = 0;
  Serial.println("Waiting for Wifi to connect");
  while ( c < 20 ) {
    if (WiFi.status() == WL_CONNECTED) {
      return true;
    }
    delay(500);
    Serial.print('*');
    c++;
  }
  Serial.println();
  Serial.println("Connect timed out, opening AP");
  return false;
}

void launchWeb(int webtype) {
  Serial.println();
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("SoftAP IP: ");
  Serial.println(WiFi.softAPIP());
  createWebServer(webtype);

  // Start the server
  server.begin();
  Serial.println("Server started");
}

void setupAP(void) {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  int n = WiFi.scanNetworks();
  Serial.println("Scan done");
  if (n == 0)
    Serial.println("No networks found");
  else
  {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");
      delay(10);
    }
  }
  Serial.println("");
  st = "<ol>";
  for (int i = 0; i < n; ++i)
  {
    // Print SSID and RSSI for each network found
    st += "<li>";
    st += WiFi.SSID(i);
    st += " (";
    st += WiFi.RSSI(i);
    st += ")";
    st += (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*";
    st += "</li>";
  }
  st += "</ol>";
  delay(100);

  WiFi.softAP(ssid, passphrase, 6);
  Serial.println("Soft AP");
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("PASS: ");
  Serial.println(passphrase);
  launchWeb(1);
  Serial.println("over");
}

void createWebServer(int webtype)
{
  if ( webtype == 1 ) {
    server.on("/", []() {
      IPAddress ip = WiFi.softAPIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      content = "<!DOCTYPE HTML>\r\n<html><title></title><body>Hello from ESP8266 at ";
      content += ipStr;
      content += "<p>";
      content += scanWifi();
      content += "</p><form method='get' action='setting'><table><tr><td>SSID: </td><td><input name='ssid' length=32></td></tr><tr><td>PASS: </td><td><input name='pass' length=64></td></tr><tr><td></td><td><input type='submit'></td></tr></table></form></body>";
      content += "</html>";
      server.send(200, "text/html", content);
    });

    server.on("/setting", []() {
      String qsid = server.arg("ssid");
      String qpass = server.arg("pass");
      if (qsid.length() > 0 && qpass.length() > 0) {
        Serial.println("clearing eeprom");
        for (int i = 0; i < 96; ++i) {
          EEPROM.write(i, 0);
        }
        Serial.println(qsid);
        Serial.println("");
        Serial.println(qpass);
        Serial.println("");

        Serial.println("writing eeprom ssid:");
        for (int i = 0; i < qsid.length(); ++i)
        {
          EEPROM.write(i, qsid[i]);
          Serial.print("Wrote: ");
          Serial.println(qsid[i]);
        }
        Serial.println("writing eeprom pass:");
        for (int i = 0; i < qpass.length(); ++i)
        {
          EEPROM.write(32 + i, qpass[i]);
          Serial.print("Wrote: ");
          Serial.println(qpass[i]);
        }
        EEPROM.commit();
        content = "{\"Success\":\"saved to eeprom... reset to boot into new wifi\"}";
        statusCode = 200;
      } else {
        content = "{\"Error\":\"404 not found\"}";
        statusCode = 404;
        Serial.println("Sending 404");
      }
      server.send(statusCode, "application/json", content);
    });
  } else if (webtype == 0) {
    server.on("/", []() {
      IPAddress ip = WiFi.localIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      //      server.send(200, "application/json", "{\"IP\":\"" + ipStr + "\"}");
      content = "<html><head><title>Update My Electric Energy Sensor Infomation</title></head><body style='font-family:Verdana'><form action='update' method='get'>";
      content += "<center><h3>My Electric Energy Sensor Infomation</h3><h4>IP Address : " + ipStr + "</h4><hr/><table><tr>";
      //
      content +="<tr>";
      content +="<td>Number of EOJs: </td>";
      content +="<td><input type='text' name='NumOfEOJs' value='" + (String)EEPROM.read(97) + "'size='3'></td>";
      content +="</tr>";
      
      for(int i=0;i<11;i++){
        content +="<tr>";
        content +="<td>EOJ Code : </td>";
        content +="<td><input type='text' name='X"+(String)(3*i+0)+"' value='" + (String)EEPROM.read(98+3*i+0) + "' size='3'></td>";
        content +="<td><input type='text' name='X"+(String)(3*i+1)+"' value='" + (String)EEPROM.read(98+3*i+1) + "' size='3'></td>";
        content +="<td><input type='text' name='X"+(String)(3*i+2)+"' value='" + (String)EEPROM.read(98+3*i+2) + "' size='3'></td>";
        content +="</tr>";
      }
      content +="<tr>"; 
      content +="<td><p style='font-size:11'>Example: 0x00 = 0; 0x22 = 34; 0x01 = 1</p></td>";
      content +="</tr>"; 
      //
      content += "<td>Operation Status : </td><td><input type='radio' value='ON' name='OperationStatus'";
      EEPROM.read(161) == 0x30 ? content += "checked" : "";
      content += ">ON<input type='radio' value='OFF' name='OperationStatus'";
      EEPROM.read(161) == 0x31 ? content += "checked" : "";
      content += ">OFF</td></tr><tr><td>Power Limited   : </td><td><input type='number' name='PowerLimit' value='";
      content += int((EEPROM.read(163) & 0xff) << 8) | EEPROM.read(162);
      content += "'/>&nbsp;W</td></tr><tr><td>Location : </td><td><input type='text' name='Location' value='"+(String)EEPROM.read(164)+"'/></td></tr><tr><td></td><td><input type='submit' name='Submit'/></td></tr></table></center></form>";
      content += "<form method='get' action='changewifi'><center><p><p>WiFi: <input type='submit' value='Choose'></center></form></body></html>";
      server.send(200, "text/html", content);
    });

    server.on("/changewifi", []() {
      IPAddress ip = WiFi.localIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      content = "<!DOCTYPE HTML>\r\n<html><title></title><body>Hello from ESP8266 at ";
      content += ipStr;
      content += "<p>";
      content += scanWifi();
      content += "</p><form method='get' action='setting'><table><tr><td>SSID: </td><td><input name='ssid' length=32></td></tr><tr><td>PASS: </td><td><input name='pass' length=64></td></tr><tr><td></td><td><input type='submit'></td></tr></table></form></body>";
      content += "</html>";
      server.send(200, "text/html", content);
    });

    server.on("/setting", []() {
      String qsid = server.arg("ssid");
      String qpass = server.arg("pass");
      if (qsid.length() > 0 && qpass.length() > 0) {
        Serial.println("clearing eeprom");
        for (int i = 0; i < 96; ++i) {
          EEPROM.write(i, 0);
        }
        Serial.println(qsid);
        Serial.println("");
        Serial.println(qpass);
        Serial.println("");

        Serial.println("writing eeprom ssid:");
        for (int i = 0; i < qsid.length(); ++i)
        {
          EEPROM.write(i, qsid[i]);
          Serial.print("Wrote: ");
          Serial.println(qsid[i]);
        }
        Serial.println("writing eeprom pass:");
        for (int i = 0; i < qpass.length(); ++i)
        {
          EEPROM.write(32 + i, qpass[i]);
          Serial.print("Wrote: ");
          Serial.println(qpass[i]);
        }
        EEPROM.commit();
        content = "{\"Success\":\"saved to eeprom... reset to boot into new wifi\"}";
        statusCode = 200;
      } else {
        content = "{\"Error\":\"404 not found\"}";
        statusCode = 404;
        Serial.println("Sending 404");
      }
      server.send(statusCode, "application/json", content);
    });

    server.on("/update", []() {
      bool testX=true;
      String qOstatus = server.arg("OperationStatus");
      String qPower = server.arg("PowerLimit");
      String qLocation=server.arg("Location");
      String qNumOfEOJs=server.arg("NumOfEOJs");
      if(qNumOfEOJs.length()==0) testX=false;
      String qX[qNumOfEOJs.toInt()*3];
      for(int i=0;i<qNumOfEOJs.toInt()*3;i++){
        String temp="X"+(String)(i);
        qX[i] = server.arg(temp);
        if(qX[i].length()==0) testX=false;
      }
      
      if (qOstatus.length() > 0 && qPower.length() > 0 && qLocation.length()>0 && testX) {
        Serial.println("Update status and power limit");

        if (qOstatus.equals("ON")) {
          EEPROM.write(161, 0x30);
          digitalWrite(LED_BUILTIN, HIGH);
          digitalWrite(Relay_Pin, HIGH);
          Serial.println("Device on");
        } else if (qOstatus.equals("OFF")) {
          EEPROM.write(161, 0x31);
          digitalWrite(LED_BUILTIN, LOW);
          digitalWrite(Relay_Pin, LOW);
          Serial.println("Device off");
        } else {
          Serial.println("Update status failed");
        }
        
        if(EEPROM.read(162) != (qPower.toInt() & 0xff)) {
          EEPROM.write(162, qPower.toInt() & 0xff);
        }
        if(EEPROM.read(163) != ((qPower.toInt() >> 8) & 0xff)) {
          EEPROM.write(163, (qPower.toInt() >> 8) & 0xff);
        }
        if(EEPROM.read(97) != (qNumOfEOJs.toInt())) {
          EEPROM.write(97, qNumOfEOJs.toInt());
        }
        for(int i=0;i<qNumOfEOJs.toInt()*3;i++){
          if(EEPROM.read(98+i) != qX[i].toInt()) {
            EEPROM.write(98+i, qX[i].toInt());
          }
        }
        if(EEPROM.read(164) != qLocation.toInt()){
          EEPROM.write(164, qLocation.toInt());
        }
        EEPROM.commit();
        
      }
      server.sendContent("HTTP/1.1 301 OK\r\nLocation: /\r\nCache-Control: no-cache\r\n\r\n");
    });
  }
}

String scanWifi() {
  String s = "<ol>";
  int n = WiFi.scanNetworks();
  if (n == 0) {
    s += "No networks found";
  } else {
    for (int i = 0; i < n; ++i) {
      // Print SSID and RSSI for each network found
      s += "<li>";
      s += WiFi.SSID(i);
      s += " (";
      s += WiFi.RSSI(i);
      s += ")";
      s += (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*";
      s += "</li>";
    }
  }
  s += "</ol>";
  delay(100);
  return s;
}

int count = 0;
void loop() {
  server.handleClient();
/*h'
  byte deviceStatus = EEPROM.read(100);
  Serial.println(deviceStatus);
  if (deviceStatus == 0x30) {
    digitalWrite(LED_BUILTIN, LOW);
    digitalWrite(4, LOW);
  } else if (deviceStatus == 0x31) {
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(4, HIGH);
  } else {
    Serial.println("Can't get deviceStatus. Set it!");
  }
  */
  int noBytes = UDPReceiver.parsePacket();
  if (noBytes) {
    Serial.println("Recevied a packet........");
    byte request[noBytes];
    for (int i = 0; i < noBytes; i++) {
      request[i] = UDPReceiver.read();
    }
    // read a packet
//    byte trans_ID[] = {request[2], request[3]};
//    byte req_EOJ[] = {request[4], request[5], request[6]};
//    byte ESV = request[10];
//    byte EPC = request[12];
    packet req;
    // send a packet    packet req;
    req.readPacket(request, noBytes);
    req.printPacket();
    
    //set TID, DEOJ 
    packet res(&req);
    //set SEOJ
    for(int i=0;i<3;i++){
      res.SEOJ[i]=req.DEOJ[i];
    }
    byte ESV = req.ESV;
    Serial.print("OPC: ");
    Serial.println(req.OPC);
    IPAddress reqIP = UDPReceiver.remoteIP();
    int reqPort = UDPReceiver.remotePort();
    
    
//    req.readPacket(request, noBytes);
//    //fixbug
//    Serial.println("Truoc xu ly");
//    Serial.println("Request: ");
//    req.printPacket();
//    Serial.println("Response: ");
//    res.printPacket();
//    //
    if (ESV == Get) {
      res.ESV=0x72;//Get_Res
      for(int i=0;i<req.OPC;i++){
        byte EPC=req.properties[i].EPC;
        switch(EPC){
          case Location:{
            res.properties[res.OPC].EPC=EPC;
            res.properties[res.OPC].PDC=1;
            for(int i=0;i<res.properties[res.OPC].PDC;i++){
              res.properties[res.OPC].EDT[i]=EEPROM.read(164+i);
            }
            res.OPC++;

            Serial.println("Sended Location packet!");
            break;
          }
          case OperationStatus:{
            
            res.properties[res.OPC].EPC=EPC;
            res.properties[res.OPC].PDC=1;
            res.properties[res.OPC].EDT[0]=EEPROM.read(161);
            res.OPC++;

            Serial.println("Sended Status packet!");
            break;
          }
          case PowerLimit:{
            res.properties[res.OPC].EPC=EPC;
            res.properties[res.OPC].PDC=2;
            for(int i=0;i<res.properties[res.OPC].PDC;i++){
              res.properties[res.OPC].EDT[i]=EEPROM.read(162+i);
            }
            res.OPC++;

            Serial.println("Sended power limit");
            break;
          }
          
          case 0xE1:{
            //Medium capacity
            if(req.DEOJ[0]==0&&req.DEOJ[1]==34){
              float capacity = (float) ((getMaxValue() - 500) * (5.0 / 1024) * 1000 / 118) / sqrt(2) * 220;
              thing.floatValue = capacity;
              byte bytes[4] = {0, 0, 0, 0};
              for (int i = 0; i < 4; i++) {
                bytes[i] = thing.bytes[3 - i];
              }
              res.properties[res.OPC].EPC=EPC;
              res.properties[res.OPC].PDC=4;
              for(int i=0;i<res.properties[res.OPC].PDC;i++){
                res.properties[res.OPC].EDT[i]=bytes[i];
              }
              res.OPC++;
  
              Serial.print("Sended capacity medium ");
              Serial.println(capacity);
              break;  
            }
          }
          case GET_echo:{
            byte resMessage[] = {0x10, 0x81, req.TID[0], req.TID[1],0x0E, 0xF0, 0x01, 0x0E, 0xF0, 0x01,INF, 0x01,0xD5, 0x04, 0x01, req.DEOJ[0], req.DEOJ[1], req.DEOJ[2]}; //packet ECHONET Lite
            sendMessage(reqIP, reqPort, resMessage);
            Serial.println("Sended info packet");
            break;
          }
          
          case 0xE0:{
            //Humidity
            if(EEPROM.read(98)==0x00 &&EEPROM.read(99)==0x12){
              byte humidity=(byte)dht.readHumidity();
              res.properties[res.OPC].EPC=EPC;
              res.properties[res.OPC].PDC=1;
              for(int i=0;i<res.properties[res.OPC].PDC;i++){
                res.properties[res.OPC].EDT[i]=humidity;
              }
              res.OPC++;
  
              Serial.print("Sended Humidity: ");
              Serial.println(humidity);
              break;  
            }
            //Temperature
            else if(EEPROM.read(98)==0x00 &&EEPROM.read(99)==0x11){
              byte temperature=(byte)dht.readTemperature();
              res.properties[res.OPC].EPC=EPC;
              res.properties[res.OPC].PDC=2;
              //for(int i=0;i<res.properties[res.OPC].PDC;i++){
                res.properties[res.OPC].EDT[0]=0;
                res.properties[res.OPC].EDT[1]=temperature;
              //}
              res.OPC++;
  
              Serial.print("Sended Temperature: ");
              Serial.println(temperature);
              break;  
            }
            //
//            byte resMessage[] = {0x10, 0x81, req.TID[0], req.TID[1],EEPROM.read(98), EEPROM.read(99), EEPROM.read(100), req.SEOJ[0], req.SEOJ[1], req.SEOJ[2],Get_Res, 0x01,EPC, 0x04, 0x00, 0x00, 0x00, 0x00}; //packet ECHONET Lite
//            sendMessage(reqIP, reqPort, resMessage);
//            Serial.println("Sended integaral medium");
          }
          default:{
            Serial.print("EPC Code not found");
            Serial.println(EPC);
          }
        }
      }
//      //fixbug
//      Serial.println("Sau xu ly");
//      Serial.println("Request: ");
//      req.printPacket();
//      Serial.println("Response: ");
//      res.printPacket();
//      Serial.println("Array response: ");
//      byte *a=res.writePacket();
//      for(int i=0;i<sizeOfResponsePacket;i++){
//        Serial.print(a[i]);
//        Serial.print(" ");
//      }
//      Serial.println("-------");
//      //
      Serial.println("Response: ");
      res.printPacket();
      byte *a=res.writePacket();
      sendMessage(reqIP, reqPort, a);
      free(a);
    } else if (ESV == SetC) {
        res.ESV=Set_Res;
        for(int i=0;i<req.OPC;i++){
          byte EPC=req.properties[i].EPC;
          switch(EPC){
            case OperationStatus:{
              int EEPROM_index_begin=161;
              for(int j=0;j<req.properties[i].PDC;j++){
                if(EEPROM.read(EEPROM_index_begin+j) != req.properties[i].EDT[j]) {
                  EEPROM.write(EEPROM_index_begin+j, req.properties[i].EDT[j]);                  
                  delay(10);
                }
              }
              setStatusDevice();
              res.properties[i].EPC=EPC;
              res.properties[i].PDC=req.properties[i].PDC;
              for(int j=0;j<res.properties[i].PDC;j++){
                res.properties[i].EDT[j]=req.properties[i].EDT[j];
              }
              
              Serial.println("Sended SET status response packet! OK!");
              
              break;
            }
            case PowerLimit:{
              int EEPROM_index_begin=162;
              for(int j=0;j<req.properties[i].PDC;j++){
                if(EEPROM.read(EEPROM_index_begin+j) != req.properties[i].EDT[j]) {
                  EEPROM.write(EEPROM_index_begin+j, req.properties[i].EDT[j]);
                  delay(10);
                }
              }
              res.properties[i].EPC=EPC;
              res.properties[i].PDC=req.properties[i].PDC;
              for(int j=0;j<res.properties[i].PDC;j++){
                res.properties[i].EDT[j]=req.properties[i].EDT[j];
              }
              
              
              Serial.println("Sended SET power limit response packet! OK!");
              break;
            }
            default:{
              Serial.print("EPC code not found!  ");
              Serial.print("ESV: ");
              Serial.print(ESV);
              Serial.print(" EPC: ");
              Serial.println(EPC);
            }
          }
        }
        Serial.println("Response: ");
        res.printPacket();
        byte *a=res.writePacket();
        sendMessage(reqIP, reqPort, a);
        free(a);
        
    } else if(ESV == SetI) {
        for(int i=0;i<req.OPC;i++){
          byte EPC=req.properties[i].EPC;
          switch(EPC){
            case OperationStatus:{
              int EEPROM_index_begin=161;
              for(int j=0;j<req.properties[i].PDC;j++){
                if(EEPROM.read(EEPROM_index_begin+j) != req.properties[i].EDT[j]) {
                  EEPROM.write(EEPROM_index_begin+j, req.properties[i].EDT[j]);                  
                  delay(10);
                }
              }
              setStatusDevice();
              Serial.println("Set Status");
              
              break;
            }
            case PowerLimit:{
              int EEPROM_index_begin=162;
              for(int j=0;j<req.properties[i].PDC;j++){
                if(EEPROM.read(EEPROM_index_begin+j) != req.properties[i].EDT[j]) {
                  EEPROM.write(EEPROM_index_begin+j, req.properties[i].EDT[j]);
                  delay(10);
                }
              }
              Serial.println("Set PowerLimit");
              break;
            }
            default:{
              Serial.print("EPC code not found!  ");
              Serial.print("ESV: ");
              Serial.print(ESV);
              Serial.print(" EPC: ");
              Serial.println(EPC);
            }
          }
        }
    } else if (ESV == INF_REQ) {
      byte EPC=req.properties[0].EPC;
      if(EPC == GET_echo) {
        sendEchoNode(ipMulti,portMulti);
        Serial.println("Sended info packet");
      } else {
        Serial.print("EPC code not found!  ");
        Serial.print("ESV: ");
        Serial.print(ESV);
        Serial.print(" EPC: ");
        Serial.println(EPC);
      }
    } else {
      Serial.print("ESV Code Not Found :");
      Serial.println(ESV);
    }
  } else {
    delay(1000);
    count++;
    if (count > 20) {
      sendEchoNode(ipMulti,portMulti);
    }
      
      count = 0;
    }
}


int getMaxValue() {
  int sensorValue;    //value read from the sensor
  int sensorMax = 0;
  uint32_t start_time = millis();
  while ((millis() - start_time) < 1000) {  //sample for 1000ms
    sensorValue = analogRead(A0);
    //delay(100);
    if (sensorValue > sensorMax) {
      /*record the maximum sensor value*/

      sensorMax = sensorValue;
    }
  }
  return sensorMax - 3;
}
void sendMessage(IPAddress ip, int port, byte* message) {
//  //fixbug
//  Serial.println("Before send:");
//      for(int i=0;i<sizeOfResponsePacket;i++){
//        Serial.print(message[i]);
//        Serial.print(" ");
//      }
//  Serial.println();
//  //
  UDPSender.beginPacket(ip, port);
  for (int i = 0; i < sizeOfResponsePacket; i++) {
    UDPSender.write(message[i]);
    Serial.print(message[i]);
    Serial.print(" ");
  }
  Serial.println();
  UDPSender.endPacket();
}
void sendEchoNode(IPAddress ipMulti, int portMulti ){
  Serial.println("Send Echo Node");
  //Node Profile Class: 0xD5 Instance list notification
  int lengthPacket=15;
  int qualityEOJs=EEPROM.read(97);
  int PDC=qualityEOJs*3+1;
  lengthPacket+=(PDC-1);
  byte echoMyNode[lengthPacket];
  byte echoMyNode_head[]={0x10, 0x81, 0x00, 0x00, 0x0E, 0xF0, 0x01, 0x0E, 0xF0, 0x01, 0x73, 0x01, 0xD5, PDC, qualityEOJs};
  for(int i=0;i<sizeof(echoMyNode_head);i++){
    echoMyNode[i]=echoMyNode_head[i];
  }
  
  for(int i=0;i<lengthPacket-15;i++){
    echoMyNode[15+i]=EEPROM.read(98+i);
  }
  UDPSender.beginPacket(ipMulti, portMulti);
  for (int i = 0; i < sizeof(echoMyNode); i++) {
    UDPSender.write(echoMyNode[i]);
    Serial.print(echoMyNode[i]);
    Serial.print(" ");
  }
  UDPSender.endPacket();
}
void setStatusDevice() {
  byte deviceStatus = EEPROM.read(161);
  if (deviceStatus == 0x30) {
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(Relay_Pin, HIGH);
    Serial.println("Device on");
  } else if (deviceStatus == 0x31) {
    digitalWrite(LED_BUILTIN, LOW);
    digitalWrite(Relay_Pin, LOW);
    Serial.println("Device off");
  } else {
    Serial.println("Can't get deviceStatus. Set it!");
  }
}
