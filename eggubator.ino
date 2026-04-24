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
unsigned long LOG_INTERVAL = 10000;
unsigned long EGG_TURN_INTERVAL = 7200000;

// Target temperature and humidity (can be changed via web/stage selection)
unsigned long TARGET_TEMP = 375;    // Default 37.5°C
unsigned long TARGET_HUMIDITY = 600; // Default 60.0%

// Global variables
float currentTemp = 0;
float currentHumidity = 0;
bool heaterState = false;
bool atomizerState = false;
bool fanState = false;
bool servoEnabled = false;
int servoPosition = 0; // -1 = -45deg, 0 = center, 1 = +45deg
int heaterMode = AUTO;
bool stageLockdown = false;  // false = incubation (1-18), true = lockdown (19-21)
int atomizerMode = AUTO;
int fanMode = AUTO;
int servoMode = AUTO;
unsigned long lastReadTime = 0;
unsigned long lastOtaCheck = 0;
unsigned long lastServoTurn = 0;

// CPU monitoring
unsigned long lastCpuCheck = 0;
unsigned long cpuCyclesStart = 0;
unsigned long cpuCyclesEnd = 0;
unsigned long cpuUtil = 0;

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

// ============================================
// WEB SERVER HANDLERS
// ============================================
void handleRoot() {
  server.send(200, "text/html; charset=utf-8", INDEX_HTML);
}

void handleMockPage() {
  server.send(200, "text/html; charset=utf-8", MOCK_HTML);
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
                ",\"heater\":" + String(heaterState ? "true" : "false") +
                ",\"atomizer\":" + String(atomizerState ? "true" : "false") +
                ",\"fan\":" + String(fanState ? "true" : "false") +
                ",\"servo\":" + String(servoEnabled ? "true" : "false") +
                ",\"version\":\"" + FIRMWARE_VERSION + "\"" +
                ",\"uptime\":\"" + uptimeStr + "\"" +
                ",\"mock\":" + String(useMockSensor ? "true" : "false") +
                ",\"autosim\":" + String(autoSimMode ? "true" : "false") +
                ",\"stageLockdown\":" + String(stageLockdown ? "true" : "false") +
                ",\"heaterMode\":" + String(heaterMode) +
                ",\"atomizerMode\":" + String(atomizerMode) +
                ",\"fanMode\":" + String(fanMode) +
                ",\"servoMode\":" + String(servoMode) +
                ",\"logCnt\":" + String(logIndex) +
                ",\"sys\":{\"heapFree\":" + String(ESP.getFreeHeap()) +
                ",\"cpu\":" + String(cpuUtil) +
                "}}";
  
  // Note: getLogDataForWeb handles adding the "log" array to the JSON
  String jsonWithLog = json.substring(0, json.length() - 1);
  getLogDataForWeb(jsonWithLog);
  jsonWithLog += "}";
  
  server.send(200, "application/json", jsonWithLog);
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
      if (isKillOff) {
        servoEnabled = false;
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

void handleMockSensor() {
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
    server.send(200, "text/plain", "Log interval set to " + String(val/1000) + "s");
  } else if (server.hasArg("eggTurnInterval")) {
    unsigned long val = server.arg("eggTurnInterval").toInt();
    EGG_TURN_INTERVAL = val;
    server.send(200, "text/plain", "Egg turner interval set to " + String(val/3600000) + " hours");
  } else if (server.hasArg("stageType")) {
    String type = server.arg("stageType");
    if (type == "lockdown") {
      stageLockdown = true;
      TARGET_TEMP = 375;
      TARGET_HUMIDITY = 650;
    } else if (type == "incubation") {
      stageLockdown = false;
      TARGET_TEMP = 375;
      TARGET_HUMIDITY = 550;
    }
    server.send(200, "text/plain", "Stage set to " + type);
  } else {
    String json = "{\"enabled\":" + String(useMockSensor ? "true" : "false") + 
                  ",\"autosim\":" + String(autoSimMode ? "true" : "false") +
                  ",\"temp\":" + String(mockTemp) + 
                  ",\"hum\":" + String(mockHum) +
                  ",\"logInterval\":" + String(LOG_INTERVAL) +
                  ",\"eggTurnInterval\":" + String(EGG_TURN_INTERVAL) +
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
    
    if (heaterMode == AUTO) {
      if (currentTemp < TARGET_TEMP - TEMP_HYSTERESIS) {
        heaterState = true;
      } else if (currentTemp > TARGET_TEMP + TEMP_HYSTERESIS) {
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
    
    if (atomizerMode == AUTO) {
      if (currentHumidity < TARGET_HUMIDITY) {
        if (!atomizerPulsing && !atomizerInOffPhase) {
          atomizerState = true;
          digitalWrite(RELAY_ATOMIZER, HIGH);
          atomizerPulseStart = millis();
          atomizerPulsing = true;
        } else if (atomizerPulsing && (millis() - atomizerPulseStart >= PULSE_ON_TIME)) {
          atomizerState = false;
          digitalWrite(RELAY_ATOMIZER, LOW);
          atomizerPulsing = false;
          atomizerInOffPhase = true;
          atomizerOffStart = millis();
        } else if (atomizerInOffPhase && (millis() - atomizerOffStart >= PULSE_OFF_TIME)) {
          atomizerInOffPhase = false;
        }
      } else {
        if (atomizerState) {
          atomizerState = false;
          digitalWrite(RELAY_ATOMIZER, LOW);
          atomizerPulsing = false;
          atomizerInOffPhase = false;
        }
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
    
    bool tempStable = (currentTemp >= TARGET_TEMP - TEMP_HYSTERESIS && 
                     currentTemp <= TARGET_TEMP + TEMP_HYSTERESIS);
    bool humStable = (currentHumidity >= TARGET_HUMIDITY - HUMIDITY_HYSTERESIS && 
                     currentHumidity <= TARGET_HUMIDITY + HUMIDITY_HYSTERESIS);
    
    if (fanMode == AUTO) {
      bool withinHeaterWindow = (!heaterState && (now - heaterLastChanged < FAN_EXTEND_TIME));
      bool withinAtomizerWindow = (!atomizerState && (now - atomizerLastChanged < FAN_EXTEND_TIME));
      
      if (heaterState || withinHeaterWindow || atomizerState || withinAtomizerWindow || 
          currentTemp > MAX_SAFE_TEMP) {
        fanState = true;
      } else if (tempStable && humStable) {
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
  static int currentAngle = 0;
  
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
    currentAngle = SERVO_CENTER - SERVO_ANGLE;
    eggServo.write(currentAngle);
    servoPosition = -1;
    Serial.println("Egg turn started");
  }
  
  if (turning && (millis() - turnStartTime < EGG_TURN_DURATION)) {
    unsigned long elapsed = millis() - turnStartTime;
    int targetAngle;
    
    if (elapsed < EGG_TURN_DURATION / 2) {
      targetAngle = map(elapsed, 0, EGG_TURN_DURATION/2, SERVO_CENTER - SERVO_ANGLE, SERVO_CENTER + SERVO_ANGLE);
      servoPosition = 1;
    } else {
      targetAngle = map(elapsed, EGG_TURN_DURATION/2, EGG_TURN_DURATION, SERVO_CENTER + SERVO_ANGLE, SERVO_CENTER - SERVO_ANGLE);
      servoPosition = -1;
    }
    
    if (targetAngle != currentAngle) {
      currentAngle = targetAngle;
      eggServo.write(targetAngle);
    }
  }
  
  if (turning && (millis() - turnStartTime >= EGG_TURN_DURATION)) {
    turning = false;
    eggServo.write(SERVO_CENTER - SERVO_ANGLE);
    servoPosition = -1;
    lastServoTurn = millis();
    Serial.println("Egg turn completed");
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

  initLogging();
  initRecovery();
  connectWiFi();
  markBootSuccess();

  Serial.print("Firmware: ");
  Serial.println(FIRMWARE_VERSION);

// Setup web server
  server.on("/", handleRoot);
  server.on("/mock", handleMockPage);
  server.on("/data", handleData);
  server.on("/control", handleControl);
  server.on("/ota/check", handleOtaCheck);
  server.on("/ota/update", handleOtaUpdate);
  server.on("/mock/api", handleMockSensor);
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

  // CPU monitoring - measure cycles every second
  if (millis() - lastCpuCheck > 1000) {
    cpuCyclesEnd = ESP.getCycleCount();
    if (cpuCyclesStart > 0) {
      unsigned long cycles = cpuCyclesEnd - cpuCyclesStart;
      cpuUtil = (cycles / 8000); // 80MHz / 10000 = rough %, adjusted
      if (cpuUtil > 100) cpuUtil = 100;
    }
    cpuCyclesStart = cpuCyclesEnd;
    lastCpuCheck = millis();
  }

  if (millis() - lastReadTime > 2000) {
    if (autoSimMode) {
      updateAutoSim(heaterState, atomizerState, fanState);
    }
    float t = readDHT22();
    float h = readHumidity();

    if (t > 0 && h > 0) {
      currentTemp = t;
      currentHumidity = h;
      autoControl();
    }
    lastReadTime = millis();
  }

  if (millis() - lastLogTime >= LOG_INTERVAL) {
    logData(currentTemp, currentHumidity, heaterState, atomizerState, fanState, servoPosition);
    lastLogTime = millis();
  }

  if (servoEnabled) {
    rotateEggs();
  }

  if (millis() - lastOtaCheck > 3600000) {
    checkAndUpdateAuto();
    lastOtaCheck = millis();
  }
}
