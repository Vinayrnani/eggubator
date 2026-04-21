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

static int currentSector = 0;

#define EEPROM_SECTOR_ADDR 20
#define FLASH_BASE_ADDR 0x100000
#define SECTOR_THRESHOLD 390

void initLogging() {
  logIndex = 0;
  logFull = false;
  lastLogTime = millis();
  lastSaveFlashTime = millis();
  
  currentSector = EEPROM.read(EEPROM_SECTOR_ADDR);
  currentSector++;
  if (currentSector >= LOG_SECTOR_COUNT) {
    currentSector = 0;
  }
  EEPROM.write(EEPROM_SECTOR_ADDR, currentSector);
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
  if (currentSector >= LOG_SECTOR_COUNT) {
    currentSector = 0;
  }
  EEPROM.write(EEPROM_SECTOR_ADDR, currentSector);
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
  EEPROM.write(EEPROM_SECTOR_ADDR, currentSector);
  EEPROM.commit();
  Serial.println("Logs cleared");
}

void loadSettings() {
  extern unsigned long LOG_INTERVAL;
  extern unsigned long SAVE_FLASH_INTERVAL;
  extern unsigned long EGG_TURN_INTERVAL;
  extern unsigned long PULSE_ON_TIME;
  extern bool stageLockdown;
  uint32_t val = 0;
  
  EEPROM.get(EEPROM_LOG_INTERVAL, val);
  LOG_INTERVAL = (val == 0xFFFFFFFF || val == 0) ? 10000 : val;
  
  val = 0;
  EEPROM.get(EEPROM_SAVE_FLASH, val);
  SAVE_FLASH_INTERVAL = (val == 0xFFFFFFFF || val == 0) ? 7200000 : val;
  
  val = 0;
  EEPROM.get(EEPROM_EGG_TURN, val);
  EGG_TURN_INTERVAL = (val == 0xFFFFFFFF || val == 0) ? 7200000 : val;
  
  val = 0;
  EEPROM.get(EEPROM_PULSE_ON, val);
  PULSE_ON_TIME = (val == 0xFFFFFFFF || val == 0) ? 2000 : val;
  
  uint8_t stage = EEPROM.read(EEPROM_STAGE);
  stageLockdown = (stage == 1);
  
  Serial.print("Settings loaded - Log:");
  Serial.print(LOG_INTERVAL/1000);
  Serial.print("s Flash:");
  Serial.print(SAVE_FLASH_INTERVAL/60000);
  Serial.print("m Turn:");
  Serial.print(EGG_TURN_INTERVAL/3600000);
  Serial.print("h Pulse:");
  Serial.print(PULSE_ON_TIME/1000);
  Serial.print("s Stage:");
  Serial.println(stageLockdown ? "LOCKDOWN" : "INCUBATION");
}

void saveSettings() {
  extern unsigned long LOG_INTERVAL;
  extern unsigned long SAVE_FLASH_INTERVAL;
  extern unsigned long EGG_TURN_INTERVAL;
  extern unsigned long PULSE_ON_TIME;
  extern bool stageLockdown;
  bool changed = false;
  uint32_t oldVal = 0;
  uint8_t oldStage;
  
  EEPROM.get(EEPROM_LOG_INTERVAL, oldVal);
  if (LOG_INTERVAL != oldVal) { EEPROM.put(EEPROM_LOG_INTERVAL, LOG_INTERVAL); changed = true; }
  
  oldVal = 0;
  EEPROM.get(EEPROM_SAVE_FLASH, oldVal);
  if (SAVE_FLASH_INTERVAL != oldVal) { EEPROM.put(EEPROM_SAVE_FLASH, SAVE_FLASH_INTERVAL); changed = true; }
  
  oldVal = 0;
  EEPROM.get(EEPROM_EGG_TURN, oldVal);
  if (EGG_TURN_INTERVAL != oldVal) { EEPROM.put(EEPROM_EGG_TURN, EGG_TURN_INTERVAL); changed = true; }
  
  oldVal = 0;
  EEPROM.get(EEPROM_PULSE_ON, oldVal);
  if (PULSE_ON_TIME != oldVal) { EEPROM.put(EEPROM_PULSE_ON, PULSE_ON_TIME); changed = true; }
  
  oldStage = EEPROM.read(EEPROM_STAGE);
  uint8_t newStage = stageLockdown ? 1 : 0;
  if (oldStage != newStage) { EEPROM.write(EEPROM_STAGE, newStage); changed = true; }
  
  if (changed) {
    EEPROM.commit();
    Serial.println("Settings saved");
  }
}