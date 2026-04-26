#define FLASH_SECTOR_SIZE FLASH_SECTOR_SIZE_VAL
#include "logging.h"
#include <EEPROM.h>

uint8_t currentBootId = 0;
uint16_t currentSector = 0;
uint16_t currentOffset = 0;
uint32_t logsInCurrentBoot = 0;

unsigned long lastLogTime = 0;
float lastLoggedTemp = -100.0;
float lastLoggedHum = -100.0;
uint8_t lastLoggedStates = 0xFF;

void initLogging(uint8_t bootId) {
  currentBootId = bootId;
  logsInCurrentBoot = 0;
  
  EEPROM.get(EEPROM_CURRENT_SECTOR, currentSector);
  if (currentSector >= FLASH_NUM_SECTORS) {
    currentSector = 0;
    EEPROM.put(EEPROM_CURRENT_SECTOR, currentSector);
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
}

bool logData(float temp, float hum, bool heater, bool atomizer, bool fan, int servo, unsigned long forceInterval) {
  uint8_t t_encoded = (uint8_t)((temp - 20.0) * 10.0 + 0.5);
  uint8_t h_encoded = (uint8_t)(hum + 0.5);

  uint8_t turner_val = 0;
  if (servo == -1) turner_val = 2;
  else if (servo == 0) turner_val = 0;
  else if (servo == 1) turner_val = 1;

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

  uint32_t writeAddr = FLASH_LOG_START + (currentSector * FLASH_SECTOR_SIZE) + (currentOffset * sizeof(LogEntry));
  ESP.flashWrite(writeAddr, (uint32_t*)&entry, sizeof(LogEntry));

  lastLoggedTemp = temp;
  lastLoggedHum = hum;
  lastLoggedStates = states;
  lastLogTime = millis();

  currentOffset++;
  logsInCurrentBoot++;

  if (currentOffset >= LOGS_PER_SECTOR) {
    currentSector = (currentSector + 1) % FLASH_NUM_SECTORS;
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

  int scanSector = currentSector;
  int scanOffset = currentOffset - 1;

  if (scanOffset < 0) {
    scanSector = (scanSector - 1 + FLASH_NUM_SECTORS) % FLASH_NUM_SECTORS;
    scanOffset = LOGS_PER_SECTOR - 1;
  }

  int targetSector = -1;
  int targetOffset = -1;
  int logsScanned = 0;

  // Maximum backward scan to limit HTTP blocking (e.g., 30000 logs ~30 days)
  int MAX_SCAN = 30000;
  
  while (logsScanned < MAX_LOG_ENTRIES && logsScanned < MAX_SCAN) {
    if (logsScanned % 1000 == 0) ESP.wdtFeed(); // feed watchdog

    uint32_t addr = FLASH_LOG_START + (scanSector * FLASH_SECTOR_SIZE) + (scanOffset * sizeof(LogEntry));
    LogEntry entry;
    ESP.flashRead(addr, (uint32_t*)&entry, sizeof(LogEntry));

    // Stop scanning if we hit erased flash (the tail of the circular buffer)
    if (entry.timeSec == 0xFFFFFFFF) break;

    // Check if we hit the exact log requested
    if (sinceBootId != 0 || sinceTimeSec != 0) {
      if (entry.bootId == sinceBootId && entry.timeSec == sinceTimeSec) {
        targetOffset = scanOffset + 1;
        targetSector = scanSector;
        if (targetOffset >= LOGS_PER_SECTOR) {
          targetOffset = 0;
          targetSector = (scanSector + 1) % FLASH_NUM_SECTORS;
        }
        break;
      }
    }

    scanOffset--;
    if (scanOffset < 0) {
      scanSector = (scanSector - 1 + FLASH_NUM_SECTORS) % FLASH_NUM_SECTORS;
      scanOffset = LOGS_PER_SECTOR - 1;
    }
    logsScanned++;
  }

  if (targetSector == -1) {
    // Exact log not found (or since=0). Return logs from the oldest point we scanned back to.
    targetSector = scanSector;
    targetOffset = scanOffset + 1;
    if (targetOffset >= LOGS_PER_SECTOR) {
      targetOffset = 0;
      targetSector = (scanSector + 1) % FLASH_NUM_SECTORS;
    }
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
  currentSector = 0;
  currentOffset = 0;
  EEPROM.put(EEPROM_CURRENT_SECTOR, currentSector);
  EEPROM.commit();
  ESP.flashEraseSector((FLASH_LOG_START) / FLASH_SECTOR_SIZE);
  lastLoggedTemp = -100.0;
  lastLoggedHum = -100.0;
  lastLoggedStates = 0xFF;
}
