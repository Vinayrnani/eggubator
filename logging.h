#ifndef LOGGING_H
#define LOGGING_H

#include <Arduino.h>
#include <EEPROM.h>

struct __attribute__((packed)) LogEntry {
  uint32_t timestamp;
  uint16_t temp;
  uint16_t hum;
  uint8_t states;
  uint8_t servoPos;
};

struct __attribute__((packed)) FlashLogSectorHeader {
  uint32_t magic;
  uint32_t version;
  uint32_t bootId;
  uint32_t entryCount;
  uint32_t entrySize;
};

#define MAX_LOG_ENTRIES 500
#define LOG_SECTOR_COUNT 256
#define SECTOR_SIZE 4096
#define LOG_FLASH_SIZE (LOG_SECTOR_COUNT * SECTOR_SIZE)
#define SECTOR_THRESHOLD 390

#define FLASH_LOG_MAGIC 0x45474755UL
#define FLASH_LOG_VERSION 1UL
#define FLASH_LOG_ENTRIES_PER_SECTOR ((SECTOR_SIZE - (int)sizeof(FlashLogSectorHeader)) / (int)sizeof(LogEntry))

#define STATE_HEATER    0x01
#define STATE_ATOMIZER  0x02
#define STATE_FAN       0x04
#define STATE_SERVO     0x08

extern LogEntry logBuffer[MAX_LOG_ENTRIES];
extern int logIndex;
extern bool logFull;
extern unsigned long lastLogTime;
extern unsigned long lastSaveFlashTime;
extern int currentSector;
extern int currentBootFlashSectorCount;

void initLogging();
void logData(float temp, float hum, bool heater, bool atomizer, bool fan, int servo);
void getLogDataForWeb(String& json);
void getFlashLogDataForWeb(int sector, String& json);
bool shouldSaveToFlash();
void saveLogsToFlash();
void clearLogs();
void loadSettings();
void saveSettings();
uint32_t byteRead(int addr);
void byteWrite(int addr, uint32_t val);
int getFlashLogStartSector();
int getFlashLogSectorCount();
bool hasFlashLogSector(int sector);

#endif
