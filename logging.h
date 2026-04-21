#ifndef LOGGING_H
#define LOGGING_H

#include <Arduino.h>
#include <EEPROM.h>
#include "config.h"

struct LogEntry {
  uint32_t timestamp;
  uint16_t temp;
  uint16_t hum;
  uint8_t states;
  int8_t servoPos;
  uint8_t reserved[2];
};

static_assert(sizeof(LogEntry) % 4 == 0, "LogEntry must remain word-aligned");

#define SECTOR_SIZE 4096
#define LOG_FLASH_SIZE (LOG_SECTOR_COUNT * SECTOR_SIZE)
#define LOG_ENTRIES_PER_SECTOR ((int)(SECTOR_SIZE / sizeof(LogEntry)))
#define LOG_SECTOR_HEADROOM 8
#define SECTOR_THRESHOLD (LOG_ENTRIES_PER_SECTOR > LOG_SECTOR_HEADROOM ? (LOG_ENTRIES_PER_SECTOR - LOG_SECTOR_HEADROOM) : LOG_ENTRIES_PER_SECTOR)

#define STATE_HEATER   0x01
#define STATE_ATOMIZER 0x02
#define STATE_FAN      0x04
#define STATE_SERVO    0x08

extern LogEntry logBuffer[MAX_LOG_ENTRIES];
extern int logIndex;
extern bool logFull;
extern unsigned long lastLogTime;
extern unsigned long lastSaveFlashTime;
extern int currentSector;

void initLogging();
void logData(float temp, float hum, bool heater, bool atomizer, bool fan, int servoPos);
void getLogDataForWeb(String& json);
void getFlashLogDataForWeb(int sector, String& json);
bool shouldSaveToFlash();
void saveLogsToFlash();
void clearLogs();
int getLogEntryCount();
size_t getLogStorageBytes();
uint32_t byteRead(int addr);
void byteWrite(int addr, uint32_t val);

#endif
