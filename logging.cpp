#include <Arduino.h>
#include <EEPROM.h>
#include "logging.h"
#include "config.h"

LogEntry logBuffer[MAX_LOG_ENTRIES];
int logIndex = 0;
bool logFull = false;
unsigned long lastLogTime = 0;

void initLogging() {
  logIndex = 0;
  logFull = false;
  lastLogTime = millis();
  
  EEPROM.begin(512);
  Serial.println("Logging initialized (RAM-only)");
}

void logData(float temp, float hum, bool heater, bool atomizer, bool fan, int servo) {
  uint8_t states = 0;
  if (heater) states |= STATE_HEATER;
  if (atomizer) states |= STATE_ATOMIZER;
  if (fan) states |= STATE_FAN;
  if (servo != 0) states |= STATE_SERVO;

  logBuffer[logIndex].timestamp = millis();
  logBuffer[logIndex].temp = (uint16_t)(temp * 10 + 0.5);
  logBuffer[logIndex].hum = (uint16_t)(hum * 10 + 0.5);
  logBuffer[logIndex].states = states;
  logBuffer[logIndex].servoPos = (uint8_t)(servo + 90); // Store as 0-180
  
  logIndex++;
  if (logIndex >= MAX_LOG_ENTRIES) {
    logIndex = 0;
    logFull = true;
  }
}

void getLogDataForWeb(String& json) {
  int startIdx = logFull ? logIndex : 0;
  int count = logFull ? MAX_LOG_ENTRIES : logIndex;
  
  if (count > 0) {
    json += ",\"log\":[";
    for (int i = 0; i < count; i++) {
      int idx = (startIdx + i) % MAX_LOG_ENTRIES;
      float tempVal = logBuffer[idx].temp / 10.0;
      float humVal = logBuffer[idx].hum / 10.0;
      bool h = logBuffer[idx].states & STATE_HEATER;
      bool a = logBuffer[idx].states & STATE_ATOMIZER;
      bool f = logBuffer[idx].states & STATE_FAN;
      bool s = logBuffer[idx].states & STATE_SERVO;
      
      json += "{\"t\":" + String(logBuffer[idx].timestamp) +
             ",\"temp\":" + String(tempVal, 1) +
             ",\"hum\":" + String(humVal, 1) +
             ",\"h\":" + String(h ? "true" : "false") +
             ",\"a\":" + String(a ? "true" : "false") +
             ",\"f\":" + String(f ? "true" : "false") +
             ",\"s\":" + String(s ? "true" : "false") + "}";
      if (i < count - 1) json += ",";
    }
    json += "]";
  } else {
    json += ",\"log\":[]";
  }
}

void clearLogs() {
  logIndex = 0;
  logFull = false;
  Serial.println("RAM logs cleared");
}

void loadSettings() {
  extern unsigned long LOG_INTERVAL;
  extern unsigned long EGG_TURN_INTERVAL;
  extern unsigned long PULSE_ON_TIME;
  extern bool stageLockdown;
  
  if (EEPROM.read(EEPROM_SETTINGS_MAGIC) != 0xA5) {
    LOG_INTERVAL = 10000;
    EGG_TURN_INTERVAL = 7200000;
    PULSE_ON_TIME = 2000;
    stageLockdown = false;
    byteWrite(EEPROM_LOG_INTERVAL, LOG_INTERVAL);
    byteWrite(EEPROM_EGG_TURN, EGG_TURN_INTERVAL);
    byteWrite(EEPROM_PULSE_ON, PULSE_ON_TIME);
    EEPROM.write(EEPROM_STAGE, 0);
    EEPROM.write(EEPROM_SETTINGS_MAGIC, 0xA5);
    EEPROM.commit();
    return;
  }
  
  LOG_INTERVAL = byteRead(EEPROM_LOG_INTERVAL);
  EGG_TURN_INTERVAL = byteRead(EEPROM_EGG_TURN);
  PULSE_ON_TIME = byteRead(EEPROM_PULSE_ON);
  stageLockdown = (EEPROM.read(EEPROM_STAGE) == 1);
}

void saveSettings() {
  extern unsigned long LOG_INTERVAL;
  extern unsigned long EGG_TURN_INTERVAL;
  extern unsigned long PULSE_ON_TIME;
  extern bool stageLockdown;
  
  byteWrite(EEPROM_LOG_INTERVAL, LOG_INTERVAL);
  byteWrite(EEPROM_EGG_TURN, EGG_TURN_INTERVAL);
  byteWrite(EEPROM_PULSE_ON, PULSE_ON_TIME);
  EEPROM.write(EEPROM_STAGE, stageLockdown ? 1 : 0);
  EEPROM.commit();
}

uint32_t byteRead(int addr) {
  uint32_t val = 0;
  for (int i = 0; i < 4; i++) {
    val |= ((uint32_t)EEPROM.read(addr + i) << ((3 - i) * 8));
  }
  return val;
}

void byteWrite(int addr, uint32_t val) {
  for (int i = 0; i < 4; i++) {
    EEPROM.write(addr + i, (val >> ((3 - i) * 8)) & 0xFF);
  }
}
