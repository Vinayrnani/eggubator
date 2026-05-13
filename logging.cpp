#include "logging.h"
#include <EEPROM.h>

uint8_t currentBootId = 0;
uint16_t currentSector = 0;
uint16_t currentOffset = 0;
uint16_t startSector = 0;
uint32_t logsInCurrentBoot = 0;
uint32_t totalLogsCached = 0;
unsigned long lastLogTime = 0;

BootIndexEntry* bootIndex = nullptr;
int bootIndexCount = 0;
int bootIndexCapacity = 0;

float lastLoggedTemp = -100.0;
float lastLoggedHum = -100.0;
uint8_t lastLoggedStates = 0xFF;

void initLogging(uint8_t bootId) {
  currentBootId = bootId;
  logsInCurrentBoot = 0;
  
  EEPROM.get(EEPROM_CURRENT_SECTOR, currentSector);
  EEPROM.get(EEPROM_START_SECTOR, startSector);

  if (currentSector >= FLASH_NUM_SECTORS || startSector >= FLASH_NUM_SECTORS) {
    currentSector = 0;
    startSector = 0;
    EEPROM.put(EEPROM_CURRENT_SECTOR, currentSector);
    EEPROM.put(EEPROM_START_SECTOR, startSector);
    EEPROM.commit();
    ESP.flashEraseSector((FLASH_LOG_START + (currentSector * FLASH_SECTOR_SIZE)) / FLASH_SECTOR_SIZE);
  }

  // Scan current sector for the first blank slot
  uint32_t sectorAddr = FLASH_LOG_START + (currentSector * FLASH_SECTOR_SIZE);
  currentOffset = 0;
  LogEntry entry;
  for (int i = 0; i < LOGS_PER_SECTOR; i++) {
    ESP.flashRead(sectorAddr + (i * sizeof(LogEntry)), (uint32_t*)&entry, sizeof(LogEntry));
    if (entry.timeSec == 0xFFFFFFFF) {
      currentOffset = i;
      break;
    }
    if (i == LOGS_PER_SECTOR - 1) {
      // Sector is perfectly full, we should move to next
      currentSector = (currentSector + 1) % FLASH_NUM_SECTORS;
      if (currentSector == startSector) {
        startSector = (startSector + 1) % FLASH_NUM_SECTORS;
        EEPROM.put(EEPROM_START_SECTOR, startSector);
      }
      EEPROM.put(EEPROM_CURRENT_SECTOR, currentSector);
      EEPROM.commit();
      ESP.flashEraseSector((FLASH_LOG_START + (currentSector * FLASH_SECTOR_SIZE)) / FLASH_SECTOR_SIZE);
      currentOffset = 0;
    }
  }

  lastLogTime = 0;
  lastLoggedTemp = -100.0;
  lastLoggedHum = -100.0;
  lastLoggedStates = 0xFF;

  // Build boot index dynamically
  bootIndexCount = 0;
  bootIndexCapacity = 16;
  if (bootIndex != nullptr) free(bootIndex);
  bootIndex = (BootIndexEntry*)malloc(bootIndexCapacity * sizeof(BootIndexEntry));
  uint8_t lastBootId = 0xFF;

  // Scan forward from oldest to current position, record bootId changes and count logs
  int s = startSector;
  int o = 0;
  totalLogsCached = 0;
  while (!(s == currentSector && o == currentOffset)) {
    ESP.wdtFeed();
    uint32_t addr = FLASH_LOG_START + (s * FLASH_SECTOR_SIZE) + (o * sizeof(LogEntry));
    LogEntry entry;
    ESP.flashRead(addr, (uint32_t*)&entry, sizeof(LogEntry));

    if (entry.timeSec == 0xFFFFFFFF) break;

    totalLogsCached++;

    if (bootIndexCount == 0 || entry.bootId != lastBootId) {
      if (bootIndexCount >= bootIndexCapacity) {
        bootIndexCapacity *= 2;
        bootIndex = (BootIndexEntry*)realloc(bootIndex, bootIndexCapacity * sizeof(BootIndexEntry));
      }
      bootIndex[bootIndexCount].bootId = entry.bootId;
      bootIndex[bootIndexCount].sector = s;
      bootIndex[bootIndexCount].offset = o;
      bootIndexCount++;
      lastBootId = entry.bootId;
    }

    o++;
    if (o >= LOGS_PER_SECTOR) {
      o = 0;
      s = (s + 1) % FLASH_NUM_SECTORS;
    }
  }
}

bool logData(float temp, float hum, bool heater, bool atomizer, bool fan, int servo, unsigned long forceInterval) {
  uint8_t t_encoded = (uint8_t)((temp - 20.0) * 10.0 + 0.5);
  uint8_t h_encoded = (uint8_t)(hum + 0.5);

  uint8_t turner_val = 0;
  if (servo == 1) turner_val = 1;
  else if (servo == 2) turner_val = 2;
  else turner_val = 0;

  uint8_t states = 0;
  if (heater) states |= STATE_HEATER;
  if (atomizer) states |= STATE_ATOMIZER;
  if (fan) states |= STATE_FAN;
  states = SET_TURNER(states, turner_val);

  bool significant = false;
  if (states != lastLoggedStates) significant = true;
  else if (lastLogTime == 0 || (millis() - lastLogTime >= forceInterval)) significant = true;

  if (!significant) return false;

  LogEntry entry;
  entry.timeSec = millis() / 1000;
  entry.temp = t_encoded;
  entry.hum = h_encoded;
  entry.states = states;
  entry.bootId = currentBootId;

  // Add to boot index if this is a new bootId
  if (bootIndexCount == 0 || currentBootId != bootIndex[bootIndexCount-1].bootId) {
    if (bootIndexCount >= bootIndexCapacity) {
      bootIndexCapacity = bootIndexCapacity == 0 ? 16 : bootIndexCapacity * 2;
      bootIndex = (BootIndexEntry*)realloc(bootIndex, bootIndexCapacity * sizeof(BootIndexEntry));
    }
    bootIndex[bootIndexCount].bootId = currentBootId;
    bootIndex[bootIndexCount].sector = currentSector;
    bootIndex[bootIndexCount].offset = currentOffset;
    bootIndexCount++;
  }

  uint32_t writeAddr = FLASH_LOG_START + (currentSector * FLASH_SECTOR_SIZE) + (currentOffset * sizeof(LogEntry));
  ESP.flashWrite(writeAddr, (uint32_t*)&entry, sizeof(LogEntry));

  lastLoggedTemp = temp;
  lastLoggedHum = hum;
  lastLoggedStates = states;
  lastLogTime = millis();

  currentOffset++;
  logsInCurrentBoot++;
  totalLogsCached++;

  if (currentOffset >= LOGS_PER_SECTOR) {
    currentSector = (currentSector + 1) % FLASH_NUM_SECTORS;
    if (currentSector == startSector) {
      startSector = (startSector + 1) % FLASH_NUM_SECTORS;
      EEPROM.put(EEPROM_START_SECTOR, startSector);
    }
    EEPROM.put(EEPROM_CURRENT_SECTOR, currentSector);
    EEPROM.commit();
    ESP.flashEraseSector((FLASH_LOG_START + (currentSector * FLASH_SECTOR_SIZE)) / FLASH_SECTOR_SIZE);
    currentOffset = 0;
  }

  return true;
}

int getLogHex(String& hex, int maxEntries, uint8_t sinceBootId, uint32_t sinceTimeSec) {
  hex.reserve(maxEntries * 16);
  int sent = 0;

  // Find starting position using boot index
  int targetSector = -1;
  int targetOffset = -1;

  if (sinceBootId == 0 && sinceTimeSec == 0) {
    // Start from oldest
    if (bootIndexCount > 0) {
      targetSector = bootIndex[0].sector;
      targetOffset = bootIndex[0].offset;
    }
  } else {
    // Find position AFTER (sinceBootId, sinceTimeSec)
    int bootStartS = -1, bootStartO = -1;
    for (int i = bootIndexCount - 1; i >= 0; i--) {
      if (bootIndex[i].bootId == sinceBootId) {
        bootStartS = bootIndex[i].sector;
        bootStartO = bootIndex[i].offset;
        break;
      }
    }

    if (bootStartS == -1) {
      // BootId not in index - fall back to scanning forward from startSector
      int scanS = startSector;
      int scanO = 0;
      while (!(scanS == currentSector && scanO == currentOffset)) {
        ESP.wdtFeed();
        uint32_t addr = FLASH_LOG_START + (scanS * FLASH_SECTOR_SIZE) + (scanO * sizeof(LogEntry));
        LogEntry entry;
        ESP.flashRead(addr, (uint32_t*)&entry, sizeof(LogEntry));
        if (entry.timeSec == 0xFFFFFFFF) break;
        if (entry.bootId == sinceBootId && entry.timeSec == sinceTimeSec) {
          // Found it, next position
          scanO++;
          if (scanO >= LOGS_PER_SECTOR) { scanO = 0; scanS = (scanS + 1) % FLASH_NUM_SECTORS; }
          targetSector = scanS;
          targetOffset = scanO;
          break;
        }
        scanO++;
        if (scanO >= LOGS_PER_SECTOR) {
          scanS = (scanS + 1) % FLASH_NUM_SECTORS;
          scanO = 0;
        }
      }
    } else {
      // Scan forward from boot start to find exact match
      int s = bootStartS;
      int o = bootStartO;
      while (!(s == currentSector && o == currentOffset)) {
        ESP.wdtFeed();
        uint32_t addr = FLASH_LOG_START + (s * FLASH_SECTOR_SIZE) + (o * sizeof(LogEntry));
        LogEntry entry;
        ESP.flashRead(addr, (uint32_t*)&entry, sizeof(LogEntry));
        if (entry.timeSec == 0xFFFFFFFF) break;
        if (entry.bootId == sinceBootId && entry.timeSec == sinceTimeSec) {
          // Found it! Start from next position
          o++;
          if (o >= LOGS_PER_SECTOR) { o = 0; s = (s + 1) % FLASH_NUM_SECTORS; }
          targetSector = s;
          targetOffset = o;
          break;
        }
        o++;
        if (o >= LOGS_PER_SECTOR) { o = 0; s = (s + 1) % FLASH_NUM_SECTORS; }
      }
    }
  }

  // If target not found, return 0 (prevent infinite loop)
  if (targetSector == -1) {
    return 0;
  }

  // Now scan forward from target to collect maxEntries (or up to current position)
  int currentS = targetSector;
  int currentO = targetOffset;
  
  while (sent < maxEntries && !(currentS == currentSector && currentO == currentOffset)) {
    if (sent % 50 == 0) ESP.wdtFeed();

    uint32_t addr = FLASH_LOG_START + (currentS * FLASH_SECTOR_SIZE) + (currentO * sizeof(LogEntry));
    LogEntry entry;
    ESP.flashRead(addr, (uint32_t*)&entry, sizeof(LogEntry));
    
    if (entry.timeSec != 0xFFFFFFFF) {
      uint8_t* ptr = (uint8_t*)&entry;
      for (int j = 0; j < 8; j++) {
        if (ptr[j] < 16) hex += "0";
        hex += String(ptr[j], HEX);
      }
      sent++;
    } else {
      // Reached empty flash, means buffer wasn't wrapped and we hit the end
      break;
    }

    currentO++;
    if (currentO >= LOGS_PER_SECTOR) {
      currentO = 0;
      currentS = (currentS + 1) % FLASH_NUM_SECTORS;
    }
  }

  return sent;
}

void clearLogs() {
  startSector = (currentSector + 1) % FLASH_NUM_SECTORS;
  currentSector = startSector;
  currentOffset = 0;
  EEPROM.put(EEPROM_START_SECTOR, startSector);
  EEPROM.put(EEPROM_CURRENT_SECTOR, currentSector);
  EEPROM.commit();
  ESP.flashEraseSector((FLASH_LOG_START + (currentSector * FLASH_SECTOR_SIZE)) / FLASH_SECTOR_SIZE);
  lastLoggedTemp = -100.0;
  lastLoggedHum = -100.0;
  lastLoggedStates = 0xFF;
  totalLogsCached = 0;
  bootIndexCount = 0;
}

int getTotalLogs() {
  return totalLogsCached;
}
