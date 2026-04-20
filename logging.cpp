#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "logging.h"
#include "config.h"

LogEntry logBuffer[MAX_LOG_ENTRIES];
int logIndex = 0;
bool logFull = false;
unsigned long lastLogTime = 0;
unsigned long lastSaveFlashTime = 0;

static int currentSector = 0;
static bool sectorsInitialized[LOG_SECTOR_COUNT] = {false};

#define FLASH_BASE_ADDR 0x100000

void initLogging() {
  logIndex = 0;
  logFull = false;
  lastLogTime = millis();
  lastSaveFlashTime = millis();
  currentSector = 0;
  Serial.println("Logging initialized - Flash storage ready");
}

void logData(float temp, float hum, bool heater, bool atomizer, bool fan, int servo) {
  uint8_t states = 0;
  if (heater) states |= STATE_HEATER;
  if (atomizer) states |= STATE_ATOMIZER;
  if (fan) states |= STATE_FAN;
  if (servo > 0) states |= STATE_SERVO;

  logBuffer[logIndex].timestamp = millis();
  logBuffer[logIndex].temp = (uint16_t)(temp * 10 + 0.5);
  logBuffer[logIndex].hum = (uint16_t)(hum * 10 + 0.5);
  logBuffer[logIndex].states = states;
  logBuffer[logIndex].servoPos = (uint8_t)servo;
  
  logIndex++;
  if (logIndex >= MAX_LOG_ENTRIES) {
    logIndex = 0;
    logFull = true;
  }
}

void getLogDataForWeb(String& json) {
  int startIdx = logFull ? logIndex : 0;
  int count = logFull ? MAX_LOG_ENTRIES : logIndex;
  
  if (count > 0) {
    json += ",\"log\":[";
    for (int i = 0; i < count; i++) {
      int idx = (startIdx + i) % MAX_LOG_ENTRIES;
      float tempVal = logBuffer[idx].temp / 10.0;
      float humVal = logBuffer[idx].hum / 10.0;
      bool h = logBuffer[idx].states & STATE_HEATER;
      bool a = logBuffer[idx].states & STATE_ATOMIZER;
      bool f = logBuffer[idx].states & STATE_FAN;
      bool s = logBuffer[idx].states & STATE_SERVO;
      
      json += "{\"t\":" + String(logBuffer[idx].timestamp) +
             ",\"temp\":" + String(tempVal, 1) +
             ",\"hum\":" + String(humVal, 1) +
             ",\"h\":" + String(h ? "true" : "false") +
             ",\"a\":" + String(a ? "true" : "false") +
             ",\"f\":" + String(f ? "true" : "false") +
             ",\"s\":" + String(s ? "true" : "false") + "}";
      if (i < count - 1) json += ",";
    }
    json += "]";
  } else {
    json += ",\"log\":[]";
  }
}

bool shouldSaveToFlash() {
  unsigned long elapsed = millis() - lastSaveFlashTime;
  bool ramFull = (logFull || logIndex >= MAX_LOG_ENTRIES);
  bool timeDue = (elapsed >= SAVE_FLASH_INTERVAL);
  return ramFull || timeDue;
}

void saveLogsToFlash() {
  if (!logFull && logIndex == 0) return;
  
  int entriesToSave = logFull ? MAX_LOG_ENTRIES : logIndex;
  size_t sizeNeeded = entriesToSave * sizeof(LogEntry);
  
  if (sizeNeeded >= SECTOR_SIZE) {
    sizeNeeded = SECTOR_SIZE;
    entriesToSave = SECTOR_SIZE / sizeof(LogEntry);
  }
  
  uint32_t flashAddr = FLASH_BASE_ADDR + (currentSector * SECTOR_SIZE);
  
  ESP.flashEraseSector(flashAddr / SECTOR_SIZE);
  
  ESP.flashWrite(flashAddr, (uint32_t*)logBuffer, sizeNeeded);
  
  sectorsInitialized[currentSector] = true;
  
  currentSector++;
  if (currentSector >= LOG_SECTOR_COUNT) {
    currentSector = 0;
  }
  
  logIndex = 0;
  logFull = false;
  lastSaveFlashTime = millis();
  
  Serial.print("Saved ");
  Serial.print(entriesToSave);
  Serial.print(" logs to sector ");
  Serial.println(currentSector);
}

void getLogsFromFlash(String& json) {
  json += ",\"logs\":[";
  bool firstEntry = true;
  
  for (int sec = 0; sec < LOG_SECTOR_COUNT; sec++) {
    if (!sectorsInitialized[sec]) continue;
    
    uint32_t flashAddr = FLASH_BASE_ADDR + (sec * SECTOR_SIZE);
    uint8_t buffer[SECTOR_SIZE];
    
    ESP.flashRead(flashAddr, (uint32_t*)buffer, SECTOR_SIZE);
    
    int entriesInSector = SECTOR_SIZE / sizeof(LogEntry);
    LogEntry* entries = (LogEntry*)buffer;
    
    for (int i = 0; i < entriesInSector; i++) {
      if (entries[i].timestamp == 0) continue;
      
      float tempVal = entries[i].temp / 10.0;
      float humVal = entries[i].hum / 10.0;
      bool h = entries[i].states & STATE_HEATER;
      bool a = entries[i].states & STATE_ATOMIZER;
      bool f = entries[i].states & STATE_FAN;
      bool s = entries[i].states & STATE_SERVO;
      
      if (!firstEntry) json += ",";
      json += "{\"t\":" + String(entries[i].timestamp) +
             ",\"temp\":" + String(tempVal, 1) +
             ",\"hum\":" + String(humVal, 1) +
             ",\"h\":" + String(h ? "true" : "false") +
             ",\"a\":" + String(a ? "true" : "false") +
             ",\"f\":" + String(f ? "true" : "false") +
             ",\"s\":" + String(s ? "true" : "false") + "}";
      firstEntry = false;
    }
  }
  
  json += "],\"sectors\":" + String(currentSector);
}

void clearLogs() {
  logIndex = 0;
  logFull = false;
  currentSector = 0;
  for (int i = 0; i < LOG_SECTOR_COUNT; i++) {
    sectorsInitialized[i] = false;
  }
  Serial.println("Logs cleared");
}