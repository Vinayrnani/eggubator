// ============================================
// EGG INCUBATOR CONTROLLER - Modular Version
// SAT ARCHITECTURE ENABLED
// ============================================

// Include header files
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266HTTPClient.h>
#include <Servo.h>
#include <EEPROM.h>
#include <ESP8266mDNS.h>

#include "config.h"
#include "dht_sensor.h"
#include "wifi_manager.h"
#include "logging.h"
#include "updates.h"
#include "web_ui.h"

extern bool useMockSensor;
extern bool autoSimMode;
extern float mockTemp;
extern float mockHum;
extern float simTemp;
extern float simHum;
extern void updateAutoSim(bool heater, bool atomizer, bool fan);

#define KILL_OFF 0
#define AUTO 1

// Configurable timing (can be changed via web)
unsigned long LOG_INTERVAL = 90000;
unsigned long EGG_TURN_INTERVAL = 7200000;
unsigned long EGG_TURN_DURATION = 10000;
unsigned long PULSE_ON_TIME = 3000;

// Target temperature and humidity (can be changed via web/stage selection)
float TARGET_TEMP = 37.5;    // Default 37.5°C
float TARGET_HUMIDITY = 55.0; // Default 55.0%

// Global variables
float currentTemp = 0;
float currentHumidity = 0;
bool heaterState = false;
bool atomizerState = false;
bool fanState = false;
bool servoEnabled = true;
int servoPosition = 0; // -1 = -45deg, 0 = center, 1 = +45deg
int heaterMode = AUTO;
bool stageLockdown = false;  // false = incubation (1-18), true = lockdown (19-21)
int atomizerMode = AUTO;
int fanMode = AUTO;
int servoMode = AUTO;
unsigned long lastReadTime = 0;
unsigned long lastOtaCheck = 0;
unsigned long lastServoTurn = 0;
bool restingAt45 = true; // Servo position: true=45deg(left), false=135deg(right)
int8_t angleAdjustment = 0;
bool adjustingAngle = false;
unsigned long angleAdjustStartTime = 0;
int8_t angleAdjustFrom = 0;
int8_t angleAdjustTo = 0;

Servo myServo;

struct BootTimestamp {
  uint8_t bootId;
  uint32_t startUnix;
  uint32_t duration;
};

BootTimestamp* bootTable = nullptr;
int bootTableCount = 0;
int bootTableCapacity = 0;

uint32_t startTimestamp = 0;
unsigned long lastEEPROMSaveMillis = 0;
unsigned long lastElapsedMillis = 0;

// Control state variables
unsigned long atomizerPulseStart = 0;
bool atomizerPulsing = false;
bool atomizerInOffPhase = false;
unsigned long atomizerOffStart = 0;
unsigned long heaterLastChanged = 0;
bool heaterWasOn = false;
unsigned long atomizerLastChanged = 0;
bool atomizerWasOn = false;

// Web server
ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;


// EEPROM addresses for settings
#define EEPROM_SETTINGS_MAGIC 40
#define EEPROM_BOOT_ID 12
#define EEPROM_SERVO_REST 13
// EEPROM_ELAPSED_ADDR 14 removed - SAT uses startTimestamp + bootTable
#define SETTINGS_MAGIC_VAL 0xA8

struct DeviceSettings {
  uint8_t magic;
  bool stageLockdown;
  unsigned long logInterval;
  unsigned long turnInterval;
  unsigned long pulseOnTime;
  uint32_t startTimestamp;
  uint8_t turnDurationSeconds;
  int8_t angleAdjustment;
};

uint32_t getElapsedSeconds() {
  uint8_t currentBootId = EEPROM.read(EEPROM_BOOT_ID);
  uint32_t startUnix = 0;
  for (int i = 0; i < bootTableCount; i++) {
    if (bootTable[i].bootId == currentBootId) {
      startUnix = bootTable[i].startUnix;
      break;
    }
  }
  if (startUnix == 0) return 0; // Not synced yet
  return startUnix + (millis() / 1000) - startTimestamp;
}

uint32_t getCurrentDay() {
  return getElapsedSeconds() / 86400;
}

bool isLockdown() {
  return getCurrentDay() >= 18;
}

void saveSettings() {
  DeviceSettings settings;
  EEPROM.get(EEPROM_SETTINGS_MAGIC, settings);
  
  bool changed = false;
  if (settings.magic != SETTINGS_MAGIC_VAL) { settings.magic = SETTINGS_MAGIC_VAL; changed = true; }
  if (settings.stageLockdown != stageLockdown) { settings.stageLockdown = stageLockdown; changed = true; }
  if (settings.logInterval != LOG_INTERVAL) { settings.logInterval = LOG_INTERVAL; changed = true; }
  if (EGG_TURN_INTERVAL != 20000 && settings.turnInterval != EGG_TURN_INTERVAL) { settings.turnInterval = EGG_TURN_INTERVAL; changed = true; }
  if (settings.turnDurationSeconds != (EGG_TURN_DURATION / 1000)) { settings.turnDurationSeconds = (EGG_TURN_DURATION / 1000); changed = true; }
  if (settings.angleAdjustment != angleAdjustment) { settings.angleAdjustment = angleAdjustment; changed = true; }
  if (settings.pulseOnTime != PULSE_ON_TIME) { settings.pulseOnTime = PULSE_ON_TIME; changed = true; }
  if (settings.startTimestamp != startTimestamp) { settings.startTimestamp = startTimestamp; changed = true; }
  
  if (changed) {
    EEPROM.put(EEPROM_SETTINGS_MAGIC, settings);
    EEPROM.commit();
    Serial.println("Settings saved to EEPROM");
  }
}

void loadSettings() {
  DeviceSettings settings;
  EEPROM.get(EEPROM_SETTINGS_MAGIC, settings);
  
  if (settings.magic == SETTINGS_MAGIC_VAL) {
    stageLockdown = settings.stageLockdown;
    LOG_INTERVAL = settings.logInterval;
    EGG_TURN_INTERVAL = settings.turnInterval;
    EGG_TURN_DURATION = settings.turnDurationSeconds > 0 && settings.turnDurationSeconds <= 10 ? settings.turnDurationSeconds * 1000 : 10000;
    angleAdjustment = settings.angleAdjustment;
    startTimestamp = settings.startTimestamp;
    restingAt45 = EEPROM.read(EEPROM_SERVO_REST) != 0;
    
    if (settings.pulseOnTime == 2000 || settings.pulseOnTime == 3000 || settings.pulseOnTime == 4000 || settings.pulseOnTime == 5000) {
      PULSE_ON_TIME = settings.pulseOnTime;
    } else {
      PULSE_ON_TIME = 3000;
    }
    
    uint32_t currentDay = getCurrentDay();
    stageLockdown = (currentDay >= 18);

    if (stageLockdown) {
      TARGET_TEMP = 37.5;
      TARGET_HUMIDITY = 65.0;
      servoEnabled = false;
    } else {
      TARGET_TEMP = 37.5;
      TARGET_HUMIDITY = 55.0;
      servoEnabled = true;
    }
    Serial.println("Settings loaded from EEPROM");
  } else {
    Serial.println("No saved settings found, using defaults");
    startTimestamp = 0;
    saveSettings();
  }
}

void addBootEntry(uint8_t bootId, uint32_t startUnix, uint32_t duration = 0) {
  for (int i = 0; i < bootTableCount; i++) {
    if (bootTable[i].bootId == bootId) {
      bootTable[i].startUnix = startUnix;
      if (duration > 0) bootTable[i].duration = duration;
      return;
    }
  }

  if (bootTableCount >= bootTableCapacity) {
    bootTableCapacity = (bootTableCapacity == 0) ? 8 : bootTableCapacity * 2;
    bootTable = (BootTimestamp*)realloc(bootTable, bootTableCapacity * sizeof(BootTimestamp));
  }

  bootTable[bootTableCount].bootId = bootId;
  bootTable[bootTableCount].startUnix = startUnix;
  bootTable[bootTableCount].duration = duration;
  bootTableCount++;
}

void sortBootTable() {
  for (int i = 0; i < bootTableCount - 1; i++) {
    for (int j = i + 1; j < bootTableCount; j++) {
      if (bootTable[i].bootId > bootTable[j].bootId) {
        BootTimestamp temp = bootTable[i];
        bootTable[i] = bootTable[j];
        bootTable[j] = temp;
      }
    }
  }
}

void prepareBootTable() {
  uint32_t lastKnownStartUnix = 0;
  uint8_t lastKnownBootId = 0;
  EEPROM.get(EEPROM_LAST_KNOWN_START_UNIX, lastKnownStartUnix);
  lastKnownBootId = EEPROM.read(EEPROM_LAST_KNOWN_BOOT_ID);

  for (int i = 0; i < bootIndexCount; i++) {
    uint32_t duration = getBootDuration(i);
    uint8_t bId = bootIndex[i].bootId;
    addBootEntry(bId, 0, duration);
  }

  sortBootTable();

  int anchorIdx = -1;
  for (int i = 0; i < bootTableCount; i++) {
    if (bootTable[i].bootId == lastKnownBootId) {
      bootTable[i].startUnix = lastKnownStartUnix;
      anchorIdx = i;
      break;
    }
  }

  if (anchorIdx != -1) {
    for (int i = anchorIdx - 1; i >= 0; i--) {
      bootTable[i].startUnix = bootTable[i+1].startUnix - bootTable[i].duration;
    }
    for (int i = anchorIdx + 1; i < bootTableCount; i++) {
      bootTable[i].startUnix = bootTable[i-1].startUnix + bootTable[i-1].duration;
    }
  }

  uint8_t currentBootId = EEPROM.read(EEPROM_BOOT_ID);
  uint32_t currentStartUnix = 0;
  if (bootTableCount > 0) {
    bool found = false;
    for(int i=0; i<bootTableCount; i++) {
        if(bootTable[i].bootId == currentBootId) {
            found = true;
            break;
        }
    }
    if (!found) {
        currentStartUnix = bootTable[bootTableCount - 1].startUnix + bootTable[bootTableCount - 1].duration;
    }
  } else {
    currentStartUnix = lastKnownStartUnix;
  }
  
  if (currentStartUnix > 0) {
    addBootEntry(currentBootId, currentStartUnix, 0);
    sortBootTable();
  }
}

// ============================================
// WEB SERVER HANDLERS
// ============================================
void handleRoot() {
  server.send(200, "text/html; charset=utf-8", INDEX_HTML);
}

void handleSettingsPage() {
  server.send(200, "text/html; charset=utf-8", SETTINGS_HTML);
}

void handleDexiePage() {
  server.send(200, "text/html; charset=utf-8", DEXIE_HTML);
}

void handleStatus() {
  uint32_t elapsed = getElapsedSeconds();
  uint32_t curDay = getCurrentDay();
  unsigned long uptimeSec = millis() / 1000;
  int days = uptimeSec / 86400;
  int hours = (uptimeSec % 86400) / 3600;
  int mins = (uptimeSec % 3600) / 60;
  int secs = uptimeSec % 60;
  String uptimeStr = "";
  if (days > 0) uptimeStr += String(days) + "d ";
  uptimeStr += String(hours) + "h " + String(mins) + "m " + String(secs) + "s";
  
  String json = "{\"temperature\":" + String(currentTemp) +
                ",\"humidity\":" + String(currentHumidity) +
                ",\"heater\":" + String(heaterState ? 1 : 0) +
                ",\"atomizer\":" + String(atomizerState ? 1 : 0) +
                ",\"fan\":" + String(fanState ? 1 : 0) +
                ",\"servo\":" + String(servoPosition) +
                ",\"version\":\"" + FIRMWARE_VERSION + "\"" +
                ",\"uptime\":\"" + uptimeStr + "\"" +
                ",\"mock\":" + String(useMockSensor ? 1 : 0) +
                ",\"autosim\":" + String(autoSimMode ? 1 : 0) +
                ",\"stageLockdown\":" + String(stageLockdown ? 1 : 0) +
                ",\"targetTemp\":" + String(TARGET_TEMP) +
                ",\"targetHum\":" + String(TARGET_HUMIDITY) +
                ",\"heapFree\":" + String(ESP.getFreeHeap()) +
                ",\"ip\":\"" + WiFi.localIP().toString() + "\"" +
                ",\"rssi\":" + String(WiFi.RSSI()) +
                ",\"uptimeSec\":" + String(uptimeSec) +
                ",\"bootId\":" + String(currentBootId) +
                ",\"currentSector\":" + String(currentSector) +
                ",\"startTimestamp\":" + String(startTimestamp) +
                ",\"elapsedSeconds\":" + String(elapsed) +
                ",\"currentDay\":" + String(curDay) +
                ",\"logsInCurrentBoot\":" + String(logsInCurrentBoot) + "}";

  server.send(200, "application/json", json);
}

void handleGetTimestamps() {
  uint8_t currentBId = EEPROM.read(EEPROM_BOOT_ID);
  uint32_t currentStartUnix = 0;
  for (int i = 0; i < bootTableCount; i++) {
    if (bootTable[i].bootId == currentBId) {
      currentStartUnix = bootTable[i].startUnix;
      break;
    }
  }

  String json = "{";
  json += "\"currentBootId\":" + String(currentBId);
  json += ",\"currentStartUnix\":" + String(currentStartUnix);
  json += ",\"bootUptimeSec\":" + String(millis() / 1000);
  json += ",\"bootTable\":[";
  for (int i = 0; i < bootTableCount; i++) {
    json += "{\"bootId\":" + String(bootTable[i].bootId);
    json += ",\"startUnix\":" + String(bootTable[i].startUnix);
    json += "}";
    if (i < bootTableCount - 1) json += ",";
  }
  json += "]}";
  server.send(200, "application/json", json);
}

void handlePutTimestamps() {
  if (server.hasArg("plain") == false) {
    server.send(400, "text/plain", "Body not found");
    return;
  }

  String body = server.arg("plain");
  
  int entriesCount = 0;
  int pos = 0;
  while ((pos = body.indexOf("{\"bootId\":", pos)) != -1) {
    pos += 10;
    int endBootId = body.indexOf(",", pos);
    uint8_t bId = body.substring(pos, endBootId).toInt();
    
    int startUnixPos = body.indexOf("\"startUnix\":", endBootId);
    startUnixPos += 12;
    int endStartUnix = body.indexOf("}", startUnixPos);
    if (endStartUnix == -1) endStartUnix = body.indexOf(",", startUnixPos);
    if (endStartUnix == -1) break;
    uint32_t sUnix = body.substring(startUnixPos, endStartUnix).toInt();
    
    addBootEntry(bId, sUnix);
    entriesCount++;
    pos = endStartUnix;
  }

  sortBootTable();

  uint8_t currentBId = EEPROM.read(EEPROM_BOOT_ID);
  uint32_t currentStartUnix = 0;
  for (int i = 0; i < bootTableCount; i++) {
    if (bootTable[i].bootId == currentBId) {
      currentStartUnix = bootTable[i].startUnix;
      break;
    }
  }

  bool eepromUpdated = false;
  int32_t drift = 0;
  if (currentStartUnix > 0) {
    uint32_t lastKnownStartUnix;
    EEPROM.get(EEPROM_LAST_KNOWN_START_UNIX, lastKnownStartUnix);
    uint8_t lastKnownBootId = EEPROM.read(EEPROM_LAST_KNOWN_BOOT_ID);
    
    if (lastKnownBootId == currentBId) {
        drift = (int32_t)currentStartUnix - (int32_t)lastKnownStartUnix;
    }
    
    if (abs(drift) > 5 || lastKnownBootId != currentBId) {
      EEPROM.write(EEPROM_LAST_KNOWN_BOOT_ID, currentBId);
      EEPROM.put(EEPROM_LAST_KNOWN_START_UNIX, currentStartUnix);
      EEPROM.commit();
      eepromUpdated = true;
    }
  }

  String json = "{\"synced\":true";
  json += ",\"entriesStored\":" + String(entriesCount);
  json += ",\"driftCalculated\":" + String(drift);
  json += ",\"eepromUpdated\":" + String(eepromUpdated ? "true" : "false");
  json += ",\"currentBootId\":" + String(currentBId);
  json += ",\"currentStartUnix\":" + String(currentStartUnix);
  json += "}";
  server.send(200, "application/json", json);
}

void handleData() {
  unsigned long uptimeSec = millis() / 1000;
  int days = uptimeSec / 86400;
  int hours = (uptimeSec % 86400) / 3600;
  int mins = (uptimeSec % 3600) / 60;
  int secs = uptimeSec % 60;
  String uptimeStr = "";
  if (days > 0) uptimeStr += String(days) + "d ";
  uptimeStr += String(hours) + "h " + String(mins) + "m " + String(secs) + "s";
  
  String json = "{\"temperature\":" + String(currentTemp) +
                ",\"humidity\":" + String(currentHumidity) +
                ",\"heater\":" + String(heaterState ? 1 : 0) +
                ",\"atomizer\":" + String(atomizerState ? 1 : 0) +
                ",\"fan\":" + String(fanState ? 1 : 0) +
                ",\"servo\":" + String(servoPosition) +
                ",\"version\":\"" + FIRMWARE_VERSION + "\"" +
                ",\"uptime\":\"" + uptimeStr + "\"" +
                ",\"mock\":" + String(useMockSensor ? 1 : 0) +
                ",\"autosim\":" + String(autoSimMode ? 1 : 0) +
                ",\"stageLockdown\":" + String(stageLockdown ? 1 : 0) +
                ",\"heaterMode\":" + String(heaterMode) +
                ",\"atomizerMode\":" + String(atomizerMode) +
                ",\"fanMode\":" + String(fanMode) +
                ",\"servoMode\":" + String(servoMode) +
                 ",\"totalLogs\":" + String(getTotalLogs()) +
                ",\"targetTemp\":" + String(TARGET_TEMP) +
                ",\"targetHum\":" + String(TARGET_HUMIDITY) +
                ",\"heapFree\":" + String(ESP.getFreeHeap()) +
                ",\"ip\":\"" + WiFi.localIP().toString() + "\"" +
                ",\"rssi\":" + String(WiFi.RSSI()) +
                ",\"millis\":" + String(millis());

  // Parse pagination params
  uint8_t sinceBootId = 0;
  if (server.hasArg("boot")) {
    sinceBootId = (uint8_t)server.arg("boot").toInt();
  }
  uint32_t sinceTimeSec = 0;
  if (server.hasArg("time")) {
    sinceTimeSec = (uint32_t)server.arg("time").toInt();
  }
  
  int count = 200;
  if (server.hasArg("count")) {
    count = server.arg("count").toInt();
    if (count > 200) count = 200;
    if (count < 1) count = 1;
  }
  
  String logHex = "";
  int sentCount = getLogHex(logHex, count, sinceBootId, sinceTimeSec);
  
  json += ",\"totalLogs\":" + String(getTotalLogs()) +
         ",\"sentCount\":" + String(sentCount) +
         ",\"logs\":\"" + logHex + "\"}";
  
  server.send(200, "application/json", json);
}

void handleControl() {
  if (server.hasArg("device") && server.hasArg("mode")) {
    String device = server.arg("device");
    String mode = server.arg("mode");
    bool isKillOff = (mode == "off");
    
    if (device == "heater") {
      heaterMode = isKillOff ? KILL_OFF : AUTO;
      if (isKillOff) {
        heaterState = false;
        digitalWrite(RELAY_HEATER, LOW);
      }
    } else if (device == "atomizer") {
      atomizerMode = isKillOff ? KILL_OFF : AUTO;
      if (isKillOff) {
        atomizerState = false;
        digitalWrite(RELAY_ATOMIZER, LOW);
        atomizerPulsing = false;
        atomizerInOffPhase = false;
      }
    } else if (device == "fan") {
      fanMode = isKillOff ? KILL_OFF : AUTO;
      if (isKillOff) {
        fanState = false;
        digitalWrite(RELAY_FAN, LOW);
      }
    } else if (device == "servo") {
      if (mode == "left") {
        servoEnabled = true;
        restingAt45 = true;
        myServo.attach(SERVO_PIN, 544, 2450);
        myServo.write(0);
        delay(500);
        myServo.detach();
        server.send(200, "text/plain", "Servo moved to left (0)");
      } else if (mode == "right") {
        servoEnabled = true;
        restingAt45 = false;
        myServo.attach(SERVO_PIN, 544, 2450);
        myServo.write(180);
        delay(500);
        myServo.detach();
        server.send(200, "text/plain", "Servo moved to right (180)");
      } else if (mode == "center") {
        servoEnabled = true;
        myServo.attach(SERVO_PIN, 544, 2450);
        myServo.write(90);
        delay(500);
        myServo.detach();
        server.send(200, "text/plain", "Servo moved to center (90)");
      } else {
        servoMode = isKillOff ? KILL_OFF : AUTO;
        servoEnabled = !isKillOff;
        if (isKillOff) {
          servoPosition = 0;
          myServo.attach(SERVO_PIN, 544, 2450);
          myServo.write(90);
          delay(500);
          myServo.detach();
        }
        server.send(200, "text/plain", device + " mode set to " + (isKillOff ? "OFF" : "AUTO"));
      }
    }
  } else {
    server.send(200, "text/plain", "Invalid request");
  }
}

void handleOtaCheck() {
  HTTPClient http;
  WiFiClient client;
  http.begin(client, VERSION_URL);
  int httpCode = http.GET();
  
  if (httpCode == 200) {
    String remoteVersion = http.getString();
    remoteVersion.trim();
    bool hasUpdate = (remoteVersion != FIRMWARE_VERSION);
    String json = "{\"update\":" + String(hasUpdate ? "true" : "false") + ",\"version\":\"" + remoteVersion + "\"}";
    server.send(200, "application/json", json);
  } else {
    server.send(200, "application/json", "{\"update\":false,\"version\":\"error\"}");
  }
  http.end();
}

void handleOtaUpdate() {
  performUpdate();
}

void handleSettingsApi() {
  if (server.hasArg("autosim")) {
    bool enable = (server.arg("autosim") == "1");
    setAutoSim(enable);
    server.send(200, "text/plain", enable ? "Auto simulation enabled" : "Auto simulation disabled");
  } else if (server.hasArg("enable")) {
    bool enable = (server.arg("enable") == "1");
    if (enable) setAutoSim(false);
    setMockSensor(enable);
    server.send(200, "text/plain", enable ? "Mock sensor enabled" : "Mock sensor disabled");
  } else if (server.hasArg("temp") && server.hasArg("hum")) {
    float t = server.arg("temp").toFloat();
    float h = server.arg("hum").toFloat();
    setAutoSim(false);
    setMockSensor(true);
    setMockValues(t, h);
    server.send(200, "text/plain", "Mock values set: " + String(t) + "C, " + String(h) + "%");
  } else if (server.hasArg("logInterval")) {
    unsigned long val = server.arg("logInterval").toInt();
    LOG_INTERVAL = val;
    saveSettings();
    server.send(200, "text/plain", "Log interval set to " + String(val/1000) + "s");
  } else if (server.hasArg("eggTurnInterval")) {
    unsigned long val = server.arg("eggTurnInterval").toInt();
    EGG_TURN_INTERVAL = val;
    if (val != 20000) {  // Don't save 20sec to EEPROM (only for mock/auto mode testing)
      saveSettings();
    }
    server.send(200, "text/plain", "Egg turner interval set to " + String(val/3600000) + " hours");
  } else if (server.hasArg("eggTurnDuration")) {
    uint8_t val = server.arg("eggTurnDuration").toInt();
    if (val > 0 && val <= 10) {
      EGG_TURN_DURATION = val * 1000;
      saveSettings();
      server.send(200, "text/plain", "Egg sweep duration set to " + String(val) + "s");
    } else {
      server.send(400, "text/plain", "Invalid duration (must be 1-10s)");
    }
  } else if (server.hasArg("angleAdjustment")) {
    int8_t val = server.arg("angleAdjustment").toInt();
    if (val >= -40 && val <= 40) {
      // Start smooth 3-second transition with cubic easing
      angleAdjustFrom = angleAdjustment;
      angleAdjustTo = val;
      angleAdjustStartTime = millis();
      adjustingAngle = true;
      myServo.attach(SERVO_PIN, 544, 2450);
      angleAdjustment = val;
      saveSettings();
      server.send(200, "text/plain", "Angle adjustment set to " + String(val));
    } else {
      server.send(400, "text/plain", "Invalid adjustment (must be -40 to 40)");
    }
  } else if (server.hasArg("pulseOnTime")) {
    unsigned long val = server.arg("pulseOnTime").toInt();
    PULSE_ON_TIME = val;
    saveSettings();
    server.send(200, "text/plain", "Atomizer pulse on time set to " + String(val/1000) + "s");
  } else if (server.hasArg("action")) {
    String action = server.arg("action");
    if (action == "newBatch" && server.hasArg("timestamp")) {
      uint32_t browserNow = (uint32_t)server.arg("timestamp").toInt();
      uint32_t currentUptime = millis() / 1000;
      uint32_t currentStartUnix = browserNow - currentUptime;
      
      startTimestamp = browserNow;
      uint8_t currentBId = EEPROM.read(EEPROM_BOOT_ID);
      addBootEntry(currentBId, currentStartUnix, 0);
      
      EEPROM.write(EEPROM_LAST_KNOWN_BOOT_ID, currentBId);
      EEPROM.put(EEPROM_LAST_KNOWN_START_UNIX, currentStartUnix);
      EEPROM.commit();

      restingAt45 = true;
      EEPROM.write(EEPROM_SERVO_REST, 1);  // Save as left (true = 1)
      EEPROM.commit();
      myServo.attach(SERVO_PIN, 544, 2450);
      myServo.write(90);
      delay(500);
      myServo.detach();
      saveSettings();
      server.send(200, "text/plain", "New batch started");
    } else if (action == "syncTime" && server.hasArg("timestamp")) {
      // syncTime is largely superseded by /timestamps PUT but kept for simple syncs
      uint32_t currentUnix = (uint32_t)server.arg("timestamp").toInt();
      uint8_t currentBId = EEPROM.read(EEPROM_BOOT_ID);
      uint32_t currentUptime = millis() / 1000;
      uint32_t browserCalculatedStartUnix = currentUnix - currentUptime;
      
      addBootEntry(currentBId, browserCalculatedStartUnix, 0);
      
      EEPROM.write(EEPROM_LAST_KNOWN_BOOT_ID, currentBId);
      EEPROM.put(EEPROM_LAST_KNOWN_START_UNIX, browserCalculatedStartUnix);
      EEPROM.commit();
      
      server.send(200, "text/plain", "Time synced via simple API");
    } else if (action == "adjustDay" && server.hasArg("dir")) {
      int dir = server.arg("dir").toInt();
      if (dir == 1) {
        if (startTimestamp >= 86400) startTimestamp -= 86400;
      } else if (dir == -1) {
        startTimestamp += 86400;
      }
      saveSettings();
      server.send(200, "text/plain", "Day adjusted");
    } else {
      server.send(400, "text/plain", "Invalid action");
    }
  } else {
    uint32_t curDay = getCurrentDay();
    uint32_t elapsed = getElapsedSeconds();
    String json = "{\"enabled\":" + String(useMockSensor ? "true" : "false") + 
                  ",\"autosim\":" + String(autoSimMode ? "true" : "false") +
                  ",\"temp\":" + String(mockTemp) + 
                  ",\"hum\":" + String(mockHum) +
                  ",\"logInterval\":" + String(LOG_INTERVAL) +
                  ",\"eggTurnInterval\":" + String(EGG_TURN_INTERVAL) +
                  ",\"eggTurnDuration\":" + String(EGG_TURN_DURATION / 1000) +
                  ",\"angleAdjustment\":" + String(angleAdjustment) +
                  ",\"pulseOnTime\":" + String(PULSE_ON_TIME) +
                  ",\"startTimestamp\":" + String(startTimestamp) +
                  ",\"elapsedSeconds\":" + String(elapsed) +
                  ",\"currentDay\":" + String(curDay) +
                  ",\"stageLockdown\":" + String(stageLockdown ? "true" : "false") + "}";
    server.send(200, "application/json", json);
  }
}

// ============================================
// AUTO CONTROL LOGIC
// ============================================
void autoControl() {
  if (!isnan(currentTemp) && !isnan(currentHumidity)) {
    unsigned long now = millis();
    
    // Heater Control
    if (heaterMode == AUTO) {
      if (currentTemp < TARGET_TEMP - TEMP_HYSTERESIS) {
        heaterState = true;
      } else if (currentTemp >= TARGET_TEMP) {
        heaterState = false;
      }
      digitalWrite(RELAY_HEATER, heaterState ? HIGH : LOW);
      
      if (heaterState != heaterWasOn) {
        heaterLastChanged = now;
        heaterWasOn = heaterState;
      }
    } else {
      if (heaterState) {
        heaterState = false;
        digitalWrite(RELAY_HEATER, LOW);
        heaterWasOn = false;
      }
    }
    
    // Atomizer Control
    if (atomizerMode == AUTO) {
      unsigned long effectivePulseOn = PULSE_ON_TIME;
      unsigned long effectivePulseOff = PULSE_OFF_TIME;
      float humDelta = TARGET_HUMIDITY - currentHumidity;

      if (humDelta >= 25.0) {
        effectivePulseOn = PULSE_ON_TIME * 2;
        effectivePulseOff = PULSE_OFF_TIME / 2;
      } else if (humDelta >= 10.0) {
        effectivePulseOn = PULSE_ON_TIME * 2;
      }

      if (currentHumidity < TARGET_HUMIDITY - HUMIDITY_HYSTERESIS) {
        if (!atomizerPulsing && !atomizerInOffPhase) {
          atomizerState = true;
          digitalWrite(RELAY_ATOMIZER, HIGH);
          atomizerPulseStart = millis();
          atomizerPulsing = true;
        }
      } else if (currentHumidity >= TARGET_HUMIDITY) {
        atomizerState = false;
        digitalWrite(RELAY_ATOMIZER, LOW);
        atomizerPulsing = false;
        atomizerInOffPhase = false;
      }

      if (atomizerPulsing && (millis() - atomizerPulseStart >= effectivePulseOn)) {
        atomizerState = false;
        digitalWrite(RELAY_ATOMIZER, LOW);
        atomizerPulsing = false;
        atomizerInOffPhase = true;
        atomizerOffStart = millis();
      } else if (atomizerInOffPhase && (millis() - atomizerOffStart >= effectivePulseOff)) {
        atomizerInOffPhase = false;
      }
      
      if (atomizerState != atomizerWasOn) {
        atomizerLastChanged = now;
        atomizerWasOn = atomizerState;
      }
    } else {
      if (atomizerState) {
        atomizerState = false;
        digitalWrite(RELAY_ATOMIZER, LOW);
        atomizerWasOn = false;
        atomizerPulsing = false;
        atomizerInOffPhase = false;
      }
    }
    
    // Fan Control
    if (fanMode == AUTO) {
      bool withinHeaterWindow = (!heaterState && (now - heaterLastChanged < FAN_EXTEND_TIME));
      bool withinAtomizerWindow = (!atomizerState && (now - atomizerLastChanged < FAN_EXTEND_TIME));
      
      if (heaterState || withinHeaterWindow || atomizerState || withinAtomizerWindow || 
          currentTemp > MAX_SAFE_TEMP) {
        fanState = true;
      } else {
        fanState = false;
      }
      digitalWrite(RELAY_FAN, fanState ? HIGH : LOW);
    } else {
      if (fanState) {
        fanState = false;
        digitalWrite(RELAY_FAN, LOW);
      }
    }
  }
}

// ============================================
// SERVO PWM (Using ServoSmooth)
// ============================================

void servoInit() {
  // Base: 45 (left) / 135 (right), angleAdjustment expands/reduces range
  int initialAngle = restingAt45 ? (45 - angleAdjustment) : (135 + angleAdjustment);
  initialAngle = constrain(initialAngle, 0, 180);
  
  myServo.attach(SERVO_PIN, 544, 2450);
  myServo.write(initialAngle);
  delay(500);
  myServo.detach();
}

// ============================================
// EGG TURNER - Manual smooth movement with standard Servo
// ============================================
void rotateEggs() {
  static bool turning = false;
  static unsigned long turnStartTime = 0;
  static int startAngle = 0;
  static int targetAngle = 180;
  
  // Disable egg turner during lockdown stage
  if (stageLockdown) {
    servoEnabled = false;
    servoPosition = 0;
    return;
  }
  
  // Trigger turn if interval elapsed
  if (!turning && (millis() - lastServoTurn > EGG_TURN_INTERVAL)) {
    turning = true;
    turnStartTime = millis();
    
    // Set start and target angles based on 45/135 base with angleAdjustment
    startAngle = restingAt45 ? (45 - angleAdjustment) : (135 + angleAdjustment);
    targetAngle = restingAt45 ? (135 + angleAdjustment) : (45 - angleAdjustment);
    
    // Attach and move to start position
    myServo.attach(SERVO_PIN, 544, 2450);
    myServo.write(startAngle);
    
    // Save NEXT resting position to EEPROM
    EEPROM.write(EEPROM_SERVO_REST, restingAt45 ? 0 : 1);
    EEPROM.commit();
    
    // Flip restingAt45 for next turn
    restingAt45 = !restingAt45;
    
    lastServoTurn = millis();
    Serial.println("Egg turn started from " + String(startAngle) + " to " + String(targetAngle));
  }
  
  if (turning) {
    servoPosition = restingAt45 ? 2 : 1;
    unsigned long elapsed = millis() - turnStartTime;
    
    if (elapsed >= EGG_TURN_DURATION) {
      // Turn finished - move to target and detach
      myServo.write(targetAngle);
      delay(100);
      myServo.detach();
      
      turning = false;
      servoPosition = restingAt45 ? 1 : 2;
      lastServoTurn = millis();
      Serial.println("Egg turn completed");
    } else {
      // Smooth slow movement over 10 seconds using linear interpolation
      float progress = (float)elapsed / (float)EGG_TURN_DURATION;
      
      // Cubic ease-in-out: elevator-like - start very slow, fast middle, end very slow
      float easedProgress = progress < 0.5 ? 4 * progress * progress * progress : 1 - pow(-2 * progress + 2, 3) / 2;
      
      int currentAngle = startAngle + (targetAngle - startAngle) * easedProgress;
      myServo.write(currentAngle);
    }
  } else {
    servoPosition = restingAt45 ? 1 : 2;
  }
  
  if (!servoEnabled || servoMode == KILL_OFF) {
    servoPosition = 0;
  }
}

// Rollback endpoints
void handleReboot() {
  server.send(200, "text/plain", "Rebooting...");
  delay(500);
  ESP.restart();
}

void handleRollback() {
  // Trigger rollback - restore previous firmware if available
  Serial.println("Rollback triggered - resetting boot counter");
  EEPROM.write(EEPROM_BOOT_COUNT, MAX_BOOT_FAILURES);
  EEPROM.commit();
  server.send(200, "text/plain", "Rollback: please reflash to recover");
}

void handleRecovery() {
  // Enter recovery mode - reset EEPROM and await reflash
  EEPROM.write(EEPROM_BOOT_OK, 0);
  EEPROM.write(EEPROM_BOOT_COUNT, 0);
  EEPROM.commit();
  server.send(200, "text/plain", "Recovery mode - please reflash");
}

void handleRecoveryReset() {
  // Reset recovery mode - allow normal boot
  EEPROM.write(EEPROM_BOOT_OK, BOOT_OK_MAGIC);
  EEPROM.write(EEPROM_BOOT_COUNT, 0);
  EEPROM.commit();
  server.send(200, "text/plain", "Recovery reset - normal boot enabled");
  delay(500);
  ESP.restart();
}

void handleClearFlash() {
  uint16_t startSector = currentSector;
  
  // Erase from sector 0 up to currentSector
  for (uint16_t i = 0; i <= currentSector; i++) {
    ESP.flashEraseSector((FLASH_LOG_START + (i * LOG_SECTOR_SIZE)) / LOG_SECTOR_SIZE);
    ESP.wdtFeed();
  }
  
  currentSector = 0;
  currentOffset = 0;
  logsInCurrentBoot = 0;
  
  EEPROM.put(EEPROM_CURRENT_SECTOR, currentSector);
  EEPROM.commit();
  
  server.send(200, "text/plain", "Flash cleared up to sector " + String(startSector) + ". Pointers reset to 0.");
}

// ============================================
// MAIN SETUP
// ============================================
void setup() {
  Serial.begin(115200);
  
  pinMode(RELAY_HEATER, OUTPUT);
  pinMode(RELAY_ATOMIZER, OUTPUT);
  pinMode(RELAY_FAN, OUTPUT);
  digitalWrite(RELAY_HEATER, LOW);
  digitalWrite(RELAY_ATOMIZER, LOW);
  digitalWrite(RELAY_FAN, LOW);

  initDHT();

  initRecovery();
  
  uint8_t bootId = EEPROM.read(EEPROM_BOOT_ID);
  bootId++;
  EEPROM.write(EEPROM_BOOT_ID, bootId);
  EEPROM.commit();

  initLogging(bootId);
  prepareBootTable();
  loadSettings();
  
  servoInit();
  
  connectWiFi();

  // Start mDNS responder for EGGubator.local
  if (MDNS.begin("EGGubator")) {
    Serial.println("mDNS responder started: EGGubator.local");
  }

  markBootSuccess();

  Serial.print("Firmware: ");
  Serial.println(FIRMWARE_VERSION);

// Setup web server
  server.on("/", handleRoot);
  server.on("/settings", handleSettingsPage);
  server.on("/dexie", handleDexiePage);
  server.on("/status", handleStatus);
  server.on("/timestamps", HTTP_GET, handleGetTimestamps);
  server.on("/timestamps", HTTP_PUT, handlePutTimestamps);
  server.on("/data", handleData);
  server.on("/control", handleControl);
  server.on("/settings/clear", handleClearFlash);
  server.on("/ota/check", handleOtaCheck);
  server.on("/ota/update", handleOtaUpdate);
  server.on("/settings/api", handleSettingsApi);
  server.on("/reboot", handleReboot);
  server.on("/rollback", handleRollback);
  server.on("/recovery", handleRecovery);
  server.on("/recovery/reset", handleRecoveryReset);
  httpUpdater.setup(&server);
  server.begin();

  Serial.println("HTTP server started");
}

// ============================================
// MAIN LOOP
// ============================================
void loop() {
  server.handleClient();
  MDNS.update();

  unsigned long currentMillis = millis();
  
  if (currentMillis - lastElapsedMillis >= 1000) {
    lastElapsedMillis = currentMillis;
    
    uint32_t curDay = getCurrentDay();
    bool newStageLockdown = (curDay >= 18);
    if (newStageLockdown != stageLockdown) {
      stageLockdown = newStageLockdown;
      if (stageLockdown) {
        TARGET_TEMP = 37.5;
        TARGET_HUMIDITY = 65.0;
        servoEnabled = false;
      } else {
        TARGET_TEMP = 37.5;
        TARGET_HUMIDITY = 55.0;
        servoEnabled = true;
      }
      saveSettings();
    }
  }

  if (currentMillis - lastReadTime > 2000) {
    if (autoSimMode) {
      updateAutoSim(heaterState, atomizerState, fanState);
    }
    float t = readDHT22();
    float h = readHumidity();

      if (!isnan(t) && !isnan(h) && t > 0 && h > 0) {
        currentTemp = t;
        currentHumidity = h;
        autoControl();
        logData(currentTemp, currentHumidity, heaterState, atomizerState, fanState, servoPosition, LOG_INTERVAL);
      }
    lastReadTime = millis();
  }

  if (servoEnabled) {
    rotateEggs();
  }

  // Smooth angle adjustment transition with cubic easing (3 seconds)
  if (adjustingAngle) {
    unsigned long elapsed = millis() - angleAdjustStartTime;
    const unsigned long ANGLE_ADJUST_DURATION = 3000;
    
    if (elapsed >= ANGLE_ADJUST_DURATION) {
      // Transition complete
      int targetAngle = restingAt45 ? (45 - angleAdjustment) : (135 + angleAdjustment);
      targetAngle = constrain(targetAngle, 0, 180);
      myServo.write(targetAngle);
      delay(50);
      myServo.detach();
      adjustingAngle = false;
      Serial.println("Angle adjustment transition complete");
    } else {
      // Cubic ease-in-out
      float progress = (float)elapsed / (float)ANGLE_ADJUST_DURATION;
      float easedProgress = progress < 0.5 ? 4 * progress * progress * progress : 1 - pow(-2 * progress + 2, 3) / 2;
      
      // Interpolate angle adjustment value
      int8_t currentAngleAdj = angleAdjustFrom + (angleAdjustTo - angleAdjustFrom) * easedProgress;
      
      // Calculate and apply servo angle
      int currentServoAngle = restingAt45 ? (45 - currentAngleAdj) : (135 + currentAngleAdj);
      currentServoAngle = constrain(currentServoAngle, 0, 180);
      myServo.write(currentServoAngle);
    }
  }

  if (millis() - lastOtaCheck > 3600000) {
    checkAndUpdateAuto();
    lastOtaCheck = millis();
  }
}
