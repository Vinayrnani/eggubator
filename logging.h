#ifndef LOGGING_H
#define LOGGING_H

#include <EEPROM.h>

struct LogEntry {
  unsigned long timestamp;
  float temperature;
  float humidity;
  bool heaterState;
  bool atomizerState;
  bool fanState;
  int servoPosition;
};

#define MAX_LOG_ENTRIES 100

// EEPROM addresses
#define EEPROM_MAGIC 0
#define EEPROM_INDEX 1
#define EEPROM_CHECK 2
#define MAGIC_VAL 0x42
#define CHECK_VAL 0xAB

extern LogEntry logBuffer[MAX_LOG_ENTRIES];
extern int logIndex;
extern bool logFull;
extern unsigned long lastLogTime;

void initLogging() {
  EEPROM.begin(512);
  
  unsigned char magic = EEPROM.read(EEPROM_MAGIC);
  unsigned char check = EEPROM.read(EEPROM_CHECK);
  unsigned char savedIndex = EEPROM.read(EEPROM_INDEX);
  
  if (magic == MAGIC_VAL && check == CHECK_VAL && savedIndex <= MAX_LOG_ENTRIES) {
    logIndex = savedIndex;
    logFull = (logIndex >= MAX_LOG_ENTRIES);
    Serial.println("Restored log data from EEPROM");
  } else {
    logIndex = 0;
    logFull = false;
    Serial.println("Starting fresh log");
  }
  
  Serial.print("Log entries: ");
  Serial.println(logIndex);
}

void saveCheckpoint() {
  EEPROM.write(EEPROM_MAGIC, MAGIC_VAL);
  EEPROM.write(EEPROM_INDEX, (unsigned char)logIndex);
  EEPROM.write(EEPROM_CHECK, CHECK_VAL);
  EEPROM.commit();
}

void logData(float temp, float hum, bool heater, bool atomizer, bool fan, int servo) {
  if (logIndex < MAX_LOG_ENTRIES) {
    logBuffer[logIndex].timestamp = millis();
    logBuffer[logIndex].temperature = temp;
    logBuffer[logIndex].humidity = hum;
    logBuffer[logIndex].heaterState = heater;
    logBuffer[logIndex].atomizerState = atomizer;
    logBuffer[logIndex].fanState = fan;
    logBuffer[logIndex].servoPosition = servo;
    logIndex++;
    if (logIndex >= MAX_LOG_ENTRIES) {
      logIndex = 0;
      logFull = true;
    }
  } else {
    logIndex = 0;
    logFull = true;
    logBuffer[logIndex].timestamp = millis();
    logBuffer[logIndex].temperature = temp;
    logBuffer[logIndex].humidity = hum;
    logBuffer[logIndex].heaterState = heater;
    logBuffer[logIndex].atomizerState = atomizer;
    logBuffer[logIndex].fanState = fan;
    logBuffer[logIndex].servoPosition = servo;
    logIndex++;
  }
  
  saveCheckpoint();
}

void getLogDataForWeb(String& json) {
  int startIdx = logFull ? logIndex : 0;
  int count = logFull ? MAX_LOG_ENTRIES : logIndex;
  int entriesToShow = count;
  
  if (entriesToShow > 0) {
    json += ",\"log\":[";
    for (int i = 0; i < entriesToShow; i++) {
      int idx = (startIdx + i) % MAX_LOG_ENTRIES;
      json += "{\"t\":" + String(logBuffer[idx].timestamp) +
             ",\"temp\":" + String(logBuffer[idx].temperature, 1) +
             ",\"hum\":" + String(logBuffer[idx].humidity, 1) +
             ",\"h\":" + String(logBuffer[idx].heaterState ? "true" : "false") +
             ",\"a\":" + String(logBuffer[idx].atomizerState ? "true" : "false") +
             ",\"f\":" + String(logBuffer[idx].fanState ? "true" : "false") +
             ",\"s\":" + String(logBuffer[idx].servoPosition ? "true" : "false") + "}";
      if (i < entriesToShow - 1) json += ",";
    }
    json += "]";
  } else {
    json += ",\"log\":[]";
  }
}

#endif
