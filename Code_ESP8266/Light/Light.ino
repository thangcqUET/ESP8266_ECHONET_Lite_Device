//May be error English syntax
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>
//Lib WiFiUdp.h : create Sender Obj and Receiver Obj
WiFiUDP UDPSender;
WiFiUDP UDPReceiver;
//Lib ESP8266WebServer.h : open port 80 in ESP8266 for connection from client
ESP8266WebServer server(80);

//byte mEOJ[] = {0x00, 0x22, 0x01}; //Electic energy sensor 1
//ESV---: Service Code
const byte SetC = 0x61;
const byte Get = 0x62;
const byte Set_Res = 0x71;
const byte Get_Res = 0x72;
const byte INF = 0x73;
//~ESV---

const byte GET_echo = 0xD6;


//EPC---
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
//~EPC---
//Address Multicast for ECHONET Lite UDP: IP: 224.0.23.0, port: 3610---
const IPAddress ipMulti = {224, 0, 23, 0};
const int portMulti = 3610;
//~ ---
//Access point
const char* ssid = "AP_ESP8266";
const char* passphrase = "12345";
//~ ---
//---
String st;
String content;
int statusCode;
//~---
//---
union {
  float floatValue;
  unsigned char bytes[4];
} thing;
//~---
class propetie_content{
  public:
  byte EPC, PDC;
  byte EDT[20];
};
class packet{
  public:
    byte TID[2], SEOJ[3], DEOJ[3], ESV, OPC;
    propetie_content E_P_E[5];
    // byte[] -> packet
    void readPacket(byte packet[], int n){
      byte SEOJ_copy[] = {packet[4], packet[5], packet[6]};
      for(int i=0;i<3;i++){
        SEOJ[i]=SEOJ_copy[i];
      }
      byte TID_copy[] = {packet[2], packet[3]};
      for(int i=0;i<3;i++){
        TID[i]=TID_copy[i];
      }
      ESV = packet[10];
      OPC = packet[11];
      int p=12;
      for(int i=0;i<OPC;i++){
        E_P_E[i].EPC=packet[p++];
        E_P_E[i].PDC=packet[p++];
        for(int j=0;j<E_P_E[i].PDC;j++){
          E_P_E[i].EDT[j]=packet[p++];
        }
      }
    }
    // packet -> byte[]
    byte* writePacket(){
      byte head_packet[12]={0x10, 0x81, TID[0], TID[1], SEOJ[0], SEOJ[1], SEOJ[2], DEOJ[0], DEOJ[1], DEOJ[2], ESV, OPC};
      int n=0;
      for(int i=0;i<OPC;i++){
        n+=(E_P_E[i].PDC+2);
      }
      n+=12;
      byte packet[n];
      for(int i=0;i<12;i++){
        packet[i]=head_packet[i];
      }
      int i=12;
      int j=0;
      while(1){
        if(j>=OPC) break;
        packet[i++]=E_P_E[j].EPC;
        packet[i++]=E_P_E[j].PDC;
        for(int t=0;t<E_P_E[j].PDC;t++){
          packet[i++]=E_P_E[j].EDT[t];
        }
        j++;
      }
      return packet;
    }
    void printPacket(){
      
      Serial.print(TID[0]+" ");
      Serial.print(TID[1]+" ");
      Serial.print(SEOJ[0]+" ");
      Serial.print(SEOJ[1]+" ");
      Serial.print(SEOJ[2]+" ");
      Serial.print(DEOJ[0]+" ");
      Serial.print(DEOJ[1]+" ");
      Serial.print(DEOJ[2]+" ");
      Serial.print(ESV+" ");
      Serial.print(OPC+" ");
      int j=0;
      while(1){
        if(j>=OPC) break;
        Serial.print(E_P_E[j].EPC+" ");
        Serial.print(E_P_E[j].PDC+" ");
        for(int t=0;t<E_P_E[j].PDC;t++){
          Serial.print(E_P_E[j].EDT[t]+" ");
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
  pinMode(4, OUTPUT);
  //digitalWrite(4,HIGH);
  pinMode(A0, INPUT);
  delay(10);
  Serial.println();
  Serial.println();
  Serial.println("Startup");
  
  // read eeprom for ssid and pass
  Serial.println("Reading EEPROM ssid");
  String esid;
  for (int i = 0; i < 32; ++i)
  {
    esid += char(EEPROM.read(i));//
  }
  Serial.print("SSID: ");
  Serial.println(esid);
  Serial.println("Reading EEPROM pass");
  String epass = "";
  for (int i = 32; i < 96; ++i)
  {
    epass += char(EEPROM.read(i));
  }
  Serial.print("PASS: ");
  Serial.println(epass);
  //WiFi.setAutoConnect(false);
  if ( esid.length() > 1 ) {
    WiFi.begin(esid.c_str(), epass.c_str());
    if (testWifi()) {
      launchWeb(0);
      UDPReceiver.beginMulticast(WiFi.localIP(),  ipMulti, portMulti);
      byte echoMyNode[] = {0x10, 0x81, 0x00, 0x00, 0x0E, 0xF0, 0x01, 0x0E, 0xF0, 0x01, 0x73, 0x01, 0xD5, 0x04, 0x01, EEPROM.read(97), EEPROM.read(98), EEPROM.read(99)};
      Serial.println("ECHO ELECTRIC ENERGY SENSOR NODE");
      UDPSender.beginPacketMulticast(ipMulti, portMulti, WiFi.localIP());
      for (int i = 0; i < sizeof(echoMyNode); i++) {
        UDPSender.write(echoMyNode[i]);
      }
      UDPSender.endPacket();
      WiFi.softAPdisconnect(true);
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
    Serial.print(WiFi.status());
    c++;
  }
  Serial.println("");
  Serial.println("Connect timed out, opening AP");
  return false;
}

void launchWeb(int webtype) {
  Serial.println("");
  Serial.println("WiFi connected");
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
  Serial.println("scan done");
  if (n == 0)
    Serial.println("no networks found");
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
  WiFi.softAP(ssid);//, passphrase, 6);
  Serial.println("softap");
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
      content += "<tr><td>EOJ Code : </td><td><input type='text' name='X1'/ value='" + (String)EEPROM.read(97) + "' size='3'><input type='text' name='X2'/ value='" + (String)EEPROM.read(98) + "' size='3'>";
      content += "<input type='text' name='X3'/ value='" + (String)EEPROM.read(99) + "' size='3'><p style='font-size:11'>Example: 0x00 = 0; 0x22 = 34; 0x01 = 1</p></td></tr>";
      content += "<td>Operation Status : </td><td><input type='radio' value='ON' name='OperationStatus'";
      EEPROM.read(100) == 0x30 ? content += "checked" : "";
      content += ">ON<input type='radio' value='OFF' name='OperationStatus'";
      EEPROM.read(100) == 0x31 ? content += "checked" : "";
      content += ">OFF</td></tr><tr><td>Power Limited   : </td><td><input type='number' name='PowerLimit' value='";
      content += int((EEPROM.read(102) & 0xff) << 8) | EEPROM.read(101);
      content += "'/>&nbsp;W</td></tr><tr><td>Location : </td><td><input type='text' name='Location' value='Not set yet' disabled/></td></tr><tr><td></td><td><input type='submit' name='Submit'/></td></tr></table></center></form>";
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
      String qOstatus = server.arg("OperationStatus");
      String qPower = server.arg("PowerLimit");
      String qX1 = server.arg("X1");
      String qX2 = server.arg("X2");
      String qX3 = server.arg("X3");
      if (qOstatus.length() > 0 && qPower.length() > 0 && qX1.length() > 0 && qX2.length() > 0 && qX3.length() > 0) {
        Serial.println("Update status and power limit");
        if (qOstatus.equals("ON")) {
          EEPROM.write(100, 0x30);
          digitalWrite(LED_BUILTIN, HIGH);
          digitalWrite(4, HIGH);
          Serial.println("Device on");
        } else if (qOstatus.equals("OFF")) {
          EEPROM.write(100, 0x31);
          digitalWrite(LED_BUILTIN, LOW);
          digitalWrite(4, LOW);
          Serial.println("Device off");
        } else {
          Serial.println("Update status failed");
        }
        if(EEPROM.read(101) != (qPower.toInt() & 0xff)) {
          EEPROM.write(101, qPower.toInt() & 0xff);
        }
        if(EEPROM.read(102) != ((qPower.toInt() >> 8) & 0xff)) {
          EEPROM.write(102, (qPower.toInt() >> 8) & 0xff);
        }
        if(EEPROM.read(97) != qX1.toInt()) {
          EEPROM.write(97, qX1.toInt());
        }
        if(EEPROM.read(98) != qX2.toInt()) {
          EEPROM.write(98, qX2.toInt());
        }
        if(EEPROM.read(99) != qX3.toInt()) {
          EEPROM.write(99, qX3.toInt());
        }
        EEPROM.commit();
      }
      server.sendContent("HTTP/1.1 301 OK\r\nLocation: /\r\nCache-Control: no-cache\r\n\r\n");
    });
  }
}
int count = 0;
void loop() {
  server.handleClient();
  // follow device status to on/off
  byte deviceStatus = EEPROM.read(100);
  if (deviceStatus == 0x30) {
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(4, HIGH);
  } else if (deviceStatus == 0x31) {
    digitalWrite(LED_BUILTIN, LOW);
    digitalWrite(4, LOW);
  } else {
    Serial.println("Can't get deviceStatus. Set it!");
  }
  int noBytes = UDPReceiver.parsePacket();
  if (noBytes) {
    byte request[noBytes];
    for (int i = 0; i < noBytes; i++) {
      request[i] = UDPReceiver.read();
    }
    Serial.print("Recevied a packet........");
    packet a;
    a.readPacket(request,noBytes);
    a.printPacket();
    byte req_EOJ[] = {request[4], request[5], request[6]};
    byte trans_ID[] = {request[2], request[3]};
    byte ESV = request[10];
    byte OPC = request[11];
    byte EPC = request[12];
    IPAddress requestIP = UDPReceiver.remoteIP();
    int requestPort = UDPReceiver.remotePort();

      for(int i=0;i<OPC;i++){
        int pPDC=pEPC+1;//position of PDC
        int pEDT=pPDC+1;
        switch (request[pEPC]) {
          //device status
          case OperationStatus: {
  
              //byte response[] = {0x10, 0x81, trans_ID[0], trans_ID[1], EEPROM.read(97), EEPROM.read(98), EEPROM.read(99), req_EOJ[0], req_EOJ[1], req_EOJ[2], Get_Res, 0x01, 0x80, 0x01, EEPROM.read(100)};
              //byte response[] = {0x10, 0x81, trans_ID[0], trans_ID[1], EEPROM.read(97), EEPROM.read(98), EEPROM.read(99), req_EOJ[0], req_EOJ[1], req_EOJ[2], Get_Res, 0x02, 0x80, 0x01, EEPROM.read(100), 0xE1, 0x04, bytes[0], bytes[1], bytes[2], bytes[3]};
              //UDPSender.beginPacketMulticast(ipMulti, portMulti, WiFi.localIP());
              response[pEPC]=request[pEPC];
              response[pPDC]=1;
              for(int i=0;i<response[pPDC];i++){
                response[pEDT+i]=EEPROM.read(100+i);
              }
              
//              UDPSender.beginPacket(requestIP, requestPort);
//              for (int i = 0; i < sizeof(response); i++) {
//                UDPSender.write(response[i]);
//              }
//              UDPSender.endPacket();
//              Serial.println("Sended Status packet");
              break;
            }
//          //info my node
//          case 0xD6: {
//              byte response[] = {0x10, 0x81, trans_ID[0], trans_ID[1], 0x0E, 0xF0, 0x01, 0x0E, 0xF0, 0x01, INF, 0x01, 0xD5, 0x04, 0x01, EEPROM.read(97), EEPROM.read(98), EEPROM.read(99)};
//              //UDPSender.beginPacketMulticast(ipMulti, portMulti, WiFi.localIP());
//              UDPSender.beginPacket(requestIP, requestPort);
//              for (int i = 0; i < sizeof(response); i++) {
//                UDPSender.write(response[i]);
//              }
//              UDPSender.endPacket();
//              Serial.println("Sended info packet");
//              break;
//            }
          
          //medium capacity
          case 0xE1: {
              float capacity = (float) ((getMaxValue() - 500) * (5.0 / 1024) * 1000 / 118) / sqrt(2) * 220;
              thing.floatValue = capacity;
              byte bytes[4] = {0, 0, 0, 0};
              for (int i = 0; i < 4; i++) {
                bytes[i] = thing.bytes[3 - i];
              }
//              byte response[] = {0x10, 0x81, trans_ID[0], trans_ID[1], EEPROM.read(97), EEPROM.read(98), EEPROM.read(99), req_EOJ[0], req_EOJ[1], req_EOJ[2], Get_Res, 0x01, 0xE1, 0x04, bytes[0], bytes[1], bytes[2], bytes[3]};
              //UDPSender.beginPacketMulticast(ipMulti, portMulti, WiFi.localIP());
              response[pEPC]=request[pEPC];
              response[pPDC]=4;
              for (int i = 0; i < 4; i++) {
                response[pEDT+i] = bytes[i];
              }
//              for(int i=0;i<response[pPDC];i++){
//                response[pPDT+i]=EEPROM.read(100+i);
//              }
//              UDPSender.beginPacket(requestIP, requestPort);
//              for (int i = 0; i < sizeof(response); i++) {
//                UDPSender.write(response[i]);
//              }
//              UDPSender.endPacket();
//              Serial.print("Sended capacity medium ");
//              Serial.println(capacity);
              break;
            }
          //power limit 2 bytes
          case 0x99: {
//              byte response[] = {0x10, 0x81, trans_ID[0], trans_ID[1], EEPROM.read(97), EEPROM.read(98), EEPROM.read(99), req_EOJ[0], req_EOJ[1], req_EOJ[2], Get_Res, 0x01, 0x99, 0x02, EEPROM.read(101), EEPROM.read(102)};
              //UDPSender.beginPacketMulticast(ipMulti, portMulti, WiFi.localIP());
              response[pEPC]=request[pEPC];
              response[pPDC]=2;
              for (int i = 0; i < 2; i++) {
                response[pEDT+i] = EEPROM.read(101+i);
              }
//              UDPSender.beginPacket(requestIP, requestPort);
//              for (int i = 0; i < sizeof(response); i++) {
//                UDPSender.write(response[i]);
//              }
//              UDPSender.endPacket();
//              Serial.println("Sended power limit");
              break;
            }
          
          
          default : {
              Serial.print("EPC Code not found");
              Serial.println(EPC);
            }
        }
        
        pEPC=pEPC+2+response[pPDC];
      }
      UDPSender.beginPacket(requestIP, requestPort);
      for (int i = 0; i < sizeof(response); i++) {
        UDPSender.write(response[i]);
      }
      UDPSender.endPacket();
      // write response
      
    }


    else if (ESV == SetC) {
      //do do dai goi tin
      int pEPC=12;
      int l=11; // do dai goi tin gui di
      
      // dem do dai goi tin can gui 
      for(int i=0;i<OPC;i++){
        int pPDC=pEPC+1;//position of PDC
        switch (request[pEPC]) {
          //Device Object Super Class Properties
          //Operation status
          case 0x80: {
              l+=2+1;
              OPC++;
              break;
          }
          //power limit 2 bytes
          case 0x99: {
              l+=2+2;
              OPC++;
              break;
          }
          //~Device Object Super Class Properties
          //General Light
          //~General Light
          default : {
              Serial.print("EPC Code not found");
              Serial.println(EPC);
          }
        }
        
        pEPC=pEPC+2+request[pPDC];
      }
      

      ////////////////////////
      pEPC=12;
      byte response[l];
      byte head_response[12]={0x10, 0x81, trans_ID[0], trans_ID[1], EEPROM.read(97), EEPROM.read(98), EEPROM.read(99), req_EOJ[0], req_EOJ[1], req_EOJ[2], Set_Res,OPC};
      for(int i=0;i<12;i++){
        response[i]=head_response[i];
      }
      for(int i=0;i<OPC;i++){
        int pPDC=pEPC+1;
        int pEDT=pOPC+1;
        switch(request[pEPC]){
          case 0x80:{
            EEPROM.write(100, request[14]);
            delay(5);
            EEPROM.commit();
            if(EEPROM.read(100)==0x30){
              Serial.print(EEPROM.read(100));
              digitalWrite(4,HIGH);
            }else if(EEPROM.read(100)==0x31){
              digitalWrite(4,LOW);
              Serial.print(EEPROM.read(100));
            }
            
            Serial.println("Sended SET status response packet! OK!");
            response[pEPC]=request[pEPC]
          }
        }


        pEPC=pEPC+2+request[pPDC];
      }
      if (EPC == OperationStatus) {
        //byte response[] = {0x10, 0x81, trans_ID[0], trans_ID[1], EEPROM.read(97), EEPROM.read(98), EEPROM.read(99), req_EOJ[0], req_EOJ[1], req_EOJ[2], Set_Res, 0x01, OperationStatus, 0x01};
        //UDPSender.beginPacketMulticast(ipMulti, portMulti, WiFi.localIP());       
        EEPROM.write(100, request[14]);
        delay(5);
        EEPROM.commit();
        if(EEPROM.read(100)==0x30){
          Serial.print(EEPROM.read(100));
          digitalWrite(4,HIGH);
        }else if(EEPROM.read(100)==0x31){
          digitalWrite(4,LOW);
          Serial.print(EEPROM.read(100));
        }
        
        Serial.println("Sended SET status response packet! OK!");
        response[]
//        UDPSender.beginPacket(requestIP, requestPort);
//        for (int i = 0; i < sizeof(response); i++) {
//          UDPSender.write(response[i]);
//        }
//        UDPSender.endPacket();
        
      } else if (EPC == PowerLimit) {
        //set power limit - 2 bytes
        byte response[] = {0x10, 0x81, trans_ID[0], trans_ID[1], EEPROM.read(97), EEPROM.read(98), EEPROM.read(99), req_EOJ[0], req_EOJ[1], req_EOJ[2], Set_Res, 0x01, OperationStatus, 0x01};
        //UDPSender.beginPacketMulticast(ipMulti, portMulti, WiFi.localIP());
        UDPSender.beginPacket(requestIP, requestPort);
        for (int i = 0; i < sizeof(response); i++) {
          UDPSender.write(response[i]);
        }
        UDPSender.endPacket();
        EEPROM.write(101, request[14]);
        EEPROM.write(102, request[15]);
        EEPROM.commit();
        //        Serial.print(EEPROM.read(101));
        //        Serial.print("-");
        //        Serial.print(EEPROM.read(102));
        //        Serial.print(" to int ");
        //        int value = ((EEPROM.read(102) & 0xff) << 8) | EEPROM.read(101);
        //        Serial.println(value);
        //        byte low  = (value & 0xff);
        //        byte high = (value >> 8) & 0xff;
        //        Serial.print(value);
        //        Serial.print(" to bytes : ");
        //        Serial.print(low, HEX);
        //        Serial.print("-");
        //        Serial.println(high, HEX);
        Serial.println("Sended SET power limit response packet! OK!");
      } else {
        Serial.println("Another SET request!");
      }
    } else if (ESV == INF) {
      Serial.println("Another node found");
    } else {
      Serial.print("ESV Code Not Found :");
      Serial.println(ESV);
    }
  } else {
    delay(1000);
    count++;
    if (count > 300) {
      byte echoMyNode[] = {0x10, 0x81, 0x00, 0x00, 0x0E, 0xF0, 0x01, 0x0E, 0xF0, 0x01, 0x73, 0x01, 0xD5, 0x04, 0x01, EEPROM.read(97), EEPROM.read(98), EEPROM.read(99)};
      Serial.println("ECHO ELECTRIC ENERGY SENSOR NODE");
      UDPSender.beginPacketMulticast(ipMulti, portMulti, WiFi.localIP());
      //UDPSender.beginPacket(ipMulti, portMulti);
      for (int i = 0; i < sizeof(echoMyNode); i++) {
        UDPSender.write(echoMyNode[i]);
      }
      UDPSender.endPacket();
      count = 0;
    }
  }
}

int getMaxValue()
{
  int sensorValue;    //value read from the sensor
  int sensorMax = 0;
  uint32_t start_time = millis();
  while ((millis() - start_time) < 1000) //sample for 1000ms
  {
    sensorValue = analogRead(A0);
    //delay(100);
    if (sensorValue > sensorMax)
    {
      /*record the maximum sensor value*/

      sensorMax = sensorValue;
    }
  }
  return sensorMax - 3;
}
