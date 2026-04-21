#ifndef LOGGING_H
#define LOGGING_H

#include <Arduino.h>
#include <EEPROM.h>

struct LogEntry {
  uint32_t timestamp;
  uint16_t temp;    // temperature × 10 (0-6553.5°C)
  uint16_t hum;     // humidity × 10 (0-6553.5%)
  uint8_t states;   // bit 0: heater, bit 1: atomizer, bit 2: fan, bit 3: servo
  uint8_t servoPos; // servo position (0-180)
};

#define MAX_LOG_ENTRIES 500
#define LOG_SECTOR_COUNT 256
#define SECTOR_SIZE 4096
#define LOG_FLASH_SIZE (LOG_SECTOR_COUNT * SECTOR_SIZE)
#define SECTOR_THRESHOLD 390

#define STATE_HEATER    0x01
#define STATE_ATOMIZER  0x02
#define STATE_FAN       0x04
#define STATE_SERVO     0x08

extern LogEntry logBuffer[MAX_LOG_ENTRIES];
extern int logIndex;
extern bool logFull;
extern unsigned long lastLogTime;
extern unsigned long lastSaveFlashTime;

void initLogging();
void logData(float temp, float hum, bool heater, bool atomizer, bool fan, int servo);
void getLogDataForWeb(String& json);
bool shouldSaveToFlash();
void saveLogsToFlash();
void clearLogs();
void loadSettings();
void saveSettings();
uint32_t byteRead(int addr);
void byteWrite(int addr, uint32_t val);

#endif
