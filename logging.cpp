#include "logging.h"
#include <Arduino.h>

LogEntry logBuffer[MAX_LOG_ENTRIES];
int logIndex = 0;
bool logFull = false;
unsigned long lastLogTime = 0;

void initLogging() {
  logIndex = 0;
  logFull = false;
  lastLogTime = 0;
}

void logData(float temp, float hum, bool heater, bool atomizer, bool fan, int servo) {
  if (logIndex >= MAX_LOG_ENTRIES) {
    logIndex = 0;
    logFull = true;
  }
  
  logBuffer[logIndex].timestamp = millis();
  logBuffer[logIndex].temp = (uint16_t)(temp * 10);
  logBuffer[logIndex].hum = (uint16_t)(hum * 10);
  
  uint8_t states = 0;
  if (heater) states |= STATE_HEATER;
  if (atomizer) states |= STATE_ATOMIZER;
  if (fan) states |= STATE_FAN;
  if (servo != 0) states |= STATE_SERVO; // non-zero means enabled or moving
  
  logBuffer[logIndex].states = states;
  logBuffer[logIndex].servoPos = (uint8_t)(servo + 1); // simple mapping for example
  
  logIndex++;
}

void getLogDataForWeb(String& json) {
  json += ",\"log\":[";
  int count = logFull ? MAX_LOG_ENTRIES : logIndex;
  int start = logFull ? logIndex : 0;
  
  for (int i = 0; i < count; i++) {
    int idx = (start + i) % MAX_LOG_ENTRIES;
    if (i > 0) json += ",";
    
    json += "{\"t\":" + String(logBuffer[idx].timestamp) +
            ",\"temp\":" + String(logBuffer[idx].temp / 10.0) +
            ",\"hum\":" + String(logBuffer[idx].hum / 10.0) +
            ",\"h\":" + String((logBuffer[idx].states & STATE_HEATER) ? 1 : 0) +
            ",\"a\":" + String((logBuffer[idx].states & STATE_ATOMIZER) ? 1 : 0) +
            ",\"f\":" + String((logBuffer[idx].states & STATE_FAN) ? 1 : 0) +
            ",\"s\":" + String((logBuffer[idx].states & STATE_SERVO) ? 1 : 0) + "}";
  }
  json += "]";
}

void clearLogs() {
  logIndex = 0;
  logFull = false;
}
