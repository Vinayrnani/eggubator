#ifndef UPDATES_H
#define UPDATES_H

#include <ESP8266HTTPClient.h>
#include <ESP8266HTTPUpdateServer.h>
#include <WiFiClient.h>
#include <EEPROM.h>

#define FIRMWARE_URL "http://YOUR_SERVER/firmware.bin"
#define VERSION_URL "http://YOUR_SERVER/version.txt"
#define FIRMWARE_VERSION "1.3.33"

extern ESP8266HTTPUpdateServer httpUpdater;

bool checkForUpdate() {
  HTTPClient http;
  WiFiClient client;
  http.begin(client, VERSION_URL);
  int httpCode = http.GET();
  
  if (httpCode == 200) {
    String remoteVersion = http.getString();
    remoteVersion.trim();
    bool hasUpdate = (remoteVersion != FIRMWARE_VERSION);
    http.end();
    return hasUpdate;
  }
  http.end();
  return false;
}

void performUpdate() {
  HTTPClient http;
  WiFiClient client;
  http.begin(client, FIRMWARE_URL);
  int httpCode = http.GET();
  
  if (httpCode == 200) {
    WiFiClient* stream = http.getStreamPtr();
    size_t size = http.getSize();
    if (Update.begin(size)) {
      if (Update.writeStream(*stream) && Update.end(true)) {
        delay(1000);
        ESP.restart();
      }
    }
  }
  http.end();
}

void checkAndUpdateAuto() {
  static bool updateInProgress = false;
  if (updateInProgress) return;
  
  if (WiFi.status() != WL_CONNECTED) return;
  
  HTTPClient http;
  WiFiClient client;
  http.begin(client, VERSION_URL);
  int httpCode = http.GET();
  
  if (httpCode == 200) {
    String remoteVersion = http.getString();
    remoteVersion.trim();
    
    if (remoteVersion != FIRMWARE_VERSION) {
      updateInProgress = true;
      http.end();
      
      http.begin(client, FIRMWARE_URL);
      httpCode = http.GET();
      
      if (httpCode == 200) {
        WiFiClient* stream = http.getStreamPtr();
        size_t size = http.getSize();
        if (Update.begin(size) && Update.writeStream(*stream) && Update.end(true)) {
          delay(1000);
          ESP.restart();
        }
      }
      http.end();
      updateInProgress = false;
    }
  }
  http.end();
}

#endif
