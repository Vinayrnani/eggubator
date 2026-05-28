#ifndef UPDATES_H
#define UPDATES_H

#include <ESP8266HTTPClient.h>
#include <ESP8266HTTPUpdateServer.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <EEPROM.h>

#define FIRMWARE_URL "https://github.com/Vinayrnani/eggubator/releases/latest/download/firmware.bin"
#define VERSION_URL "https://api.github.com/repos/Vinayrnani/eggubator/releases/latest"
#define FIRMWARE_VERSION "1.3.36"

extern ESP8266HTTPUpdateServer httpUpdater;

bool checkForUpdate() {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  
  if (http.begin(client, VERSION_URL)) {
    int httpCode = http.GET();
    
    if (httpCode == 200) {
      String payload = http.getString();
      http.end();
      
      // Parse JSON to extract tag_name
      int tagStart = payload.indexOf("\"tag_name\":\"");
      if (tagStart != -1) {
        tagStart += 12; // Length of "\"tag_name\":\""
        int tagEnd = payload.indexOf('\"', tagStart);
        if (tagEnd != -1) {
          String tagName = payload.substring(tagStart, tagEnd);
          // Compare tagName (e.g., "v1.3.36") with FIRMWARE_VERSION (e.g., "1.3.35")
          // Remove leading 'v' if present for comparison
          String versionToCompare = tagName;
          if (versionToCompare.startsWith("v")) {
            versionToCompare = versionToCompare.substring(1);
          }
          bool hasUpdate = (versionToCompare != FIRMWARE_VERSION);
          return hasUpdate;
        }
      }
    }
    http.end();
  }
  return false;
}

void performUpdate() {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  
  if (http.begin(client, FIRMWARE_URL)) {
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
}

#endif // UPDATES_H