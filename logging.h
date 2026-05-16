#ifndef LOGGING_H
#define LOGGING_H

#include <Arduino.h>

struct __attribute__((packed)) LogEntry {
  uint32_t timeSec;
  uint8_t temp;    // (T-20)*10
  uint8_t hum;     // rounded to integer
  uint8_t states;  // bits: 0:Heater, 1:Atomizer, 2:Fan, 3-7:Turner step (0-31)
  uint8_t bootId;
};

#define FLASH_LOG_START 0x200000
#ifndef FLASH_SECTOR_SIZE
#define FLASH_SECTOR_SIZE 4096
#endif
#define FLASH_NUM_SECTORS 256
#define LOGS_PER_SECTOR (FLASH_SECTOR_SIZE / sizeof(LogEntry))
#define MAX_LOG_ENTRIES (LOGS_PER_SECTOR * FLASH_NUM_SECTORS)

#define EEPROM_CURRENT_SECTOR 32
#define EEPROM_START_SECTOR 34

#define META_SECTOR_POINTER 0xFE
#define META_CORRECTION     0xFF

#define STATE_HEATER    0x01
#define STATE_ATOMIZER  0x02
#define STATE_FAN       0x04
// Turner state in bits 3-7 (0-31 steps)
#define GET_TURNER(s)   (((s) >> 3) & 0x1F)
#define SET_TURNER(s,v) (((s) & 0x07) | (((v) & 0x1F) << 3))

extern uint8_t currentBootId;
extern uint16_t currentSector;
extern uint16_t currentOffset;
extern uint16_t startSector;
extern uint32_t logsInCurrentBoot;
extern unsigned long lastLogTime;

struct BootSession {
  uint8_t bootId;
  uint16_t sector;
  uint16_t offset;
  uint32_t duration;
  uint32_t startUnix;
};

extern BootSession* bootSessions;
extern int bootSessionCount;
extern int bootSessionCapacity;

extern float lastLoggedTemp;
extern float lastLoggedHum;
extern uint8_t lastLoggedStates;

void initSectorPointers();
void initLogging(uint8_t bootId);
bool logData(float temp, float hum, bool heater, bool atomizer, bool fan, uint8_t servoStep, unsigned long forceInterval = 90000);
int getLogHex(String& hex, int maxEntries = 200, uint8_t sinceBootId = 0, uint32_t sinceTimeSec = 0);
int getTotalLogs();
void clearLogs();
void writeCorrectionLog(uint8_t bootId, uint32_t duration);
bool getLastServoPositions(uint8_t* out_steps, int count);

#endif
