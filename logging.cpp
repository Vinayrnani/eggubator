#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <string.h>
#include "logging.h"
#include "config.h"

extern uint32_t bootId;

LogEntry logBuffer[MAX_LOG_ENTRIES];
int logIndex = 0;
bool logFull = false;
unsigned long lastLogTime = 0;
unsigned long lastSaveFlashTime = 0;

int currentSector = 0;
int currentBootFlashSectorCount = 0;

#define EEPROM_SECTOR_ADDR 20
#define EEPROM_SECTOR_MAGIC_ADDR 21
#define EEPROM_SECTOR_MAGIC 0x5A
#define FLASH_BASE_ADDR 0x100000
#define VALID_STATE_MASK (STATE_HEATER | STATE_ATOMIZER | STATE_FAN | STATE_SERVO)

static bool isLogEntryValid(const LogEntry& entry, uint32_t previousTimestamp) {
  if (entry.timestamp == 0 || entry.timestamp == 0xFFFFFFFFUL) return false;
  if (previousTimestamp > 0 && entry.timestamp < previousTimestamp) return false;
  if ((entry.states & ~VALID_STATE_MASK) != 0) return false;
  if (entry.temp > 800) return false;
  if (entry.hum > 1000) return false;
  return true;
}

static void appendLogEntryJson(const LogEntry& entry, String& json, bool& first) {
  float tempVal = entry.temp / 10.0;
  float humVal = entry.hum / 10.0;
  bool h = entry.states & STATE_HEATER;
  bool a = entry.states & STATE_ATOMIZER;
  bool f = entry.states & STATE_FAN;
  bool s = entry.states & STATE_SERVO;

  if (!first) json += ",";
  json += "{\"t\":" + String(entry.timestamp) +
         ",\"temp\":" + String(tempVal, 1) +
         ",\"hum\":" + String(humVal, 1) +
         ",\"h\":" + String(h ? "true" : "false") +
         ",\"a\":" + String(a ? "true" : "false") +
         ",\"f\":" + String(f ? "true" : "false") +
         ",\"s\":" + String(s ? "true" : "false") + "}";
  first = false;
}

int getFlashLogStartSector() {
  if (currentBootFlashSectorCount <= 0) {
    return -1;
  }

  int startSector = currentSector - currentBootFlashSectorCount;
  if (startSector < 0) {
    startSector += LOG_SECTOR_COUNT;
  }
  return startSector;
}

int getFlashLogSectorCount() {
  return currentBootFlashSectorCount;
}

bool hasFlashLogSector(int sector) {
  if (sector < 0 || sector >= LOG_SECTOR_COUNT || currentBootFlashSectorCount <= 0) {
    return false;
  }

  int startSector = getFlashLogStartSector();
  for (int i = 0; i < currentBootFlashSectorCount; i++) {
    if (((startSector + i) % LOG_SECTOR_COUNT) == sector) {
      return true;
    }
  }

  return false;
}

void initLogging() {
  logIndex = 0;
  logFull = false;
  lastLogTime = millis();
  lastSaveFlashTime = millis();
  currentBootFlashSectorCount = 0;

  uint8_t storedSector = EEPROM.read(EEPROM_SECTOR_ADDR);
  uint8_t sectorMagic = EEPROM.read(EEPROM_SECTOR_MAGIC_ADDR);
  currentSector = (sectorMagic == EEPROM_SECTOR_MAGIC) ? storedSector : 0;
  if (sectorMagic != EEPROM_SECTOR_MAGIC) {
    EEPROM.write(EEPROM_SECTOR_ADDR, currentSector);
    EEPROM.write(EEPROM_SECTOR_MAGIC_ADDR, EEPROM_SECTOR_MAGIC);
    EEPROM.commit();
  }

  Serial.print("Logging initialized - Next sector: ");
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

  json += ",\"log\":[";
  bool first = true;
  uint32_t previousTimestamp = 0;
  for (int i = 0; i < count; i++) {
    int idx = (startIdx + i) % MAX_LOG_ENTRIES;
    if (!isLogEntryValid(logBuffer[idx], previousTimestamp)) {
      continue;
    }
    appendLogEntryJson(logBuffer[idx], json, first);
    previousTimestamp = logBuffer[idx].timestamp;
  }
  json += "]";
}

void getFlashLogDataForWeb(int sector, String& json) {
  if (!hasFlashLogSector(sector)) {
    json += "[]";
    return;
  }

  uint32_t flashAddr = FLASH_BASE_ADDR + (sector * SECTOR_SIZE);
  uint32_t headerWords[(sizeof(FlashLogSectorHeader) + sizeof(uint32_t) - 1) / sizeof(uint32_t)] = {0};
  FlashLogSectorHeader header;

  if (!ESP.flashRead(flashAddr, headerWords, sizeof(headerWords))) {
    json += "[]";
    return;
  }
  memcpy(&header, headerWords, sizeof(header));

  if (header.magic != FLASH_LOG_MAGIC ||
      header.version != FLASH_LOG_VERSION ||
      header.bootId != bootId ||
      header.entrySize != sizeof(LogEntry) ||
      header.entryCount == 0 ||
      header.entryCount > FLASH_LOG_ENTRIES_PER_SECTOR) {
    json += "[]";
    return;
  }

  size_t entryBytes = header.entryCount * sizeof(LogEntry);
  size_t readBytes = (entryBytes + sizeof(uint32_t) - 1) & ~(sizeof(uint32_t) - 1);
  uint32_t* rawBuf = new uint32_t[readBytes / sizeof(uint32_t)];
  if (!rawBuf) {
    json += "[]";
    return;
  }

  if (!ESP.flashRead(flashAddr + sizeof(FlashLogSectorHeader), rawBuf, readBytes)) {
    delete[] rawBuf;
    json += "[]";
    return;
  }

  LogEntry* tempBuf = new LogEntry[header.entryCount];
  if (!tempBuf) {
    delete[] rawBuf;
    json += "[]";
    return;
  }
  memcpy(tempBuf, rawBuf, entryBytes);
  delete[] rawBuf;

  json += "[";
  bool first = true;
  uint32_t previousTimestamp = 0;
  for (uint32_t i = 0; i < header.entryCount; i++) {
    if (!isLogEntryValid(tempBuf[i], previousTimestamp)) {
      break;
    }
    appendLogEntryJson(tempBuf[i], json, first);
    previousTimestamp = tempBuf[i].timestamp;
  }
  json += "]";

  delete[] tempBuf;
}

bool shouldSaveToFlash() {
  unsigned long elapsed = millis() - lastSaveFlashTime;
  bool nearFull = logFull || (logIndex >= SECTOR_THRESHOLD);
  bool timeDue = (elapsed >= SAVE_FLASH_INTERVAL);
  bool hasData = (logIndex > 0 || logFull);
  return (nearFull || timeDue) && hasData;
}

void saveLogsToFlash() {
  if (!logFull && logIndex == 0) return;

  int entriesToSave = logFull ? MAX_LOG_ENTRIES : logIndex;
  if (entriesToSave > FLASH_LOG_ENTRIES_PER_SECTOR) {
    entriesToSave = FLASH_LOG_ENTRIES_PER_SECTOR;
  }

  uint32_t* sectorWords = new uint32_t[SECTOR_SIZE / sizeof(uint32_t)];
  if (!sectorWords) {
    Serial.println("Flash save skipped: allocation failed");
    return;
  }

  memset(sectorWords, 0xFF, SECTOR_SIZE);

  FlashLogSectorHeader header;
  header.magic = FLASH_LOG_MAGIC;
  header.version = FLASH_LOG_VERSION;
  header.bootId = bootId;
  header.entryCount = entriesToSave;
  header.entrySize = sizeof(LogEntry);
  memcpy(sectorWords, &header, sizeof(header));

  LogEntry* flashEntries = reinterpret_cast<LogEntry*>(reinterpret_cast<uint8_t*>(sectorWords) + sizeof(FlashLogSectorHeader));
  int startIdx = logFull ? logIndex : 0;
  for (int i = 0; i < entriesToSave; i++) {
    int idx = (startIdx + i) % MAX_LOG_ENTRIES;
    flashEntries[i] = logBuffer[idx];
  }

  uint32_t flashAddr = FLASH_BASE_ADDR + (currentSector * SECTOR_SIZE);
  int writtenSector = currentSector;

  bool erased = ESP.flashEraseSector(flashAddr / SECTOR_SIZE);
  bool written = erased && ESP.flashWrite(flashAddr, sectorWords, SECTOR_SIZE);
  delete[] sectorWords;

  if (!written) {
    Serial.println("Flash save failed");
    return;
  }

  currentSector++;
  if (currentSector >= LOG_SECTOR_COUNT) {
    currentSector = 0;
  }
  EEPROM.write(EEPROM_SECTOR_ADDR, currentSector);
  EEPROM.write(EEPROM_SECTOR_MAGIC_ADDR, EEPROM_SECTOR_MAGIC);
  EEPROM.commit();

  if (currentBootFlashSectorCount < LOG_SECTOR_COUNT) {
    currentBootFlashSectorCount++;
  }

  logIndex = 0;
  logFull = false;
  lastSaveFlashTime = millis();

  Serial.print("Saved ");
  Serial.print(entriesToSave);
  Serial.print(" logs to sector ");
  Serial.println(writtenSector);
}

void clearLogs() {
  logIndex = 0;
  logFull = false;
  currentSector = 0;
  currentBootFlashSectorCount = 0;
  EEPROM.write(EEPROM_SECTOR_ADDR, currentSector);
  EEPROM.write(EEPROM_SECTOR_MAGIC_ADDR, EEPROM_SECTOR_MAGIC);
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
