///*
//  Blink without Delay
//
//  Turns on and off a light emitting diode (LED) connected to a digital pin,
//  without using the delay() function. This means that other code can run at the
//  same time without being interrupted by the LED code.
//
//  The circuit:
//  - Use the onboard LED.
//  - Note: Most Arduinos have an on-board LED you can control. On the UNO, MEGA
//    and ZERO it is attached to digital pin 13, on MKR1000 on pin 6. LED_BUILTIN
//    is set to the correct LED pin independent of which board is used.
//    If you want to know what pin the on-board LED is connected to on your
//    Arduino model, check the Technical Specs of your board at:
//    https://www.arduino.cc/en/Main/Products
//
//  created 2005
//  by David A. Mellis
//  modified 8 Feb 2010
//  by Paul Stoffregen
//  modified 11 Nov 2013
//  by Scott Fitzgerald
//  modified 9 Jan 2017
//  by Arturo Guadalupi
//
//  This example code is in the public domain.
//
//  http://www.arduino.cc/en/Tutorial/BlinkWithoutDelay
//*/
//
//// constants won't change. Used here to set a pin number:
//const int ledPin =  4;// the number of the LED pin
//
//// Variables will change:
//int ledState = LOW;             // ledState used to set the LED
//
//// Generally, you should use "unsigned long" for variables that hold time
//// The value will quickly become too large for an int to store
//unsigned long previousMillis = 0;        // will store last time LED was updated
//
//// constants won't change:
//const long interval = 1000;           // interval at which to blink (milliseconds)
//
//void setup() {
//  // set the digital pin as output:
//  pinMode(ledPin, OUTPUT);
//}
//
//void loop() {
//  // here is where you'd put code that needs to be running all the time.
//
//  // check to see if it's time to blink the LED; that is, if the difference
//  // between the current time and last time you blinked the LED is bigger than
//  // the interval at which you want to blink the LED.
//  unsigned long currentMillis = millis();
//
//  if (currentMillis - previousMillis >= interval) {
//    // save the last time you blinked the LED
//    previousMillis = currentMillis;
//
//    // if the LED is off turn it on and vice-versa:
//    if (ledState == LOW) {
//      ledState = HIGH;
//    } else {
//      ledState = LOW;
//    }
//
//    // set the LED with the ledState of the variable:
//    digitalWrite(4, ledState);
//  }
//}

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
      content += st;
      content += "</p><form method='get' action='setting'><label>SSID: </label><input name='ssid' length=32><input name='pass' length=64><input type='submit'></form></body>";
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
      content += "'/>&nbsp;W</td></tr><tr><td>Location : </td><td><input type='text' name='Location' value='Not set yet' disabled/></td></tr><tr><td></td><td><input type='submit' name='Submit'/></td></tr></table></center></form></body></html>";
      server.send(200, "text/html", content);
    });
    server.on("/clear", []() {
      content = "<!DOCTYPE HTML>\r\n<html>";
      content += "<p>Clearing the EEPROM</p></html>";
      server.send(200, "text/html", content);
      Serial.println("clearing ssid and pass");
      for (int i = 0; i < 96; ++i) {
        EEPROM.write(i, 0);
      }
      EEPROM.commit();
    });
    server.on("/update", []() {
      String qOstatus = server.arg("OperationStatus");
      String qPower = server.arg("PowerLimit");
      String qX1 = server.arg("X1");
      String qX2 = server.arg("X2");
      String qX3 = server.arg("X3");
      if (qOstatus.length() > 0 && qPower.length() > 0 && qX1.length() > 0 && qX2.length() > 0 && qX3.length() > 0) {
        Serial.println("Update status and power limit");
        EEPROM.write(100, 0);
        EEPROM.write(101, 0);
        EEPROM.write(102, 0);
        Serial.println("writing eeprom status and power limit");
        if (qOstatus.equals("ON")) {
          EEPROM.write(100, 0x30);
        } else if (qOstatus.equals("OFF")) {
          EEPROM.write(100, 0x31);
        } else {
          Serial.println("Update status failed");
        }
        EEPROM.write(101, qPower.toInt() & 0xff);
        EEPROM.write(102, (qPower.toInt() >> 8) & 0xff);
        EEPROM.write(97, qX1.toInt());
        EEPROM.write(98, qX2.toInt());
        EEPROM.write(99, qX3.toInt());
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
    byte req_EOJ[] = {request[4], request[5], request[6]};
    byte trans_ID[] = {request[2], request[3]};
    byte ESV = request[10];
    byte OPC = request[11];
    byte EPC = request[12];
    IPAddress requestIP = UDPReceiver.remoteIP();
    int requestPort = UDPReceiver.remotePort();
    // nhan request yeu cau gui trang thai nhieu thuoc tinh
    if (ESV == Get) {
      // read request
      int pEPC=12;
      // truong hop goi tin co nhieu thuoc tinh
      for(int i=0;i<OPC;i++){
        int pPDC=pEPC+1;//position of PDC
        switch (request[pEPC]) {
          //device status
          case OperationStatus: {
  
              byte response[] = {0x10, 0x81, trans_ID[0], trans_ID[1], EEPROM.read(97), EEPROM.read(98), EEPROM.read(99), req_EOJ[0], req_EOJ[1], req_EOJ[2], Get_Res, 0x01, 0x80, 0x01, EEPROM.read(100)};
              //byte response[] = {0x10, 0x81, trans_ID[0], trans_ID[1], EEPROM.read(97), EEPROM.read(98), EEPROM.read(99), req_EOJ[0], req_EOJ[1], req_EOJ[2], Get_Res, 0x02, 0x80, 0x01, EEPROM.read(100), 0xE1, 0x04, bytes[0], bytes[1], bytes[2], bytes[3]};
              //UDPSender.beginPacketMulticast(ipMulti, portMulti, WiFi.localIP());
              UDPSender.beginPacket(requestIP, requestPort);
              for (int i = 0; i < sizeof(response); i++) {
                UDPSender.write(response[i]);
              }
              UDPSender.endPacket();
              Serial.println("Sended Status packet");
              break;
            }
          //info my node
          case 0xD6: {
              byte response[] = {0x10, 0x81, trans_ID[0], trans_ID[1], 0x0E, 0xF0, 0x01, 0x0E, 0xF0, 0x01, INF, 0x01, 0xD5, 0x04, 0x01, EEPROM.read(97), EEPROM.read(98), EEPROM.read(99)};
              //UDPSender.beginPacketMulticast(ipMulti, portMulti, WiFi.localIP());
              UDPSender.beginPacket(requestIP, requestPort);
              for (int i = 0; i < sizeof(response); i++) {
                UDPSender.write(response[i]);
              }
              UDPSender.endPacket();
              Serial.println("Sended info packet");
              break;
            }
          // integaral
          case 0xE0: {
              byte response[] = {0x10, 0x81, trans_ID[0], trans_ID[1], EEPROM.read(97), EEPROM.read(98), EEPROM.read(99), req_EOJ[0], req_EOJ[1], req_EOJ[2], Get_Res, 0x01, 0xE0, 0x04, 0x00, 0x00, 0x00, 0x00};
              //UDPSender.beginPacketMulticast(ipMulti, portMulti, WiFi.localIP());
              UDPSender.beginPacket(requestIP, requestPort);
              for (int i = 0; i < sizeof(response); i++) {
                UDPSender.write(response[i]);
              }
              UDPSender.endPacket();
              Serial.println("Sended integaral medium");
              break;
            }
          //medium capacity
          case 0xE1: {
              float capacity = (float) ((getMaxValue() - 500) * (5.0 / 1024) * 1000 / 118) / sqrt(2) * 220;
              thing.floatValue = capacity;
              byte bytes[4] = {0, 0, 0, 0};
              for (int i = 0; i < 4; i++) {
                bytes[i] = thing.bytes[3 - i];
              }
              byte response[] = {0x10, 0x81, trans_ID[0], trans_ID[1], EEPROM.read(97), EEPROM.read(98), EEPROM.read(99), req_EOJ[0], req_EOJ[1], req_EOJ[2], Get_Res, 0x01, 0xE1, 0x04, bytes[0], bytes[1], bytes[2], bytes[3]};
              //UDPSender.beginPacketMulticast(ipMulti, portMulti, WiFi.localIP());
              UDPSender.beginPacket(requestIP, requestPort);
              for (int i = 0; i < sizeof(response); i++) {
                UDPSender.write(response[i]);
              }
              UDPSender.endPacket();
              Serial.print("Sended capacity medium ");
              Serial.println(capacity);
              break;
            }
          //power limit 2 bytes
          case 0x99: {
              byte response[] = {0x10, 0x81, trans_ID[0], trans_ID[1], EEPROM.read(97), EEPROM.read(98), EEPROM.read(99), req_EOJ[0], req_EOJ[1], req_EOJ[2], Get_Res, 0x01, 0x99, 0x02, EEPROM.read(101), EEPROM.read(102)};
              //UDPSender.beginPacketMulticast(ipMulti, portMulti, WiFi.localIP());
              UDPSender.beginPacket(requestIP, requestPort);
              for (int i = 0; i < sizeof(response); i++) {
                UDPSender.write(response[i]);
              }
              UDPSender.endPacket();
              Serial.println("Sended power limit");
              break;
            }
          //installation location
          case 0x81: {
              Serial.println("Get location request, not yet!");
              break;
            }
          //        case E2; break;
          //        case E3; break;
          //        case E4; break;
          //        case E5; break;
          default : {
              Serial.print("EPC Code not found");
              Serial.println(EPC);
            }
        }
        
        pEPC=pEPC+2+request[pPDC];
      }
      // write response
      
    }
      switch (EPC) {
        //device status
        case OperationStatus: {

            byte response[] = {0x10, 0x81, trans_ID[0], trans_ID[1], EEPROM.read(97), EEPROM.read(98), EEPROM.read(99), req_EOJ[0], req_EOJ[1], req_EOJ[2], Get_Res, 0x01, 0x80, 0x01, EEPROM.read(100)};
            //byte response[] = {0x10, 0x81, trans_ID[0], trans_ID[1], EEPROM.read(97), EEPROM.read(98), EEPROM.read(99), req_EOJ[0], req_EOJ[1], req_EOJ[2], Get_Res, 0x02, 0x80, 0x01, EEPROM.read(100), 0xE1, 0x04, bytes[0], bytes[1], bytes[2], bytes[3]};
            //UDPSender.beginPacketMulticast(ipMulti, portMulti, WiFi.localIP());
            UDPSender.beginPacket(requestIP, requestPort);
            for (int i = 0; i < sizeof(response); i++) {
              UDPSender.write(response[i]);
            }
            UDPSender.endPacket();
            Serial.println("Sended Status packet");
            break;
          }
        //info my node
        case 0xD6: {
            byte response[] = {0x10, 0x81, trans_ID[0], trans_ID[1], 0x0E, 0xF0, 0x01, 0x0E, 0xF0, 0x01, INF, 0x01, 0xD5, 0x04, 0x01, EEPROM.read(97), EEPROM.read(98), EEPROM.read(99)};
            //UDPSender.beginPacketMulticast(ipMulti, portMulti, WiFi.localIP());
            UDPSender.beginPacket(requestIP, requestPort);
            for (int i = 0; i < sizeof(response); i++) {
              UDPSender.write(response[i]);
            }
            UDPSender.endPacket();
            Serial.println("Sended info packet");
            break;
          }
        // integaral
        case 0xE0: {
            byte response[] = {0x10, 0x81, trans_ID[0], trans_ID[1], EEPROM.read(97), EEPROM.read(98), EEPROM.read(99), req_EOJ[0], req_EOJ[1], req_EOJ[2], Get_Res, 0x01, 0xE0, 0x04, 0x00, 0x00, 0x00, 0x00};
            //UDPSender.beginPacketMulticast(ipMulti, portMulti, WiFi.localIP());
            UDPSender.beginPacket(requestIP, requestPort);
            for (int i = 0; i < sizeof(response); i++) {
              UDPSender.write(response[i]);
            }
            UDPSender.endPacket();
            Serial.println("Sended integaral medium");
            break;
          }
        //medium capacity
        case 0xE1: {
            float capacity = (float) ((getMaxValue() - 500) * (5.0 / 1024) * 1000 / 118) / sqrt(2) * 220;
            thing.floatValue = capacity;
            byte bytes[4] = {0, 0, 0, 0};
            for (int i = 0; i < 4; i++) {
              bytes[i] = thing.bytes[3 - i];
            }
            byte response[] = {0x10, 0x81, trans_ID[0], trans_ID[1], EEPROM.read(97), EEPROM.read(98), EEPROM.read(99), req_EOJ[0], req_EOJ[1], req_EOJ[2], Get_Res, 0x01, 0xE1, 0x04, bytes[0], bytes[1], bytes[2], bytes[3]};
            //UDPSender.beginPacketMulticast(ipMulti, portMulti, WiFi.localIP());
            UDPSender.beginPacket(requestIP, requestPort);
            for (int i = 0; i < sizeof(response); i++) {
              UDPSender.write(response[i]);
            }
            UDPSender.endPacket();
            Serial.print("Sended capacity medium ");
            Serial.println(capacity);
            break;
          }
        //power limit 2 bytes
        case 0x99: {
            byte response[] = {0x10, 0x81, trans_ID[0], trans_ID[1], EEPROM.read(97), EEPROM.read(98), EEPROM.read(99), req_EOJ[0], req_EOJ[1], req_EOJ[2], Get_Res, 0x01, 0x99, 0x02, EEPROM.read(101), EEPROM.read(102)};
            //UDPSender.beginPacketMulticast(ipMulti, portMulti, WiFi.localIP());
            UDPSender.beginPacket(requestIP, requestPort);
            for (int i = 0; i < sizeof(response); i++) {
              UDPSender.write(response[i]);
            }
            UDPSender.endPacket();
            Serial.println("Sended power limit");
            break;
          }
        //installation location
        case 0x81: {
            Serial.println("Get location request, not yet!");
            break;
          }
        //        case E2; break;
        //        case E3; break;
        //        case E4; break;
        //        case E5; break;
        default : {
            Serial.print("EPC Code not found");
            Serial.println(EPC);
          }
      }

    } else if (ESV == SetC) {
      if (EPC == OperationStatus) {
        byte response[] = {0x10, 0x81, trans_ID[0], trans_ID[1], EEPROM.read(97), EEPROM.read(98), EEPROM.read(99), req_EOJ[0], req_EOJ[1], req_EOJ[2], Set_Res, 0x01, OperationStatus, 0x01};
        //UDPSender.beginPacketMulticast(ipMulti, portMulti, WiFi.localIP());
        UDPSender.beginPacket(requestIP, requestPort);
        for (int i = 0; i < sizeof(response); i++) {
          UDPSender.write(response[i]);
        }
        UDPSender.endPacket();
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
      } else if (EPC == Location) {
        //set location
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
    if (count > 3) {
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
