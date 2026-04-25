#ifndef LOGGING_H
#define LOGGING_H

#include <Arduino.h>

struct __attribute__((packed)) LogEntry {
  uint32_t timestamp;
  uint8_t temp;    // (T-20)*10
  uint8_t hum;     // rounded to integer
  uint8_t states;  // bits: 0:Heater, 1:Atomizer, 2:Fan, 3-4:Turner state
};

#define MAX_LOG_ENTRIES 1000

#define STATE_HEATER    0x01
#define STATE_ATOMIZER  0x02
#define STATE_FAN       0x04
// Turner state in bits 3-4
// -1 -> 2 (binary 10)
//  0 -> 0 (binary 00)
//  1 -> 1 (binary 01)
#define GET_TURNER(s)   (((s) >> 3) & 0x03)
#define SET_TURNER(s,v) ((s) | (((v) & 0x03) << 3))

extern LogEntry logBuffer[MAX_LOG_ENTRIES];
extern int logIndex;
extern bool logFull;
extern unsigned long lastLogTime;

extern float lastLoggedTemp;
extern float lastLoggedHum;
extern uint8_t lastLoggedStates;

void initLogging();
bool logData(float temp, float hum, bool heater, bool atomizer, bool fan, int servo, unsigned long forceInterval = 60000);
int getLogHex(String& hex, int maxEntries = 100, uint32_t sinceTimestamp = 0);
void clearLogs();
uint32_t getLatestLogTimestamp();
uint32_t getOldestLogTimestamp();

#endif
