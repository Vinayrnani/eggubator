#include "logging.h"
#include <Arduino.h>

LogEntry logBuffer[MAX_LOG_ENTRIES];
int logIndex = 0;
bool logFull = false;
unsigned long lastLogTime = 0;

float lastLoggedTemp = -100.0;
float lastLoggedHum = -100.0;
uint8_t lastLoggedStates = 0xFF;

void initLogging() {
  logIndex = 0;
  logFull = false;
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

  // Event-driven check
  bool significant = false;
  if (states != lastLoggedStates) significant = true;
  else if (fabs(temp - lastLoggedTemp) > 0.31) significant = true; // Use 0.31 to avoid flutter at exactly 0.3
  else if (fabs(hum - lastLoggedHum) > 3.01) significant = true;
  else if (lastLogTime == 0 || (millis() - lastLogTime >= forceInterval)) significant = true;

  if (!significant) return false;

  if (logIndex >= MAX_LOG_ENTRIES) {
    logIndex = 0;
    logFull = true;
  }
  
  logBuffer[logIndex].timestamp = millis();
  logBuffer[logIndex].temp = t_encoded;
  logBuffer[logIndex].hum = h_encoded;
  logBuffer[logIndex].states = states;
  
  lastLoggedTemp = temp;
  lastLoggedHum = hum;
  lastLoggedStates = states;
  lastLogTime = millis();
  
  logIndex++;
  return true;
}

int getLogHex(String& hex, int maxEntries, uint32_t sinceTimestamp) {
  int total = logFull ? MAX_LOG_ENTRIES : logIndex;
  int start = logFull ? logIndex : 0;
  int sent = 0;
  
  hex.reserve(maxEntries * 14);
  
  for (int i = 0; i < total && sent < maxEntries; i++) {
    int idx = (start + i) % MAX_LOG_ENTRIES;
    LogEntry& e = logBuffer[idx];
    if (e.timestamp > sinceTimestamp) {
      uint8_t* ptr = (uint8_t*)&e;
      for (int j = 0; j < 7; j++) {
        if (ptr[j] < 16) hex += "0";
        hex += String(ptr[j], HEX);
      }
      sent++;
    }
  }
  return sent;
}

void clearLogs() {
  logIndex = 0;
  logFull = false;
  lastLoggedTemp = -100.0;
  lastLoggedHum = -100.0;
  lastLoggedStates = 0xFF;
}

uint32_t getLatestLogTimestamp() {
  if (logIndex == 0 && !logFull) return 0;
  int idx;
  if (logFull) {
    idx = (logIndex - 1 + MAX_LOG_ENTRIES) % MAX_LOG_ENTRIES;
  } else {
    idx = logIndex - 1;
  }
  return logBuffer[idx].timestamp;
}

uint32_t getOldestLogTimestamp() {
  if (logIndex == 0 && !logFull) return 0;
  int idx;
  if (logFull) {
    idx = logIndex;
  } else {
    idx = 0;
  }
  return logBuffer[idx].timestamp;
}
