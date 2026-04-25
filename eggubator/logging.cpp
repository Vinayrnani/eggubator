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
  else if (fabs(temp - lastLoggedTemp) > 0.11) significant = true; // Use 0.11 to avoid flutter at exactly 0.1
  else if (fabs(hum - lastLoggedHum) > 1.01) significant = true;
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

int getLogHex(String& hex, uint32_t since) {
  int count = logFull ? MAX_LOG_ENTRIES : logIndex;
  int start = logFull ? logIndex : 0;
  int found = 0;
  
  // Quick check: if the latest log is already seen, return 0
  if (count > 0) {
    int latestIdx = (start + count - 1) % MAX_LOG_ENTRIES;
    if (logBuffer[latestIdx].timestamp <= since) return 0;
  }

  for (int i = 0; i < count; i++) {
    int idx = (start + i) % MAX_LOG_ENTRIES;
    if (since == 0 || logBuffer[idx].timestamp > since) {
      uint8_t* ptr = (uint8_t*)&logBuffer[idx];
      for (int j = 0; j < 7; j++) {
        if (ptr[j] < 16) hex += "0";
        hex += String(ptr[j], HEX);
      }
      found++;
    }
  }
  return found;
}

void clearLogs() {
  logIndex = 0;
  logFull = false;
  lastLoggedTemp = -100.0;
  lastLoggedHum = -100.0;
  lastLoggedStates = 0xFF;
}
