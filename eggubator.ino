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
Servo eggServo;

// EEPROM addresses for settings
#define EEPROM_SETTINGS_MAGIC 40
#define EEPROM_BOOT_ID 12
#define SETTINGS_MAGIC_VAL 0xA5

struct DeviceSettings {
  uint8_t magic;
  bool stageLockdown;
  unsigned long logInterval;
  unsigned long turnInterval;
  unsigned long pulseOnTime;
};

void saveSettings() {
  DeviceSettings settings;
  EEPROM.get(EEPROM_SETTINGS_MAGIC, settings);
  
  bool changed = false;
  if (settings.magic != SETTINGS_MAGIC_VAL) { settings.magic = SETTINGS_MAGIC_VAL; changed = true; }
  if (settings.stageLockdown != stageLockdown) { settings.stageLockdown = stageLockdown; changed = true; }
  if (settings.logInterval != LOG_INTERVAL) { settings.logInterval = LOG_INTERVAL; changed = true; }
  if (settings.turnInterval != EGG_TURN_INTERVAL) { settings.turnInterval = EGG_TURN_INTERVAL; changed = true; }
  if (settings.pulseOnTime != PULSE_ON_TIME) { settings.pulseOnTime = PULSE_ON_TIME; changed = true; }
  
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
    
    if (settings.pulseOnTime == 2000 || settings.pulseOnTime == 3000 || settings.pulseOnTime == 4000 || settings.pulseOnTime == 5000) {
      PULSE_ON_TIME = settings.pulseOnTime;
    } else {
      PULSE_ON_TIME = 3000;
    }
    
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
                ",\"heaterMode\":" + String(heaterMode) +
                ",\"atomizerMode\":" + String(atomizerMode) +
                ",\"fanMode\":" + String(fanMode) +
                ",\"servoMode\":" + String(servoMode) +
                ",\"totalLogs\":" + String(MAX_LOG_ENTRIES) +
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
  
  uint32_t totalLogs = MAX_LOG_ENTRIES;
  
  String logHex = "";
  int sentCount = getLogHex(logHex, count, sinceBootId, sinceTimeSec);
  
  json += ",\"totalLogs\":" + String(MAX_LOG_ENTRIES) +
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
      servoMode = isKillOff ? KILL_OFF : AUTO;
      servoEnabled = !isKillOff;
      if (isKillOff) {
        servoPosition = 0;
        eggServo.write(SERVO_CENTER);
      }
    }
    server.send(200, "text/plain", device + " mode set to " + (isKillOff ? "OFF" : "AUTO"));
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
    saveSettings();
    server.send(200, "text/plain", "Egg turner interval set to " + String(val/3600000) + " hours");
  } else if (server.hasArg("pulseOnTime")) {
    unsigned long val = server.arg("pulseOnTime").toInt();
    PULSE_ON_TIME = val;
    saveSettings();
    server.send(200, "text/plain", "Atomizer pulse on time set to " + String(val/1000) + "s");
  } else if (server.hasArg("stageType")) {
    String type = server.arg("stageType");
    if (type == "lockdown") {
      stageLockdown = true;
      TARGET_TEMP = 37.5;
      TARGET_HUMIDITY = 65.0;
      servoEnabled = false;
    } else if (type == "incubation") {
      stageLockdown = false;
      TARGET_TEMP = 37.5;
      TARGET_HUMIDITY = 55.0;
      servoEnabled = true;
    }
    saveSettings();
    server.send(200, "text/plain", "Stage set to " + type);
  } else {
    String json = "{\"enabled\":" + String(useMockSensor ? "true" : "false") + 
                  ",\"autosim\":" + String(autoSimMode ? "true" : "false") +
                  ",\"temp\":" + String(mockTemp) + 
                  ",\"hum\":" + String(mockHum) +
                  ",\"logInterval\":" + String(LOG_INTERVAL) +
                  ",\"eggTurnInterval\":" + String(EGG_TURN_INTERVAL) +
                  ",\"pulseOnTime\":" + String(PULSE_ON_TIME) +
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

      if (atomizerPulsing && (millis() - atomizerPulseStart >= PULSE_ON_TIME)) {
        atomizerState = false;
        digitalWrite(RELAY_ATOMIZER, LOW);
        atomizerPulsing = false;
        atomizerInOffPhase = true;
        atomizerOffStart = millis();
      } else if (atomizerInOffPhase && (millis() - atomizerOffStart >= PULSE_OFF_TIME)) {
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
// EGG TURNER
// ============================================
void rotateEggs() {
  static bool turning = false;
  static unsigned long turnStartTime = 0;
  static bool restingAt45 = true; // State: true=45deg, false=135deg
  
  // Disable egg turner during lockdown stage
  if (stageLockdown) {
    servoEnabled = false;
    servoPosition = 0;
    eggServo.write(SERVO_CENTER);
    return;
  }
  
  if (servoEnabled && servoMode == AUTO && !turning && (millis() - lastServoTurn > EGG_TURN_INTERVAL)) {
    turning = true;
    turnStartTime = millis();
    Serial.println("Egg turn started");
  }
  
  if (turning) {
    unsigned long elapsed = millis() - turnStartTime;
    int startAngle = restingAt45 ? 45 : 135;
    int endAngle = restingAt45 ? 135 : 45;
    
    int targetAngle = map(elapsed, 0, EGG_TURN_DURATION, startAngle, endAngle);
    eggServo.write(targetAngle);
    
    // Update visual position indicator
    servoPosition = restingAt45 ? 2 : 1; // 2=Right(135), 1=Left(45)
    
    if (elapsed >= EGG_TURN_DURATION) {
      eggServo.write(endAngle);
      turning = false;
      restingAt45 = !restingAt45; // Toggle resting state
      servoPosition = restingAt45 ? 1 : 2;
      lastServoTurn = millis();
      Serial.println("Egg turn completed");
    }
  } else {
    // Hold position
    eggServo.write(restingAt45 ? 45 : 135);
    servoPosition = restingAt45 ? 1 : 2;
  }
  
  if (!servoEnabled || servoMode == KILL_OFF) {
    servoPosition = 0;
    eggServo.write(SERVO_CENTER);
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
  eggServo.attach(SERVO_PIN);

  initRecovery();
  
  uint8_t bootId = EEPROM.read(EEPROM_BOOT_ID);
  bootId++;
  EEPROM.write(EEPROM_BOOT_ID, bootId);
  EEPROM.commit();

  initLogging(bootId);
  loadSettings();
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

  if (millis() - lastReadTime > 2000) {
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

  if (millis() - lastOtaCheck > 3600000) {
    checkAndUpdateAuto();
    lastOtaCheck = millis();
  }
}
