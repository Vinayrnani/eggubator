// ============================================
// EGG INCUBATOR CONTROLLER - Modular Version
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
#include "sat_manager.h"

extern bool useMockSensor;
extern bool autoSimMode;
extern float mockTemp;
extern float mockHum;
extern void updateAutoSim(bool heater, bool atomizer, bool fan);


#define KILL_OFF 0
#define AUTO 1

// Configurable timing (can be changed via web)
unsigned long LOG_INTERVAL = 90000;
unsigned long EGG_TURN_INTERVAL = 7200000;
unsigned long EGG_TURN_DURATION = 2000; // ms per step (each step = 6°)
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
int servoPosition = 0; // Current servo step (0-31, each step = 6°)
int heaterMode = AUTO;
bool stageLockdown = false;  // false = incubation (1-18), true = lockdown (19-21)
int atomizerMode = AUTO;
int fanMode = AUTO;
int servoMode = AUTO;
unsigned long lastReadTime = 0;
unsigned long lastOtaCheck = 0;
unsigned long lastServoTurn = 0;
uint8_t currentServoStep = 7; // Step 7 = 42° (min angle)
bool sweeping = false;
bool isMovingTowardsMax = true;
uint8_t sweepTargetStep = 22; // Target step during sweep
unsigned long lastStepTime = 0;
bool movingInStep = false;
int stepStartAngle = 0;
int stepTargetAngle = 0;
unsigned long stepMoveStart = 0;
int8_t angleAdjustment = 0;

Servo myServo;

uint32_t startTimestamp = 0;

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

// ============================================
// AUTO CONTROL LOGIC
// ============================================

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
  }
}

void loadSettings() {
  DeviceSettings settings;
  EEPROM.get(EEPROM_SETTINGS_MAGIC, settings);
  
  if (settings.magic == SETTINGS_MAGIC_VAL) {
    stageLockdown = settings.stageLockdown;
    LOG_INTERVAL = settings.logInterval;
    EGG_TURN_INTERVAL = settings.turnInterval;
    EGG_TURN_DURATION = settings.turnDurationSeconds > 0 && settings.turnDurationSeconds <= 10 ? settings.turnDurationSeconds * 1000 : 2000;
    angleAdjustment = settings.angleAdjustment;
    startTimestamp = settings.startTimestamp;
    
    if (settings.pulseOnTime == 2000 || settings.pulseOnTime == 3000 || settings.pulseOnTime == 4000 || settings.pulseOnTime == 5000) {
      PULSE_ON_TIME = settings.pulseOnTime;
    } else {
      PULSE_ON_TIME = 3000;
    }
    
    stageLockdown = (getCurrentDay() >= 18);

    if (stageLockdown) {
      TARGET_TEMP = 37.5;
      TARGET_HUMIDITY = 65.0;
      servoEnabled = false;
    } else {
      TARGET_TEMP = 37.5;
      TARGET_HUMIDITY = 55.0;
      servoEnabled = true;
    }
  } else {
    startTimestamp = 0;
    saveSettings();
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
                ",\"startSector\":" + String(startSector) +
                ",\"startTimestamp\":" + String(startTimestamp) +
                ",\"elapsedSeconds\":" + String(getElapsedSeconds()) +
                ",\"currentDay\":" + String(getCurrentDay()) +
                ",\"logsInCurrentBoot\":" + String(logsInCurrentBoot) + "}";

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
                ",\"targetTemp\":" + String(TARGET_TEMP) +
                ",\"targetHum\":" + String(TARGET_HUMIDITY) +
                ",\"heapFree\":" + String(ESP.getFreeHeap()) +
                ",\"ip\":\"" + WiFi.localIP().toString() + "\"" +
                ",\"rssi\":" + String(WiFi.RSSI()) +
                ",\"uptimeSec\":" + String(uptimeSec) +
                ",\"bootId\":" + String(currentBootId) +
                ",\"currentSector\":" + String(currentSector) +
                ",\"startTimestamp\":" + String(startTimestamp) +
                ",\"elapsedSeconds\":" + String(getElapsedSeconds()) +
                ",\"currentDay\":" + String(getCurrentDay()) +
                ",\"logsInCurrentBoot\":" + String(logsInCurrentBoot) +
                ",\"bootStartUnix\":" + String(startTimestamp + getElapsedSeconds() - uptimeSec);

json += ",\"totalLogs\":" + String(getTotalLogs());

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
  
  json += ",\"sentCount\":" + String(sentCount) +
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
        int8_t adjSteps = angleAdjustment / 6;
        currentServoStep = constrain(7 - adjSteps, 0, 31);
        int angle = constrain(currentServoStep * 6, 0, 180);
        int pulseWidth = map(angle, 0, 180, 544, 2450);
        myServo.attach(SERVO_PIN, 544, 2450, pulseWidth);
        myServo.write(angle);
        myServo.detach();
        sweeping = false;
        server.send(200, "text/plain", "Servo moved to left (" + String(angle) + "°)");
      } else if (mode == "right") {
        servoEnabled = true;
        int8_t adjSteps = angleAdjustment / 6;
        currentServoStep = constrain(22 + adjSteps, 0, 31);
        int angle = constrain(currentServoStep * 6, 0, 180);
        int pulseWidth = map(angle, 0, 180, 544, 2450);
        myServo.attach(SERVO_PIN, 544, 2450, pulseWidth);
        myServo.write(angle);
        myServo.detach();
        sweeping = false;
        server.send(200, "text/plain", "Servo moved to right (" + String(angle) + "°)");
      } else {
        servoMode = isKillOff ? KILL_OFF : AUTO;
        servoEnabled = !isKillOff;
        if (isKillOff) {
          servoPosition = 0;
          int angle = constrain(currentServoStep * 6, 0, 180);
          int pulseWidth = map(angle, 0, 180, 544, 2450);
          myServo.attach(SERVO_PIN, 544, 2450, pulseWidth);
          myServo.write(angle);
          myServo.detach();
          sweeping = false;
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
    if (val >= -42 && val <= 42) {
      angleAdjustment = val;
      saveSettings();
      server.send(200, "text/plain", "Angle adjustment set to " + String(val));
    } else {
      server.send(400, "text/plain", "Invalid adjustment (must be -42 to 42)");
    }
  } else if (server.hasArg("pulseOnTime")) {
    unsigned long val = server.arg("pulseOnTime").toInt();
    PULSE_ON_TIME = val;
    saveSettings();
    server.send(200, "text/plain", "Atomizer pulse on time set to " + String(val/1000) + "s");
  } else if (server.hasArg("action")) {
    String action = server.arg("action");
    if (action == "newBatch" && server.hasArg("timestamp")) {
      startTimestamp = (uint32_t)server.arg("timestamp").toInt();
      clearLogs();
      
      // Smooth 3-second continuous sweep to minAngle
      int8_t adjSteps = angleAdjustment / 6;
      uint8_t targetStep = constrain(7 - adjSteps, 0, 31);
      int targetAngle = targetStep * 6;
      int startAngle = currentServoStep * 6;
      
      myServo.attach(SERVO_PIN, 544, 2450);
      const unsigned long SWEEP_DURATION = 3000;
      unsigned long sweepStart = millis();
      while (millis() - sweepStart < SWEEP_DURATION) {
        float progress = (float)(millis() - sweepStart) / SWEEP_DURATION;
        int angle = startAngle + (targetAngle - startAngle) * progress;
        angle = constrain(angle, 0, 180);
        myServo.write(angle);
        delay(10);
        ESP.wdtFeed();
      }
      myServo.write(targetAngle);
      delay(50);
      myServo.detach();
      
      currentServoStep = targetStep;
      sweeping = false;
      saveSettings();
      server.send(200, "text/plain", "New batch started");
    } else if (action == "syncTime" && server.hasArg("timestamp")) {
      uint32_t currentUnix = (uint32_t)server.arg("timestamp").toInt();
      if (currentUnix > 0) {
        uint32_t bootUptime = getBootUptime();
        uint32_t browserCalculatedStartUnix = currentUnix - bootUptime;
        for (int i = 0; i < bootSessionCount; i++) {
          if (bootSessions[i].bootId == currentBootId) {
            int32_t drift = abs((int32_t)(browserCalculatedStartUnix - bootSessions[i].startUnix));
            if (drift > 5) {
              bootSessions[i].startUnix = browserCalculatedStartUnix;
              EEPROM.put(EEPROM_LAST_KNOWN_START_UNIX, browserCalculatedStartUnix);
              EEPROM.write(EEPROM_LAST_KNOWN_BOOT_ID, currentBootId);
              EEPROM.commit();
              server.send(200, "text/plain", "Time synced with drift=" + String(drift));
              return;
            }
            break;
          }
        }
      }
      server.send(200, "text/plain", "No sync needed");
    } else if (action == "adjustDay" && server.hasArg("dir")) {
      int dir = server.arg("dir").toInt();
      uint32_t currentElapsed = getElapsedSeconds();
      if (dir == 1) {
        if (startTimestamp >= 86400) startTimestamp -= 86400;
      } else if (dir == -1 && currentElapsed >= 86400) {
        startTimestamp += 86400;
      }
      saveSettings();
      server.send(200, "text/plain", "Day adjusted");
    } else {
      server.send(400, "text/plain", "Invalid action");
    }
  } else {
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
                  ",\"elapsedSeconds\":" + String(getElapsedSeconds()) +
                  ",\"currentDay\":" + String(getCurrentDay()) +
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
  int angle = constrain(currentServoStep * 6, 0, 180);
  myServo.attach(SERVO_PIN, 544, 2450, angle);  // <200 = angle mode in Servo::write()
  // No detach — servo holds position until first sweep
}

// ============================================
// EGG TURNER - Smooth step-based servo sweep
// ============================================
void rotateEggs() {
  // Disable egg turner during lockdown stage
  if (stageLockdown) {
    servoEnabled = false;
    servoPosition = 0;
    return;
  }

  // Calculate step endpoints: base 42° (step 7) to 132° (step 22) ± angleAdjustment
  int8_t adjSteps = angleAdjustment / 6;
  uint8_t minStep = constrain(7 - adjSteps, 0, 31);
  uint8_t maxStep = constrain(22 + adjSteps, 0, 31);

  // Start turning if interval elapsed
  if (!sweeping && !movingInStep && (millis() - lastServoTurn > EGG_TURN_INTERVAL)) {
    sweeping = true;
    lastStepTime = millis();
    lastServoTurn = millis();
    
    // Target the endpoint based on current direction
    if (isMovingTowardsMax) {
      sweepTargetStep = maxStep;
      // If we already hit max, flip direction
      if (currentServoStep >= maxStep) {
        sweepTargetStep = minStep;
        isMovingTowardsMax = false;
      }
    } else {
      sweepTargetStep = minStep;
      // If we already hit min, flip direction
      if (currentServoStep <= minStep) {
        sweepTargetStep = maxStep;
        isMovingTowardsMax = true;
      }
    }
    
    // Attach servo for the sweep
    myServo.attach(SERVO_PIN, 544, 2450);
  }

  if (sweeping) {
    unsigned long now = millis();
    
    if (!movingInStep) {
      // Check if sweep complete
      if (currentServoStep == sweepTargetStep) {
        delay(50);
        myServo.detach();
        sweeping = false;
        lastServoTurn = now;
      } else {
        // Start a smooth 6-degree movement over EGG_TURN_DURATION
        stepStartAngle = currentServoStep * 6;
        if (currentServoStep < sweepTargetStep) {
          currentServoStep++;
        } else {
          currentServoStep--;
        }
        stepTargetAngle = currentServoStep * 6;
        stepMoveStart = now;
        movingInStep = true;
      }
    }
    
    if (movingInStep) {
      unsigned long elapsed = now - stepMoveStart;
      float progress = (float)elapsed / (float)EGG_TURN_DURATION;
      
      if (progress >= 1.0f) {
        // Movement complete - set final position
        int angle = constrain(stepTargetAngle, 0, 180);
        myServo.write(angle);
        movingInStep = false;
        lastStepTime = now;
      } else {
        // Linear interpolation from start to target angle
        int angle = stepStartAngle + (int)((stepTargetAngle - stepStartAngle) * progress);
        angle = constrain(angle, 0, 180);
        myServo.write(angle);
      }
    }
  }
  
  servoPosition = currentServoStep;
  
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
  clearLogs();
  prepareBootTable();
  
  // Reset Boot ID in EEPROM
  EEPROM.write(EEPROM_BOOT_ID, 0);
  EEPROM.commit();
  
  // Send response and reboot
  server.send(200, "text/plain", "Flash cleared, boot ID reset to 0, rebooting...");
  delay(500); // Allow time for TCP transmission
  ESP.restart();
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

  // Hold servo pin LOW immediately to suppress SPI boot noise on GPIO14
  pinMode(SERVO_PIN, OUTPUT);
  digitalWrite(SERVO_PIN, LOW);

  connectWiFi();

  // Start mDNS responder for EGGubator.local
  if (MDNS.begin("EGGubator")) {
  }


// Setup web server
  server.on("/", handleRoot);
  server.on("/settings", handleSettingsPage);
  server.on("/dexie", handleDexiePage);
  server.on("/status", handleStatus);
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
  server.on("/timestamps", handleTimestamps);
  httpUpdater.setup(&server);
  server.begin();
  
  
  initRecovery();

  uint8_t bootId = EEPROM.read(EEPROM_BOOT_ID);
  bootId++;
  EEPROM.write(EEPROM_BOOT_ID, bootId);
  EEPROM.commit();

  markBootSuccess();

  initSectorPointers();
  prepareBootTable();
  initLogging(bootId);
  loadSettings();

  initDHT();

  uint8_t recoveredSteps[3] = {7, 7, 7};
  if (getLastServoPositions(recoveredSteps, 3)) {
    currentServoStep = recoveredSteps[0];
    // Simple direction detection: if pos0 > pos1 > pos2, moving towards min.
    // If pos0 < pos1 < pos2, moving towards max.
    if (recoveredSteps[0] > recoveredSteps[1] && recoveredSteps[1] > recoveredSteps[2]) {
      isMovingTowardsMax = false;
    } else if (recoveredSteps[0] < recoveredSteps[1] && recoveredSteps[1] < recoveredSteps[2]) {
      isMovingTowardsMax = true;
    }
  }

  servoInit();
}

// ============================================
// MAIN LOOP
// ============================================
void loop() {
  processDNS();
  server.handleClient();
  MDNS.update();

  unsigned long currentMillis = millis();

  {
    bool newStageLockdown = (getCurrentDay() >= 18);
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
      }
      // Always log state (servo position, relays) even if DHT sensor unavailable
      logData(currentTemp, currentHumidity, heaterState, atomizerState, fanState, currentServoStep, LOG_INTERVAL);
    lastReadTime = millis();
  }

  if (servoEnabled) {
    rotateEggs();
  }

  if (millis() - lastOtaCheck > 3600000) {
    checkAndUpdateAuto();
    lastOtaCheck = millis();
  }
}
