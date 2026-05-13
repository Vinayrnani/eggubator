#ifndef SAT_MANAGER_H
#define SAT_MANAGER_H

#include <Arduino.h>
#include <ESP8266WebServer.h>
#include "logging.h"

// EEPROM addresses for SAT
#define EEPROM_BOOT_ID 12
#define EEPROM_LAST_KNOWN_BOOT_ID 15
#define EEPROM_LAST_KNOWN_START_UNIX 16

struct BootTimestamp {
  uint8_t bootId;
  uint32_t startUnix;
  uint32_t duration;
};

extern BootTimestamp* bootTable;
extern int bootTableCount;
extern uint32_t startTimestamp;

extern ESP8266WebServer server;

uint32_t scanBootDuration(int bootIdx);
void sortBootTable();
void prepareBootTable();
uint32_t getBootUptime();
uint32_t getElapsedSeconds();
uint32_t getCurrentDay();
void handleTimestamps();

#endif
