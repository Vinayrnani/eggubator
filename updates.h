#ifndef UPDATES_H
#define UPDATES_H

#include <ESP8266HTTPClient.h>
#include <ESP8266HTTPUpdateServer.h>
#include <WiFiClient.h>
#include <EEPROM.h>

#define FIRMWARE_URL "http://YOUR_SERVER/firmware.bin"
#define VERSION_URL "http://YOUR_SERVER/version.txt"
#define FIRMWARE_VERSION "1.2.5"

extern ESP8266HTTPUpdateServer httpUpdater;

// EEPROM addresses for boot recovery
#define EEPROM_BOOT_OK 10
#define EEPROM_BOOT_COUNT 11
#define BOOT_OK_MAGIC 0x55
#define MAX_BOOT_FAILURES 3

extern ESP8266WebServer* pServer;

void initRecovery() {
  EEPROM.begin(512);
  uint8_t bootOk = EEPROM.read(EEPROM_BOOT_OK);
  uint8_t bootCount = EEPROM.read(EEPROM_BOOT_COUNT);
  
  Serial.print("Boot status: ");
  Serial.print(bootOk, HEX);
  Serial.print(" Count: ");
  Serial.println(bootCount);
  
  // If previously failed, increment failure count
  if (bootOk != BOOT_OK_MAGIC) {
    bootCount++;
    EEPROM.write(EEPROM_BOOT_COUNT, bootCount);
    EEPROM.commit();
    Serial.print("Boot failure #");
    Serial.println(bootCount);
    
    // If too many failures, enter recovery mode
    if (bootCount >= MAX_BOOT_FAILURES) {
      Serial.println("ENTERING RECOVERY MODE - Rolling back!");
      // Reset boot count after entering recovery
      EEPROM.write(EEPROM_BOOT_COUNT, 0);
      EEPROM.commit();
    }
  } else {
    // Successful boot - reset count
    bootCount = 0;
    EEPROM.write(EEPROM_BOOT_COUNT, 0);
    EEPROM.commit();
  }
  
  // Mark that we're starting boot
  EEPROM.write(EEPROM_BOOT_OK, 0);
  EEPROM.commit();
}

void markBootSuccess() {
  EEPROM.write(EEPROM_BOOT_OK, BOOT_OK_MAGIC);
  EEPROM.commit();
  Serial.println("Boot marked as OK");
}

void setupOTA(ESP8266WebServer& server) {
  httpUpdater.setup(&server);
  Serial.println("OTA update enabled");
}

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
        Serial.println("Update complete, rebooting...");
        delay(1000);
        ESP.restart();
      }
    }
  }
  http.end();
  Serial.println("Update failed");
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
      Serial.print("New firmware available: ");
      Serial.println(remoteVersion);
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
