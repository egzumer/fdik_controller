#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <ESPUI.h>

#include "heater.h"
#include "ui.h"

const char* ssid     = WIFI_SSID;                                
const char* password = WIFI_PASSWORD;

WiFiServer server(8088);


Heater heater(16);


void setup() {
  Serial.begin(115200);
  heater.begin();

  pinMode(2, OUTPUT);

  Serial.println("\n\n\n");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(2,1);
    delay(10);
    digitalWrite(2,0);
    delay(1000);
    Serial.print(".");
    
  }

  Serial.println();
  Serial.println("WiFi connected with IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();


  UI::init();



  Serial.println("\nsetup finished");
}

void loop() {
  ArduinoOTA.handle();
  heater.loop();

  if(!digitalRead(0)) {
    delay(50);
    if(digitalRead(0)) return;    
    while(!digitalRead(0));


    const uint32_t del = 20;

    static bool enabled;
    if(!enabled) {
      enabled = true;
      
      heater.turnOn(Heater::EMode::Fan);
    }
    else {
      enabled = false;
      heater.turnOff();
    }
  }

}
