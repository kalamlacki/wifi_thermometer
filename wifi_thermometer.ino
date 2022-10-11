// Copy right Łukasz Kalamłacki
// License GPLv3 https://www.gnu.org/licenses/gpl-3.0.html

#include <EEPROM.h>
#include <CRC32.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <ESP8266WebServer.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 2
#define DEFAULT_SSID "lk_e8266_thermo"

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

ESP8266WebServer server(80);

WiFiUDP Udp;

// Multicast declarations
IPAddress ipMulti(239, 13, 12, 81);
unsigned int portMulti = 3333;
char packetBuffer[65]; 

const int led = 16;

struct Config {
  char SSID_NAME[100];
  char PASSWD[100];
  char ALIAS[256];
  uint32_t crc;
  uint32_t isOK;
} config;

const PROGMEM char* confPage = "<html><body>"
"<h1>The configuration of the device:</h1>"
"<form method=\"POST\" action=\"/config\"><table border=\"0\">"
"<tr><td>Name of the thermometer:</td><td><input type=\"text\" name=\"alias\"/></td></tr>"
"<tr><td>WI-FI network name:</td><td><input type=\"text\" name=\"ssid\"/></td></tr>"
"<tr><td>WI-FI password:</td><td><input type=\"password\" name=\"passwd\"/></td></tr>"
"<tr><td colspan=\"2\" style=\"text-align:center;\"><input type=\"submit\" name=\"submit\" value=\"Set up config\"/>"
"<button type=\"submit\" formaction=\"/serial\">Check current settings</button>"
"</td></tr></table>"
"</form></body></html>";

int read_config() {
  int size = sizeof(config) - 4;
  for(int i=0; i<size; i++) {
    ((unsigned char*)&config)[i] = EEPROM.read(i);
  }
  uint32_t crc32 = CRC32::calculate(((unsigned char*)&config), size - 4);
  if(crc32 == config.crc && strlen(config.SSID_NAME) > 0) {
    config.isOK = 1;
    return 1;
  }
  else {
    memset(config.SSID_NAME, 0, sizeof(config.SSID_NAME));
    memset(config.PASSWD, 0, sizeof(config.PASSWD));
    memset(config.ALIAS, 0, sizeof(config.ALIAS));
    config.isOK=0;
    return 0;
  }
 
}


int write_config() {
  int size = sizeof(config) - 4;
  uint32_t crc32 = CRC32::calculate(((unsigned char*)&config), size - 4);
  config.crc = crc32;
  for(int i=0; i<size; i++) {
    EEPROM.write(i, ((unsigned char*)&config)[i]);
  }
  
  if(EEPROM.commit()) {
    config.isOK = 1;
    return 1;
  }
  else {
    config.isOK = 0;
    return 0;
  }
}

void handleSerial() {
  String message = "WI-FI mac: ";
  message += WiFi.macAddress();
  message += "\nName of the device: ";
  message += config.ALIAS;
  message += "\nConnected to WI-FI: ";
  message += WiFi.status() != WL_CONNECTED ? "NO" : "YES";
  message += "\nConnected WI-FI network: ";
  message += config.SSID_NAME;
  server.send(200, "text/plain", message);
  
}

void handleTemp() {
  char temp[10];
  String message = "DS18B20 Temperature=";
  digitalWrite(led, 1);
  sensors.requestTemperatures();
  sprintf(temp, "%.2f", sensors.getTempCByIndex(0));
  message += temp;
  message += "C";
  server.send(200, "text/plain", message);
  digitalWrite(led, 0);
}

void handleConfig() {
  int code;
  String tmp;
  String message = "Trying to set up new WI-FI configuration:\n\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    if(server.argName(i) == "ssid" && server.arg(i).length() > 0) {
      strcpy(config.SSID_NAME, server.arg(i).c_str());
    }
    else if(server.argName(i) == "passwd" && server.arg(i).length() > 0) {
      strcpy(config.PASSWD, server.arg(i).c_str());
    }
    else if(server.argName(i) == "alias" && server.arg(i).length() > 0 ) {
      strcpy(config.ALIAS, server.arg(i).c_str());
    }
    else {
      message += "\nUnknown parameter: ";
      message += server.argName(i);
    }
  }
  if(strlen(config.PASSWD) > 0 && strlen(config.SSID_NAME) > 0 && strlen(config.ALIAS) > 0) {
    tmp = "Saving WI-FI configuration...";
    Serial.println(tmp);
    message += "\n";
    message += tmp;
    if(write_config()) {
      tmp = "Saving was successfull!";
      Serial.println(tmp);
      message += "\n";
      message += tmp;
      code = 200;
    }
    else {
      tmp = "I could not save configuration to EEPROM!";
      Serial.println(tmp);
      message += "\n";
      message += tmp;
      code = 500;
    }
  } 
  else {
    tmp = "Missing ssid or/and passwd or/and alias parameters";
    Serial.println(tmp);
    message += "\n";
    message += tmp;
    code = 400;
  }
  
  server.send(code, "text/plain", message);
}

void handleRoot() {
  String message = confPage;
  server.send(200, "text/html", message);
}

void setup() {
  pinMode(led, OUTPUT);
  Serial.begin(115200);
  EEPROM.begin(sizeof(config));
  Serial.println("NodeMCU started....");
  sensors.begin();
 
  if(read_config()) {
    Serial.print("CRC32 of Config struct is OK, attempting to connect to: ");
    Serial.println(config.SSID_NAME);
    WiFi.mode(WIFI_STA);
    WiFi.begin(config.SSID_NAME, config.PASSWD);
    Serial.println("");

    while (WiFi.status() != WL_CONNECTED) {
       delay(1000);
       supportInputSerial();
       Serial.print(".");
     }
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(config.SSID_NAME);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    
  }
  else {
    Serial.println("CRC32 of Config struct FAILED, working as AP");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(DEFAULT_SSID);

    IPAddress myIP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(myIP);
  }
  server.on("/", handleRoot);
  server.on("/config", handleConfig);
  server.on("/temp", handleTemp);
  server.on("/serial", handleSerial);
  server.begin();
  Serial.println("HTTP server started");
  Udp.beginMulticast(WiFi.localIP(), ipMulti, portMulti);
}

void loop() {
  server.handleClient();
  supportInputSerial();
  delay(33);
  int noBytes=0;
  int message_size=0;
  noBytes= Udp.parsePacket();
  if ( noBytes>0 && noBytes<64){
    Serial.print(":Packet of ");
    Serial.print(noBytes);
    Serial.print(" received from ");
    Serial.print(Udp.remoteIP());
    Serial.print(":");
    Serial.println(Udp.remotePort());
    message_size=Udp.read(packetBuffer,64); // read the packet into the buffer
    Serial.print("Message Size: ");
    Serial.println(message_size);
    
    if (message_size>0) {
      packetBuffer[message_size]=0; //// null terminate
      Serial.println(packetBuffer);
      if(strcmp(packetBuffer, "lk_e8266_thermo_search") == 0) {
        delay(200);
        String reply = "IP=";
        reply += WiFi.localIP().toString();
        reply += ", MAC=";
        reply += WiFi.macAddress();
        Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
        Udp.write(reply.c_str());
        Udp.endPacket();
        Serial.println("Packet has been sent: ");
        Serial.println(reply);
      }
    }
    delay(100);
  }
}

void supportInputSerial() { 
  if (Serial.available() > 0) {
    uint8_t inByte = Serial.read();
    if(inByte == 'R') {
      memset(config.SSID_NAME, 0, sizeof(config.SSID_NAME));
      memset(config.PASSWD, 0, sizeof(config.PASSWD));
      write_config(); 
      Serial.println("WI-FI config has been erased, press RESET to continue...");
    }
    else {
      Serial.print("You wrote: ");
      Serial.println(inByte);
      Serial.println("but I can accept only R character to reset device in to AP mode!");
    }
  }
}
