#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include "logging.h"
#include "config.h"

LogEntry logBuffer[MAX_LOG_ENTRIES];
int logIndex = 0;
bool logFull = false;
unsigned long lastLogTime = 0;
unsigned long lastSaveFlashTime = 0;

int currentSector = 0;
int sectorsUsed = 0;

#define EEPROM_SECTOR_ADDR 20
#define EEPROM_SECTORS_USED 21
#define FLASH_BASE_ADDR 0x100000

void initLogging() {
  logIndex = 0;
  logFull = false;
  lastLogTime = millis();
  lastSaveFlashTime = millis();
  
  currentSector = EEPROM.read(EEPROM_SECTOR_ADDR);
  sectorsUsed = EEPROM.read(EEPROM_SECTORS_USED);
  currentSector++;
  if (currentSector >= LOG_SECTOR_COUNT) {
    currentSector = 0;
    sectorsUsed = 0;
  }
  EEPROM.write(EEPROM_SECTOR_ADDR, currentSector);
  EEPROM.write(EEPROM_SECTORS_USED, sectorsUsed);
  EEPROM.commit();
  
  Serial.print("Logging initialized - Sector: ");
  Serial.println(currentSector);
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

void getFlashLogDataForWeb(int sector, String& json) {
  if (sector < 0 || sector >= LOG_SECTOR_COUNT) {
    json += "[]";
    return;
  }
  
  uint32_t flashAddr = FLASH_BASE_ADDR + (sector * SECTOR_SIZE);
  int entries = SECTOR_SIZE / sizeof(LogEntry);
  
  LogEntry* tempBuf = new LogEntry[entries];
  if (!tempBuf) {
    json += "[]";
    return;
  }
  
  ESP.flashRead(flashAddr, (uint32_t*)tempBuf, entries * sizeof(LogEntry));
  
  json += "[";
  bool first = true;
  for (int i = 0; i < entries; i++) {
    if (tempBuf[i].timestamp == 0xFFFFFFFF) break; // unwritten flash
    
    float tempVal = tempBuf[i].temp / 10.0;
    float humVal = tempBuf[i].hum / 10.0;
    bool h = tempBuf[i].states & STATE_HEATER;
    bool a = tempBuf[i].states & STATE_ATOMIZER;
    bool f = tempBuf[i].states & STATE_FAN;
    bool s = tempBuf[i].states & STATE_SERVO;
    
    if (!first) json += ",";
    json += "{\"t\":" + String(tempBuf[i].timestamp) +
           ",\"temp\":" + String(tempVal, 1) +
           ",\"hum\":" + String(humVal, 1) +
           ",\"h\":" + String(h ? "true" : "false") +
           ",\"a\":" + String(a ? "true" : "false") +
           ",\"f\":" + String(f ? "true" : "false") +
           ",\"s\":" + String(s ? "true" : "false") + "}";
    first = false;
  }
  json += "]";
  
  delete[] tempBuf;
}

void getFlashLogsSince(int sinceTimestamp, int limit, String& recordsJson, int& lastTs, bool& hasMore) {
  recordsJson = "[";
  bool first = true;
  int count = 0;
  lastTs = sinceTimestamp;
  
  for (int sec = 0; sec < LOG_SECTOR_COUNT; sec++) {
    uint32_t flashAddr = FLASH_BASE_ADDR + (sec * SECTOR_SIZE);
    LogEntry tempBuf[100];
    
    ESP.flashRead(flashAddr, (uint32_t*)tempBuf, 100 * sizeof(LogEntry));
    
    for (int i = 0; i < 100; i++) {
      if (tempBuf[i].timestamp == 0xFFFFFFFF) break;
      if ((int)tempBuf[i].timestamp <= sinceTimestamp) continue;
      
      float tempVal = tempBuf[i].temp / 10.0;
      float humVal = tempBuf[i].hum / 10.0;
      bool h = tempBuf[i].states & STATE_HEATER;
      bool a = tempBuf[i].states & STATE_ATOMIZER;
      bool f = tempBuf[i].states & STATE_FAN;
      bool s = tempBuf[i].states & STATE_SERVO;
      
      if (!first) recordsJson += ",";
      recordsJson += "{\"t\":" + String(tempBuf[i].timestamp) +
             ",\"temp\":" + String(tempVal, 1) +
             ",\"hum\":" + String(humVal, 1) +
             ",\"h\":" + String(h ? "true" : "false") +
             ",\"a\":" + String(a ? "true" : "false") +
             ",\"f\":" + String(f ? "true" : "false") +
             ",\"s\":" + String(s ? "true" : "false") + "}";
      first = false;
      
      lastTs = tempBuf[i].timestamp;
      count++;
      
      if (count >= limit) {
        hasMore = true;
        recordsJson += "]";
        return;
      }
    }
  }
  
  hasMore = false;
  recordsJson += "]";
}

bool shouldSaveToFlash() {
  unsigned long elapsed = millis() - lastSaveFlashTime;
  bool nearFull = (logIndex >= SECTOR_THRESHOLD);
  bool timeDue = (elapsed >= SAVE_FLASH_INTERVAL);
  bool hasData = (logIndex > 0 || logFull);
  return (nearFull || timeDue) && hasData;
}

void saveLogsToFlash() {
  if (!logFull && logIndex == 0) return;
  
  int entriesToSave = logFull ? MAX_LOG_ENTRIES : logIndex;
  size_t sizeNeeded = entriesToSave * sizeof(LogEntry);
  
  if (sizeNeeded > SECTOR_SIZE) {
    sizeNeeded = SECTOR_SIZE;
    entriesToSave = SECTOR_SIZE / sizeof(LogEntry);
  }
  
  uint32_t flashAddr = FLASH_BASE_ADDR + (currentSector * SECTOR_SIZE);
  
  ESP.flashEraseSector(flashAddr / SECTOR_SIZE);
  ESP.flashWrite(flashAddr, (uint32_t*)logBuffer, sizeNeeded);
  
  currentSector++;
  sectorsUsed++;
  if (currentSector >= LOG_SECTOR_COUNT) {
    currentSector = 0;
    sectorsUsed = 0;
  }
  EEPROM.write(EEPROM_SECTOR_ADDR, currentSector);
  EEPROM.write(EEPROM_SECTORS_USED, sectorsUsed);
  EEPROM.commit();
  
  logIndex = 0;
  logFull = false;
  lastSaveFlashTime = millis();
  
  Serial.print("Saved ");
  Serial.print(entriesToSave);
  Serial.print(" logs to sector ");
  Serial.println(currentSector);
}

void clearLogs() {
  logIndex = 0;
  logFull = false;
  currentSector = 0;
  sectorsUsed = 0;
  EEPROM.write(EEPROM_SECTOR_ADDR, currentSector);
  EEPROM.write(EEPROM_SECTORS_USED, sectorsUsed);
  EEPROM.commit();
  Serial.println("Logs cleared");
}

void loadSettings() {
  extern unsigned long LOG_INTERVAL;
  extern unsigned long SAVE_FLASH_INTERVAL;
  extern unsigned long EGG_TURN_INTERVAL;
  extern unsigned long PULSE_ON_TIME;
  extern bool stageLockdown;
  
  if (EEPROM.read(30) != 0xA5) {
    LOG_INTERVAL = 10000;
    SAVE_FLASH_INTERVAL = 7200000;
    EGG_TURN_INTERVAL = 7200000;
    PULSE_ON_TIME = 2000;
    stageLockdown = false;
    byteWrite(31, LOG_INTERVAL);
    byteWrite(35, SAVE_FLASH_INTERVAL);
    byteWrite(39, EGG_TURN_INTERVAL);
    byteWrite(43, PULSE_ON_TIME);
    EEPROM.write(47, 0);
    EEPROM.write(30, 0xA5);
    EEPROM.commit();
    return;
  }
  
  LOG_INTERVAL = byteRead(31);
  SAVE_FLASH_INTERVAL = byteRead(35);
  EGG_TURN_INTERVAL = byteRead(39);
  PULSE_ON_TIME = byteRead(43);
  stageLockdown = (EEPROM.read(47) == 1);
}

void saveSettings() {
  extern unsigned long LOG_INTERVAL;
  extern unsigned long SAVE_FLASH_INTERVAL;
  extern unsigned long EGG_TURN_INTERVAL;
  extern unsigned long PULSE_ON_TIME;
  extern bool stageLockdown;
  
  byteWrite(31, LOG_INTERVAL);
  byteWrite(35, SAVE_FLASH_INTERVAL);
  byteWrite(39, EGG_TURN_INTERVAL);
  byteWrite(43, PULSE_ON_TIME);
  EEPROM.write(47, stageLockdown ? 1 : 0);
  EEPROM.commit();
}

uint32_t byteRead(int addr) {
  uint32_t val = 0;
  for (int i = 0; i < 4; i++) {
    val |= ((uint32_t)EEPROM.read(addr + i) << ((3 - i) * 8));
  }
  return val;
}

void byteWrite(int addr, uint32_t val) {
  for (int i = 0; i < 4; i++) {
    EEPROM.write(addr + i, (val >> ((3 - i) * 8)) & 0xFF);
  }
}

