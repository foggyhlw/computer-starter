/*
  SimpleMQTTClient.ino
  The purpose of this exemple is to illustrate a simple handling of MQTT and Wifi connection.
  Once it connects successfully to a Wifi network and a MQTT broker, it subscribe to a topic and send a message to it.
  It will also send a message delayed 5 seconds later.
*/

#include "EspMQTTClient.h"
#include <WiFiUdp.h>
#include <WiFiManager.h>
#include <WakeOnLan.h>
#include <ESP8266HTTPClient.h>

String computer_ip = "http://192.168.0.137:5001/";
String secret_code = "1234";
String base_url = computer_ip+secret_code+"/";

#define start_state_topic "/foggy/computer_parent/start/state"
#define start_cmd_topic "/foggy/computer_parent/start/cmd"
#define shutdown_state_topic "/foggy/computer_parent/shutdown/state"
#define shutdown_cmd_topic "/foggy/computer_parent/shutdown/cmd"
#define hiberate_state_topic "/foggy/computer_parent/hibernate/state"
#define hibernate_cmd_topic "/foggy/computer_parent/hibernate/cmd"
#define relay_cmd_topic "/foggy/printer_parent/cmd"
#define relay_state_topic "/foggy/printer_parent/state"

#define MACAddress "AC:FD:5D:A0:B1:4C"

const char* HOSTNAME="REMOTE_STARTER";
const char* ssid     = "username";
const char* password = "passwd";

#define relay_pin 0
WiFiUDP UDP;
WakeOnLan WOL(UDP);

EspMQTTClient client(
  "mqtt-broker-address",  // MQTT Broker server ip
  1883,              // The MQTT port, default to 1883. this line can be omitted
  "username",   // Can be omitted if not needed
  "password",   // Can be omitted if not needed
  "remote_starter"     // Client name that uniquely identify your device
);

void startwifimanager() {
  WiFiManager wifiManager;
  if (!wifiManager.autoConnect(HOSTNAME, NULL)) {
      ESP.restart();
      delay(1000);
   }
}

void wakeMyPC() {
    // const char *MACAddress = "00:24:8c:24:73:93";
    Serial.print("Send magic packet");
    WOL.sendMagicPacket(MACAddress); // Send Wake On Lan packet with the above MAC address. Default to port 9.
    // WOL.sendMagicPacket(MACAddress, 7); // Change the port number
}

void setup()
{
  Serial.begin(115200);
  startwifimanager();
  WOL.setRepeat(3, 100); // Optional, repeat the packet three times with 100ms between. WARNING delay() is used between send packet function.
  pinMode(relay_pin, OUTPUT);
  // Optionnal functionnalities of EspMQTTClient : 
  client.enableDebuggingMessages(); // Enable debugging messages sent to serial output
  client.enableHTTPWebUpdater(); // Enable the web updater. User and password default to values of MQTTUsername and MQTTPassword. These can be overrited with enableHTTPWebUpdater("user", "password").
  client.enableLastWillMessage("TestClient/lastwill", "I am going offline");  // You can activate the retain flag by setting the third parameter to true
}

void send_http_command(String data){
  HTTPClient http;
      
      // Your Domain name with URL path or IP address with path
      http.begin(data.c_str());
      
      // Send HTTP GET request
      int httpResponseCode = http.GET();
      
      if (httpResponseCode>0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        String payload = http.getString();
        Serial.println(payload);
      }
      else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
      }
      // Free resources
      http.end();
}
// This function is called once everything is connected (Wifi and MQTT)
// WARNING : YOU MUST IMPLEMENT IT IF YOU USE EspMQTTClient
void onConnectionEstablished()
{
  // make sure device won't misoperate when startup
  client.publish(start_state_topic,"0");
  client.publish(start_cmd_topic,"0");
  client.publish(shutdown_state_topic,"0");
  client.publish(shutdown_cmd_topic,"0");      
  client.publish(hiberate_state_topic,"0");
  client.publish(hibernate_cmd_topic,"0");

  // Subscribe to "mytopic/test" and display received message to Serial
  client.subscribe("start_cmd_topic", [](const String & payload) {
    Serial.println(payload);
    if( payload == "1"){
      wakeMyPC();
      client.publish(start_state_topic,"0");
      client.publish(start_cmd_topic,"0");
    }
  });

  client.subscribe(shutdown_cmd_topic, [](const String & payload) {
    Serial.println(payload);
    if( payload == "1"){
      String shutdown_url = base_url+"shutdown";
      send_http_command(shutdown_url);
      client.publish(shutdown_state_topic,"0");
      client.publish(shutdown_cmd_topic,"0");
    }
  });
  
    client.subscribe(hibernate_cmd_topic, [](const String & payload) {
    Serial.println(payload);
    if( payload == "1"){
      String hibernate_url = base_url+"hibernate";
      send_http_command(hibernate_url);
      client.publish(hiberate_state_topic,"0");
      client.publish(hibernate_cmd_topic,"0");
    }
  });

  // Subscribe to "mytopic/wildcardtest/#" and display received message to Serial
  client.subscribe(relay_cmd_topic, [](const String & payload) {
    // Serial.println("(From wildcard) topic: " + topic + ", payload: " + payload);
    if( payload == "0" ){
      digitalWrite(relay_pin,HIGH);
      client.publish(relay_state_topic, "0");
    }
    if( payload == "1" ){
      digitalWrite(relay_pin,LOW);
      client.publish(relay_state_topic, "1");
    }
  });

  // Publish a message to "mytopic/test"
  client.publish("mytopic/test", "This is a message"); // You can activate the retain flag by setting the third parameter to true

  // Execute delayed instructions
  // client.executeDelayed(5 * 1000, []() {
  //   client.publish("mytopic/wildcardtest/test123", "This is a message sent 5 seconds later");
  // });
}

void loop()
{
  client.loop();
}
