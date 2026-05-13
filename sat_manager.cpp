#include "sat_manager.h"
#include <EEPROM.h>

BootTimestamp* bootTable = NULL;
int bootTableCount = 0;

uint32_t scanBootDuration(int bootIdx) {
  int s = bootIndex[bootIdx].sector;
  int o = bootIndex[bootIdx].offset;
  uint32_t lastTimeSec = 0;

  int endS, endO;
  if (bootIdx + 1 < bootIndexCount) {
    endS = bootIndex[bootIdx + 1].sector;
    endO = bootIndex[bootIdx + 1].offset;
  } else {
    endS = currentSector;
    endO = currentOffset;
  }

  int limit = 0;
  while (!(s == endS && o == endO) && limit < 2000) {
    limit++;
    uint32_t addr = FLASH_LOG_START + (s * LOG_SECTOR_SIZE) + (o * sizeof(LogEntry));
    LogEntry entry;
    ESP.flashRead(addr, (uint32_t*)&entry, sizeof(LogEntry));
    if (entry.timeSec != 0xFFFFFFFF) {
      lastTimeSec = entry.timeSec;
    }
    o++;
    if (o >= LOGS_PER_SECTOR) { o = 0; s = (s + 1) % FLASH_NUM_SECTORS; }
  }
  return lastTimeSec;
}

void sortBootTable() {
  for (int i = 0; i < bootTableCount - 1; i++) {
    for (int j = 0; j < bootTableCount - i - 1; j++) {
      if (bootTable[j].bootId > bootTable[j + 1].bootId) {
        BootTimestamp t = bootTable[j];
        bootTable[j] = bootTable[j + 1];
        bootTable[j + 1] = t;
      }
    }
  }
}

void prepareBootTable() {
  if (bootTable) { free(bootTable); bootTable = NULL; bootTableCount = 0; }

  int allocSize = bootIndexCount + 1;
  if (allocSize < 1) allocSize = 1;
  bootTable = (BootTimestamp*)malloc(allocSize * sizeof(BootTimestamp));
  if (!bootTable) { Serial.println("FATAL: bootTable OOM"); return; }
  bootTableCount = 0;

  for (int i = 0; i < bootIndexCount && i < allocSize; i++) {
    bootTable[bootTableCount].bootId = bootIndex[i].bootId;
    bootTable[bootTableCount].duration = scanBootDuration(i);
    bootTable[bootTableCount].startUnix = 0;
    bootTableCount++;
  }
  sortBootTable();

  uint32_t lastKnownStartUnix = 0;
  uint8_t lastKnownBootId = EEPROM.read(EEPROM_LAST_KNOWN_BOOT_ID);
  EEPROM.get(EEPROM_LAST_KNOWN_START_UNIX, lastKnownStartUnix);
  if (lastKnownStartUnix > 2000000000) lastKnownStartUnix = 0;

  int anchorIdx = -1;
  for (int i = 0; i < bootTableCount; i++) {
    if (bootTable[i].bootId == lastKnownBootId) {
      bootTable[i].startUnix = lastKnownStartUnix;
      anchorIdx = i; break;
    }
  }
  if (anchorIdx >= 0) {
    for (int i = anchorIdx - 1; i >= 0; i--)
      bootTable[i].startUnix = bootTable[i + 1].startUnix - bootTable[i].duration;
    for (int i = anchorIdx + 1; i < bootTableCount; i++)
      bootTable[i].startUnix = bootTable[i - 1].startUnix + bootTable[i - 1].duration;
  }

  uint8_t curBootId = EEPROM.read(EEPROM_BOOT_ID);
  uint32_t curStart = 0;
  if (bootTableCount > 0)
    curStart = bootTable[bootTableCount - 1].startUnix + bootTable[bootTableCount - 1].duration;
  else if (lastKnownStartUnix > 0)
    curStart = lastKnownStartUnix;

  // Ensure current session exists correctly
  if (bootTableCount == 0 || bootTable[bootTableCount - 1].bootId != curBootId) {
    bootTable[bootTableCount].bootId = curBootId;
    bootTable[bootTableCount].startUnix = curStart;
    bootTable[bootTableCount].duration = 0;
    bootTableCount++;
  }

  Serial.println("SAT boot table OK (" + String(bootTableCount) + " entries)");
}

uint32_t getBootUptime() {
  return millis() / 1000;
}

uint32_t getElapsedSeconds() {
  if (bootTable == NULL || bootTableCount == 0) return 0;
  uint32_t currentStartUnix = bootTable[bootTableCount - 1].startUnix;
  if (currentStartUnix == 0 || startTimestamp == 0 || currentStartUnix < startTimestamp) {
    return getBootUptime();
  }
  return currentStartUnix + getBootUptime() - startTimestamp;
}

uint32_t getCurrentDay() {
  return getElapsedSeconds() / 86400;
}

void handleTimestamps() {
  if (server.method() == HTTP_GET) {
    String json = "{\"currentBootId\":" + String(currentBootId) +
                  ",\"bootUptimeSec\":" + String(getBootUptime()) +
                  ",\"bootTable\":[";
    for (int i = 0; i < bootTableCount; i++) {
      if (i > 0) json += ",";
      json += "{\"bootId\":" + String(bootTable[i].bootId) +
              ",\"startUnix\":" + String(bootTable[i].startUnix) + "}";
    }
    json += "]}";
    server.send(200, "application/json", json);
    return;
  }

  if (server.method() == HTTP_PUT) {
    if (!server.hasArg("plain")) {
      server.send(400, "application/json", "{\"synced\":false}");
      return;
    }

    String body = server.arg("plain");
    int count = 0, pos = 0;
    while (true) {
      int idx = body.indexOf("\"bootId\"", pos);
      if (idx == -1) break;
      count++;
      pos = idx + 8;
    }
    if (count == 0) { server.send(400, "application/json", "{\"synced\":false}"); return; }

    int tempCount = count;
    BootTimestamp* tempTable = (BootTimestamp*)malloc(tempCount * sizeof(BootTimestamp));
    if (!tempTable) { server.send(500, "application/json", "{\"synced\":false}"); return; }
    
    int tempTableCount = 0;
    pos = 0;

    uint32_t prevStartUnix = 0;
    uint8_t prevBootId = 0;
    for (int i = 0; i < count; i++) {
      int bidx = body.indexOf("\"bootId\"", pos);
      int suidx = body.indexOf("\"startUnix\"", pos);
      if (bidx == -1 || suidx == -1) break;

      int bstart = body.indexOf(':', bidx + 8) + 1;
      int bend = body.indexOf(',', bstart);
      if (bend == -1) bend = body.indexOf('}', bstart);
      uint8_t bootId = (uint8_t)body.substring(bstart, bend).toInt();

      int sustart = body.indexOf(':', suidx + 10) + 1;
      int suend = body.indexOf(',', sustart);
      if (suend == -1) suend = body.indexOf('}', sustart);
      uint32_t startUnix = (uint32_t)body.substring(sustart, suend).toInt();

      if (bootId == currentBootId) prevStartUnix = startUnix;
      if (bootId == currentBootId) prevBootId = bootId;

      tempTable[tempTableCount].bootId = bootId;
      tempTable[tempTableCount].startUnix = startUnix;
      
      uint32_t duration = 0;
      if (bootTable != NULL) {
        for (int j = 0; j < bootTableCount; j++) {
          if (bootTable[j].bootId == bootId) {
            duration = bootTable[j].duration;
            break;
          }
        }
      }
      tempTable[tempTableCount].duration = duration;
      
      tempTableCount++;
      pos = body.indexOf('}', bidx) + 1;
    }

    // Update EEPROM anchor only if drift > 5s on current boot
    if (prevStartUnix > 0 && prevBootId == currentBootId) {
      uint32_t espBootStart = 0;
      for (int i = 0; i < bootTableCount; i++) {
        if (bootTable[i].bootId == currentBootId) { espBootStart = bootTable[i].startUnix; break; }
      }
      int32_t drift = abs((int32_t)(prevStartUnix - espBootStart));
      if (drift > 5) {
        EEPROM.put(EEPROM_LAST_KNOWN_START_UNIX, prevStartUnix);
        EEPROM.write(EEPROM_LAST_KNOWN_BOOT_ID, currentBootId);
        EEPROM.commit();
      }
    }

    if (bootTable) { free(bootTable); }
    bootTable = tempTable;
    bootTableCount = tempTableCount;

    server.send(200, "application/json",
      "{\"synced\":true,\"entriesStored\":" + String(bootTableCount) + "}");
    return;
  }

  server.send(405, "text/plain", "Method not allowed");
}
