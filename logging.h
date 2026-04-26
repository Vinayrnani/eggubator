#ifndef LOGGING_H
#define LOGGING_H

#include <Arduino.h>

struct __attribute__((packed)) LogEntry {
  uint32_t timeSec;
  uint8_t temp;    // (T-20)*10
  uint8_t hum;     // rounded to integer
  uint8_t states;  // bits: 0:Heater, 1:Atomizer, 2:Fan, 3-4:Turner state
  uint8_t bootId;
};

#define FLASH_LOG_START 0x200000
#define FLASH_SECTOR_SIZE_VAL 4096
#define FLASH_NUM_SECTORS 256
#define LOGS_PER_SECTOR (FLASH_SECTOR_SIZE / sizeof(LogEntry))
#define MAX_LOG_ENTRIES (LOGS_PER_SECTOR * FLASH_NUM_SECTORS)

#define EEPROM_CURRENT_SECTOR 32

#define STATE_HEATER    0x01
#define STATE_ATOMIZER  0x02
#define STATE_FAN       0x04
// Turner state in bits 3-4
#define GET_TURNER(s)   (((s) >> 3) & 0x03)
#define SET_TURNER(s,v) ((s) | (((v) & 0x03) << 3))

extern uint8_t currentBootId;
extern unsigned long lastLogTime;

extern float lastLoggedTemp;
extern float lastLoggedHum;
extern uint8_t lastLoggedStates;

void initLogging(uint8_t bootId);
bool logData(float temp, float hum, bool heater, bool atomizer, bool fan, int servo, unsigned long forceInterval = 90000);
int getLogHex(String& hex, int maxEntries = 200, uint8_t sinceBootId = 0, uint32_t sinceTimeSec = 0);
void clearLogs();

#endif
