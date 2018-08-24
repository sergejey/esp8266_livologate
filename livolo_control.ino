#include <ESP8266WiFi.h>
#include <PubSubClient.h>

//needed for library
#include <LivoloTx.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h> 
#include <ArduinoOTA.h>
#include <EEPROM.h>
#include <ESP8266HTTPClient.h>


//#include <SoftwareSerial.h>

IPAddress ip(192,168,1,1);

const int EEPROM_MQTT_SERVER=50;
const int EEPROM_MQTT_TOPIC=100;
const int EEPROM_STRING_MAX=50;

const int ENABLE_DEBUG_PRINT = 0;

int latestKey = 0;
int latestRemoteId = 0;

String latestSerialData;
String latestRequest;
const byte numChars = 200;
char receivedChars[numChars]; // an array to store the received data
boolean newData = false;

// Update these with values suitable for your network.

String mqtt_server;
char* mqtt_user = "";
char* mqtt_password = "";
String mqtt_topic_out;

#include <Ticker.h>
Ticker ticker;
ESP8266WebServer server(80);

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
long lastConnectTry = 0;
char msg[50];
int value = 0;
int commandId=0;

String http_server;

#define TX_PIN D0 // Connect tx pin
LivoloTx gLivolo(TX_PIN);

     const uint8_t LIVOLO_KEY_MAX = 10;
     const uint8_t LIVOLO_KEYS[LIVOLO_KEY_MAX] = {
      0,
      96,
      120,
      24,
      80,
      48,
      108,
      12,
      72,
      40,
    };
    static const uint8_t LIVOLO_KEY_OFF = 106;


void callback(char* topic, byte* payload, unsigned int length) {
  /*
  debugPrint("Message arrived [");
  debugPrint(topic);
  debugPrint("] ");
  for (int i = 0; i < length; i++) {
    debugPrint((char)payload[i]);
  }
  debugPrintln();
  */

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }

}

void debugPrint(String data) {
  if (ENABLE_DEBUG_PRINT == 1) {
   Serial.print(data);
  }
}

void debugPrintln(String data) {
  if (ENABLE_DEBUG_PRINT == 1) {
   Serial.println(data);
  }
}

String getFreeMemory() {
  long  fh = ESP.getFreeHeap();
  String freeHeap;
  char  fhc[20];
  ltoa(fh, fhc, 10);
  freeHeap = String(fhc);  
  return freeHeap;
}

void reconnect() {
  // Loop until we're reconnected
  if (!client.connected()) {
    digitalWrite(BUILTIN_LED, HIGH);    
    debugPrint("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(),mqtt_user,mqtt_password)) {
      debugPrintln("connected");
      // Once connected, publish an announcement...
      client.publish(mqtt_topic_out.c_str(), "hello world");
      // ... and resubscribe
      //client.subscribe(mqtt_topic_in);
      digitalWrite(BUILTIN_LED, LOW);    
    } else {
      debugPrint("failed, rc=");
      //debugPrint(client.state());
      debugPrintln(" try again in 5 seconds");
      // Wait 5 seconds before retrying
    }
  }
}

//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
  debugPrintln("Entered config mode ");
  //debugPrintln(WiFi.softAPIP());
  debugPrintln(myWiFiManager->getConfigPortalSSID());
  //ticker.attach(0.2, tick);
}


void handleRoot() {

  String freeHeap;
  freeHeap = getFreeMemory();  
  
  String message = "<html><head><title>PZEM Reader</title></head><body><h1>PZEM Reader</h1>\n";
  message += "<p>Status: <span id='latestData'>";
  message += latestSerialData;
  message += " / Memory: ";
  message += freeHeap;  
  message += "</span>\n";
  message += "<h2>Settings</h2><form action='/save' method='get'>MQTT Server:<br/><input type='text' name='server' value='";
  message += mqtt_server;
  message += "' size='60'><br/>";
  message += "Topic:<br/><input type='text' name='base' value='";
  message += mqtt_topic_out;    
  message += "'size='60'><br/><input type='submit' value='Save settings'></form>";
  message += "</p>";  
  message += "<script language='javascript' src='https://ajax.googleapis.com/ajax/libs/jquery/2.2.4/jquery.min.js'></script>";  
  message += "<script language=\"javascript\">\n";
  message += "var status_timer;\n";
  message += "function updateStatus() {\n";
  message += "$.ajax({  url: \"/data\"  }).done(function(data) {  $('#latestData').html(data);status_timer=setTimeout('updateStatus();', 500); });";
  message += "}\n";
  message += "$(document).ready(function() {  updateStatus();   });\n";      
  message += "</script>";  
  message += "<p>Livolo Key example: <a href='/rfkey?button=1&remote=8500' target=_blank>/rfkey?button=1&remote=8500</a> (<i>button</i> -- button number (1-10), <i>remote</i> -- remote ID)</p>";  
  message += "<p><a href='/restart' onclick='return confirm(\"Are you sure? Please confirm\");'>Reboot</a></p>";  
  message += "</body></html>"; 
  server.sendHeader("Connection", "close");
     
  server.send(200, "text/html", message);

  server.sendHeader("Connection", "close");
     
  server.send(200, "text/html", message);
}



void handleData() {

  String freeHeap;
  freeHeap = getFreeMemory();  

  String message = "";

  if (client.connected()) {
    message += "<font color='green'>MQTT Connected</font> Data: ";
  } else {
   message += "<font color='red'>MQTT Disconnected</font> Data: ";
  }
  message += latestSerialData;
  message += "; Memory: ";
  message += freeHeap;  
  server.sendHeader("Connection", "close");
  server.send(200, "text/plain", message);
}

void handleKey() {
    const String& strButton = server.arg("button");
    const String& strRemote = server.arg("remote");

  String message = "";

    if (strButton.equals("off")) {
      latestKey = 106;
    } else {
     int keyIndex=strButton.toInt();
     if (keyIndex<1) keyIndex=1;
     latestKey = LIVOLO_KEYS[keyIndex-1];
    }
    latestRemoteId = strRemote.toInt();
    if (latestRemoteId==0) {
      latestRemoteId = 6400;
    }
    message += "Sending key code ";
    message += latestKey;
    message += " by remote ";
    message += latestRemoteId;
    gLivolo.sendButton(latestRemoteId, latestKey);
    message += " ... DONE";
    
  server.sendHeader("Connection", "close");
  server.send(200, "text/plain", message);
}

void handleUpdateConfig() {
  debugPrintln("Web: config update.");  
  for (uint8_t i=0; i<server.args(); i++){
    if (server.argName(i)=="server") {
      mqtt_server=server.arg(i);
    }
    if (server.argName(i)=="base") {
      mqtt_topic_out=server.arg(i);
    }
  }
  String message = "<html><body><script language='javascript'>tm=setTimeout(\"window.location.href='/';\",2000);</script>Data saved!</body></html>";  

  writeStringEEPROM(mqtt_server,EEPROM_MQTT_SERVER);
  writeStringEEPROM(mqtt_topic_out, EEPROM_MQTT_TOPIC);
  
  server.sendHeader("Connection", "close");  
  server.send(200, "text/html", message);
}

void handleRestart() {
  String message = "<html><body><script language='javascript'>tm=setTimeout(\"window.location.href='/';\",2000);</script>Rebooting...</body></html>";  
  server.sendHeader("Connection", "close");  
  server.send(200, "text/html", message);  
  ESP.restart();
}

void handleNotFound(){
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.sendHeader("Connection", "close");  
  server.send(404, "text/plain", message);
}

String readStringEEPROM(int startIdx) {

  debugPrint("Reading string from ");
  debugPrint(String(startIdx));

  
    String res="";
    for (int i = 0; i < EEPROM_STRING_MAX; ++i)
    {
      byte sm=EEPROM.read(startIdx+i);
      if (sm>0) {
       res+= char(sm);        
      } else {
        break;
      }
    }
    debugPrint(" string: ");
    debugPrintln(res);
    return res;
}

void writeStringEEPROM(String str, int startIdx) {
  debugPrint("Writing string to ");
  debugPrint(String(startIdx));
  debugPrint(" string: ");
  debugPrintln(str);
          for (int i = 0; i < EEPROM_STRING_MAX; ++i)
          {
            EEPROM.write(startIdx+i, 0);
          }    
          for (int i = 0; i < str.length(); ++i)
            {
              EEPROM.write(startIdx+i, str[i]);
            }    
          EEPROM.commit();
          delay(500);  
}

//This function will write a 2 byte integer to the eeprom at the specified address and address + 1
void writeIntEEPROM(unsigned int p_value, int p_address) {
      byte lowByte = ((p_value >> 0) & 0xFF);
      byte highByte = ((p_value >> 8) & 0xFF);

      EEPROM.write(p_address, lowByte);
      EEPROM.write(p_address + 1, highByte);
      }

//This function will read a 2 byte integer from the eeprom at the specified address and address + 1
unsigned int readIntEEPROM(int p_address) {
      byte lowByte = EEPROM.read(p_address);
      byte highByte = EEPROM.read(p_address + 1);

      return ((lowByte << 0) & 0xFF) + ((highByte << 8) & 0xFF00);
      }



void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  digitalWrite(BUILTIN_LED, HIGH);

  if (ENABLE_DEBUG_PRINT == 1) {
   Serial.begin(9600);
  }

    pinMode(TX_PIN, OUTPUT);
    digitalWrite(TX_PIN, LOW);
 
    WiFiManager wifiManager;
    wifiManager.setAPCallback(configModeCallback);
    String AcceePointName = "LivoloControl_"+String(random(0xffff), HEX);
    
    if (!wifiManager.autoConnect(AcceePointName.c_str(),"1111")) {
      debugPrintln("Failed to connect!");
      ESP.reset();
      delay(1000);
    }
    digitalWrite(BUILTIN_LED, LOW);

    EEPROM.begin(512);
    mqtt_server=readStringEEPROM(EEPROM_MQTT_SERVER);    
    mqtt_topic_out=readStringEEPROM(EEPROM_MQTT_TOPIC);    
  
  client.setServer(mqtt_server.c_str(), 1883);
  client.setCallback(callback);

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/rfkey", handleKey);
  server.on("/restart", handleRestart);
  server.on("/save", handleUpdateConfig);
  server.onNotFound(handleNotFound);  
  server.begin();

  ArduinoOTA.setHostname(AcceePointName.c_str());

  ArduinoOTA.onStart([]() {
    Serial.println("Start OTA");
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("End OTA");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\n", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();  
  
}


void sendNewData() {
  String destTopic;
  destTopic = mqtt_topic_out;
  debugPrintln(destTopic);  
  debugPrintln(latestSerialData);  
  client.publish(destTopic.c_str(), latestSerialData.c_str());
}


void loop() {

  long now = millis()/1000;

  ArduinoOTA.handle();
  server.handleClient();

  if (!client.connected()) {
    if (now-lastConnectTry>=5) {
     reconnect();      
     lastConnectTry = now;      
    }
  } else {
   client.loop();    
  }

  if (now > 24*60*60) {
    // auto reboot every 24 hours
    ESP.restart();
  }
 
  if (now - lastMsg > 60) {

  latestSerialData="{\"uptime\":\""+String(now)+"\"";
  latestSerialData+=",\"ip\":\""+WiFi.localIP().toString()+"\"";
  latestSerialData+=",\"latestRemoteId\":\""+String(latestRemoteId)+"\"";
  latestSerialData+=",\"latestKey\":\""+String(latestKey)+"\"";
  
  latestSerialData+="}";
  
    delay(1000);
    lastMsg = now;    
    sendNewData();
    debugPrintln(" SENT");    
  }
}


