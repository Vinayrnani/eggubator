#ifndef UPDATES_H
#define UPDATES_H

#include <ESP8266HTTPClient.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266httpUpdate.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <EEPROM.h>

#define FIRMWARE_URL "https://github.com/Vinayrnani/eggubator/releases/latest/download/firmware.bin"
#define VERSION_URL "https://api.github.com/repos/Vinayrnani/eggubator/releases/latest"
#define FIRMWARE_VERSION "1.5.5"

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
          bool hasUpdate = false;
          int vMaj = versionToCompare.substring(0, versionToCompare.indexOf('.')).toInt();
          int vMin = versionToCompare.substring(versionToCompare.indexOf('.')+1, versionToCompare.lastIndexOf('.')).toInt();
          int vPat = versionToCompare.substring(versionToCompare.lastIndexOf('.')+1).toInt();
          int cMaj = String(FIRMWARE_VERSION).substring(0, String(FIRMWARE_VERSION).indexOf('.')).toInt();
          int cMin = String(FIRMWARE_VERSION).substring(String(FIRMWARE_VERSION).indexOf('.')+1, String(FIRMWARE_VERSION).lastIndexOf('.')).toInt();
          int cPat = String(FIRMWARE_VERSION).substring(String(FIRMWARE_VERSION).lastIndexOf('.')+1).toInt();
          if (vMaj > cMaj) hasUpdate = true;
          else if (vMaj == cMaj && vMin > cMin) hasUpdate = true;
          else if (vMaj == cMaj && vMin == cMin && vPat > cPat) hasUpdate = true;
          return hasUpdate;
        }
      }
    }
    http.end();
  }
  return false;
}

bool performUpdate() {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  http.setTimeout(30000);
  
  if (!http.begin(client, FIRMWARE_URL)) {
    return false;
  }
  
  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    http.end();
    return false;
  }
  
  size_t size = http.getSize();
  if (size == 0) {
    http.end();
    return false;
  }
  
  if (!Update.begin(size, U_FLASH)) {
    http.end();
    return false;
  }
  
  WiFiClient* stream = http.getStreamPtr();
  size_t written = Update.writeStream(*stream);
  
  if (written != size || !Update.end(true)) {
    Update.end(false);
    http.end();
    return false;
  }
  
  http.end();
  delay(1000);
  ESP.restart();
  return true;
}

#endif // UPDATES_H