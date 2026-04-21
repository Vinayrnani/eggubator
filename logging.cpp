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

#define FLASH_BASE_ADDR 0x100000

namespace {
void appendLogEntryJson(const LogEntry& entry, String& json) {
  float tempVal = entry.temp / 10.0f;
  float humVal = entry.hum / 10.0f;
  bool heaterOn = (entry.states & STATE_HEATER) != 0;
  bool atomizerOn = (entry.states & STATE_ATOMIZER) != 0;
  bool fanOn = (entry.states & STATE_FAN) != 0;

  json += "{\"t\":" + String(entry.timestamp) +
          ",\"temp\":" + String(tempVal, 1) +
          ",\"hum\":" + String(humVal, 1) +
          ",\"h\":" + String(heaterOn ? "true" : "false") +
          ",\"a\":" + String(atomizerOn ? "true" : "false") +
          ",\"f\":" + String(fanOn ? "true" : "false") +
          ",\"s\":" + String((int)entry.servoPos) + "}";
}

void copyLogsChronologically(LogEntry* destination, int entriesToCopy) {
  int totalEntries = getLogEntryCount();
  if (entriesToCopy > totalEntries) {
    entriesToCopy = totalEntries;
  }

  int startIndex = logFull ? logIndex : 0;
  if (totalEntries > entriesToCopy) {
    startIndex = (startIndex + (totalEntries - entriesToCopy)) % MAX_LOG_ENTRIES;
  }

  for (int i = 0; i < entriesToCopy; i++) {
    int bufferIndex = (startIndex + i) % MAX_LOG_ENTRIES;
    destination[i] = logBuffer[bufferIndex];
  }
}
}

void initLogging() {
  logIndex = 0;
  logFull = false;
  lastLogTime = millis();
  lastSaveFlashTime = millis();

  uint8_t storedSector = EEPROM.read(EEPROM_SECTOR_ADDR);
  currentSector = (storedSector == 0xFF || storedSector >= LOG_SECTOR_COUNT) ? 0 : storedSector;

  Serial.print("Logging initialized - next sector: ");
  Serial.print(currentSector);
  Serial.print(" (capacity ");
  Serial.print(LOG_ENTRIES_PER_SECTOR);
  Serial.println(" entries per sector)");
}

void logData(float temp, float hum, bool heater, bool atomizer, bool fan, int servoPos) {
  uint8_t states = 0;
  if (heater) states |= STATE_HEATER;
  if (atomizer) states |= STATE_ATOMIZER;
  if (fan) states |= STATE_FAN;
  if (servoPos != 0) states |= STATE_SERVO;

  LogEntry& entry = logBuffer[logIndex];
  entry.timestamp = millis();
  entry.temp = (uint16_t)(temp * 10 + 0.5f);
  entry.hum = (uint16_t)(hum * 10 + 0.5f);
  entry.states = states;
  entry.servoPos = (int8_t)servoPos;
  entry.reserved[0] = 0;
  entry.reserved[1] = 0;

  logIndex++;
  if (logIndex >= MAX_LOG_ENTRIES) {
    logIndex = 0;
    logFull = true;
  }
}

int getLogEntryCount() {
  return logFull ? MAX_LOG_ENTRIES : logIndex;
}

size_t getLogStorageBytes() {
  return (size_t)getLogEntryCount() * sizeof(LogEntry);
}

void getLogDataForWeb(String& json) {
  int count = getLogEntryCount();
  int startIndex = logFull ? logIndex : 0;

  json += ",\"log\":[";
  for (int i = 0; i < count; i++) {
    if (i > 0) {
      json += ",";
    }
    int bufferIndex = (startIndex + i) % MAX_LOG_ENTRIES;
    appendLogEntryJson(logBuffer[bufferIndex], json);
  }
  json += "]";
}

void getFlashLogDataForWeb(int sector, String& json) {
  if (sector < 0 || sector >= LOG_SECTOR_COUNT) {
    json += "[]";
    return;
  }

  uint32_t flashAddr = FLASH_BASE_ADDR + (sector * SECTOR_SIZE);
  int entries = LOG_ENTRIES_PER_SECTOR;
  LogEntry* tempBuffer = new LogEntry[entries];
  if (!tempBuffer) {
    json += "[]";
    return;
  }

  ESP.flashRead(flashAddr, (uint32_t*)tempBuffer, entries * sizeof(LogEntry));

  json += "[";
  bool first = true;
  for (int i = 0; i < entries; i++) {
    if (tempBuffer[i].timestamp == 0xFFFFFFFF) {
      break;
    }
    if (!first) {
      json += ",";
    }
    appendLogEntryJson(tempBuffer[i], json);
    first = false;
  }
  json += "]";

  delete[] tempBuffer;
}

bool shouldSaveToFlash() {
  unsigned long elapsed = millis() - lastSaveFlashTime;
  bool nearFull = getLogEntryCount() >= SECTOR_THRESHOLD;
  bool timeDue = elapsed >= SAVE_FLASH_INTERVAL;
  bool hasData = getLogEntryCount() > 0;
  return (nearFull || timeDue) && hasData;
}

void saveLogsToFlash() {
  int entriesAvailable = getLogEntryCount();
  if (entriesAvailable == 0) {
    return;
  }

  int entriesToSave = entriesAvailable;
  if (entriesToSave > LOG_ENTRIES_PER_SECTOR) {
    entriesToSave = LOG_ENTRIES_PER_SECTOR;
  }

  LogEntry* orderedLogs = new LogEntry[entriesToSave];
  if (!orderedLogs) {
    Serial.println("Unable to allocate log flash buffer");
    return;
  }

  copyLogsChronologically(orderedLogs, entriesToSave);

  uint32_t flashAddr = FLASH_BASE_ADDR + (currentSector * SECTOR_SIZE);
  int savedSector = currentSector;

  if (!ESP.flashEraseSector(flashAddr / SECTOR_SIZE)) {
    Serial.println("Flash erase failed");
    delete[] orderedLogs;
    return;
  }

  if (!ESP.flashWrite(flashAddr, (uint32_t*)orderedLogs, entriesToSave * sizeof(LogEntry))) {
    Serial.println("Flash write failed");
    delete[] orderedLogs;
    return;
  }

  delete[] orderedLogs;

  currentSector = (currentSector + 1) % LOG_SECTOR_COUNT;
  EEPROM.write(EEPROM_SECTOR_ADDR, currentSector);
  EEPROM.commit();

  logIndex = 0;
  logFull = false;
  lastSaveFlashTime = millis();

  Serial.print("Saved ");
  Serial.print(entriesToSave);
  Serial.print(" logs to sector ");
  Serial.println(savedSector);
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

  if (EEPROM.read(EEPROM_SETTINGS_MAGIC) != 0xA5) {
    LOG_INTERVAL = DEFAULT_LOG_INTERVAL;
    SAVE_FLASH_INTERVAL = DEFAULT_SAVE_FLASH_INTERVAL;
    EGG_TURN_INTERVAL = DEFAULT_EGG_TURN_INTERVAL;
    PULSE_ON_TIME = DEFAULT_PULSE_ON_TIME;
    applyStageSettings(false);

    byteWrite(EEPROM_LOG_INTERVAL, LOG_INTERVAL);
    byteWrite(EEPROM_SAVE_FLASH, SAVE_FLASH_INTERVAL);
    byteWrite(EEPROM_EGG_TURN, EGG_TURN_INTERVAL);
    byteWrite(EEPROM_PULSE_ON, PULSE_ON_TIME);
    EEPROM.write(EEPROM_STAGE, 0);
    EEPROM.write(EEPROM_SETTINGS_MAGIC, 0xA5);
    EEPROM.commit();
    return;
  }

  LOG_INTERVAL = byteRead(EEPROM_LOG_INTERVAL);
  SAVE_FLASH_INTERVAL = byteRead(EEPROM_SAVE_FLASH);
  EGG_TURN_INTERVAL = byteRead(EEPROM_EGG_TURN);
  PULSE_ON_TIME = byteRead(EEPROM_PULSE_ON);

  if (LOG_INTERVAL == 0) LOG_INTERVAL = DEFAULT_LOG_INTERVAL;
  if (SAVE_FLASH_INTERVAL == 0) SAVE_FLASH_INTERVAL = DEFAULT_SAVE_FLASH_INTERVAL;
  if (EGG_TURN_INTERVAL == 0) EGG_TURN_INTERVAL = DEFAULT_EGG_TURN_INTERVAL;
  if (PULSE_ON_TIME == 0) PULSE_ON_TIME = DEFAULT_PULSE_ON_TIME;

  bool savedLockdown = EEPROM.read(EEPROM_STAGE) == 1;
  applyStageSettings(savedLockdown);
}

void saveSettings() {
  extern unsigned long LOG_INTERVAL;
  extern unsigned long SAVE_FLASH_INTERVAL;
  extern unsigned long EGG_TURN_INTERVAL;
  extern unsigned long PULSE_ON_TIME;
  extern bool stageLockdown;

  byteWrite(EEPROM_LOG_INTERVAL, LOG_INTERVAL);
  byteWrite(EEPROM_SAVE_FLASH, SAVE_FLASH_INTERVAL);
  byteWrite(EEPROM_EGG_TURN, EGG_TURN_INTERVAL);
  byteWrite(EEPROM_PULSE_ON, PULSE_ON_TIME);
  EEPROM.write(EEPROM_STAGE, stageLockdown ? 1 : 0);
  EEPROM.write(EEPROM_SETTINGS_MAGIC, 0xA5);
  EEPROM.commit();
}

uint32_t byteRead(int addr) {
  uint32_t value = 0;
  for (int i = 0; i < 4; i++) {
    value |= ((uint32_t)EEPROM.read(addr + i) << ((3 - i) * 8));
  }
  return value;
}

void byteWrite(int addr, uint32_t value) {
  for (int i = 0; i < 4; i++) {
    EEPROM.write(addr + i, (value >> ((3 - i) * 8)) & 0xFF);
  }
}
