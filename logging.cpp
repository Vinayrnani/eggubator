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

bool logData(float temp, float hum, bool heater, bool atomizer, bool fan, int servo) {
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

  if (!significant && lastLogTime != 0) return false;

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

void getLogHex(String& hex) {
  int count = logFull ? MAX_LOG_ENTRIES : logIndex;
  int start = logFull ? logIndex : 0;
  
  hex.reserve(count * 14); // 7 bytes * 2 chars per byte
  
  for (int i = 0; i < count; i++) {
    int idx = (start + i) % MAX_LOG_ENTRIES;
    uint8_t* ptr = (uint8_t*)&logBuffer[idx];
    for (int j = 0; j < 7; j++) {
      if (ptr[j] < 16) hex += "0";
      hex += String(ptr[j], HEX);
    }
  }
}

void clearLogs() {
  logIndex = 0;
  logFull = false;
  lastLoggedTemp = -100.0;
  lastLoggedHum = -100.0;
  lastLoggedStates = 0xFF;
}
