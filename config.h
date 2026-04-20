#ifndef CONFIG_H
#define CONFIG_H

// WiFi Configuration
#define WIFI_SSID "Sweet Home"
#define WIFI_PASSWORD "dishoom1234"

// Target Values
extern float TARGET_TEMP;
extern float TARGET_HUMIDITY;
#define TEMP_HYSTERESIS 0.5
#define HUMIDITY_HYSTERESIS 5.0
#define MAX_SAFE_TEMP 38.0

// Pin Definitions
#define DHTPIN D4
#define RELAY_HEATER D1
#define RELAY_ATOMIZER D2
#define RELAY_FAN D3
#define SERVO_PIN D5

// Timing Constants
extern unsigned long PULSE_ON_TIME;
#define PULSE_OFF_TIME 10000
#define FAN_EXTEND_TIME 5000
extern unsigned long LOG_INTERVAL;
extern unsigned long SAVE_FLASH_INTERVAL;
extern unsigned long EGG_TURN_INTERVAL;
#define EGG_TURN_DURATION 10000      // 10 seconds
#define SERVO_CENTER 90
#define SERVO_ANGLE 45

// Storage
#define MAX_LOG_ENTRIES 500          // RAM capacity (~4KB)
#define LOG_SECTOR_COUNT 256         // 1MB = 256 sectors for 15 days

#endif
