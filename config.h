#ifndef CONFIG_H
#define CONFIG_H

// WiFi Configuration
#define WIFI_SSID "Sweet Home"
#define WIFI_PASSWORD "dishoom1234"

// Target Values
extern float TARGET_TEMP;
extern float TARGET_HUMIDITY;
#define TEMP_HYSTERESIS 0.3
#define HUMIDITY_HYSTERESIS 5.0
#define MAX_SAFE_TEMP 38.0


// Pin Definitions
#define RELAY_HEATER D1
#define RELAY_ATOMIZER D2
#define RELAY_FAN D3
#define SERVO_PIN D5

// Timing Constants
extern unsigned long PULSE_ON_TIME;
extern unsigned long PULSE_OFF_TIME;
#define FAN_EXTEND_TIME 3000
extern unsigned long LOG_INTERVAL;
extern unsigned long EGG_TURN_INTERVAL;
extern unsigned long EGG_TURN_DURATION;

// Firmware Version
#define FIRMWARE_VERSION "1.4.4"

#endif
