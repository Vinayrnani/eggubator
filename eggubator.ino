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
unsigned long LOG_INTERVAL = DEFAULT_LOG_INTERVAL;
unsigned long SAVE_FLASH_INTERVAL = DEFAULT_SAVE_FLASH_INTERVAL;
unsigned long EGG_TURN_INTERVAL = DEFAULT_EGG_TURN_INTERVAL;
unsigned long PULSE_ON_TIME = DEFAULT_PULSE_ON_TIME;

float TARGET_TEMP = INCUBATION_TARGET_TEMP;
float TARGET_HUMIDITY = INCUBATION_TARGET_HUMIDITY;

// Global variables
uint32_t bootId = 0;
int bootStartSector = 0;
float currentTemp = 0;
float currentHumidity = 0;
bool heaterState = false;
bool atomizerState = false;
bool fanState = false;
int servoPosition = 0;
bool servoTurning = false;
int servoRestPosition = 0;
int servoCurrentAngle = SERVO_CENTER;
unsigned long servoTurnStartTime = 0;
int servoTurnStartPosition = 0;
int servoTurnTargetPosition = 0;
int heaterMode = AUTO;
bool stageLockdown = false;
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

bool isServoAutoEnabled() {
  return servoMode == AUTO && !stageLockdown;
}

int servoAngleForPosition(int position) {
  int normalized = 0;
  if (position > 0) normalized = 1;
  if (position < 0) normalized = -1;
  return SERVO_CENTER + (normalized * SERVO_ANGLE);
}

void writeServoAngle(int angle) {
  servoCurrentAngle = constrain(angle, SERVO_CENTER - SERVO_ANGLE, SERVO_CENTER + SERVO_ANGLE);
  if (eggServo.attached()) {
    eggServo.write(servoCurrentAngle);
  }

  if (servoCurrentAngle < SERVO_CENTER - 5) {
    servoPosition = -1;
  } else if (servoCurrentAngle > SERVO_CENTER + 5) {
    servoPosition = 1;
  } else {
    servoPosition = 0;
  }
}

void stopServoMotion(bool centerServo) {
  servoTurning = false;
  servoTurnStartTime = 0;
  servoTurnStartPosition = servoPosition;
  servoTurnTargetPosition = 0;
  if (centerServo) {
    servoRestPosition = 0;
    writeServoAngle(SERVO_CENTER);
  }
}

void prepareServoAutoMode() {
  if (!isServoAutoEnabled()) {
    stopServoMotion(true);
    return;
  }

  if (servoRestPosition == 0) {
    servoRestPosition = -1;
    writeServoAngle(servoAngleForPosition(servoRestPosition));
    lastServoTurn = millis();
  }
}

void applyStageSettings(bool lockdown) {
  stageLockdown = lockdown;
  TARGET_TEMP = lockdown ? LOCKDOWN_TARGET_TEMP : INCUBATION_TARGET_TEMP;
  TARGET_HUMIDITY = lockdown ? LOCKDOWN_TARGET_HUMIDITY : INCUBATION_TARGET_HUMIDITY;

  if (lockdown) {
    stopServoMotion(true);
  } else if (servoMode == AUTO) {
    prepareServoAutoMode();
  }
}

void handleRoot() {
  server.send_P(200, PSTR("text/html; charset=utf-8"), INDEX_HTML);
}

void handleMockPage() {
  server.send_P(200, PSTR("text/html; charset=utf-8"), MOCK_HTML);
}

void handleData() {
  if (server.hasArg("sector")) {
    int sector = server.arg("sector").toInt();
    String json = "";
    getFlashLogDataForWeb(sector, json);
    server.send(200, "application/json", json);
    return;
  }

  unsigned long uptimeSec = millis() / 1000;
  int days = uptimeSec / 86400;
  int hours = (uptimeSec % 86400) / 3600;
  int mins = (uptimeSec % 3600) / 60;
  int secs = uptimeSec % 60;
  String uptimeStr = "";
  if (days > 0) uptimeStr += String(days) + "d ";
  uptimeStr += String(hours) + "h " + String(mins) + "m " + String(secs) + "s";

  int ramLogCount = getLogEntryCount();
  float ramLogStorageKb = getLogStorageBytes() / 1024.0f;

  String json = "{\"temperature\":" + String(currentTemp, 1) +
                ",\"humidity\":" + String(currentHumidity, 1) +
                ",\"heater\":" + String(heaterState ? "true" : "false") +
                ",\"atomizer\":" + String(atomizerState ? "true" : "false") +
                ",\"fan\":" + String(fanState ? "true" : "false") +
                ",\"servo\":" + String(isServoAutoEnabled() ? "true" : "false") +
                ",\"servoTurning\":" + String(servoTurning ? "true" : "false") +
                ",\"servoPosition\":" + String(servoPosition) +
                ",\"version\":\"" + FIRMWARE_VERSION + "\"" +
                ",\"uptime\":\"" + uptimeStr + "\"" +
                ",\"mock\":" + String(useMockSensor ? "true" : "false") +
                ",\"autosim\":" + String(autoSimMode ? "true" : "false") +
                ",\"stageLockdown\":" + String(stageLockdown ? "true" : "false") +
                ",\"heaterMode\":" + String(heaterMode) +
                ",\"atomizerMode\":" + String(atomizerMode) +
                ",\"fanMode\":" + String(fanMode) +
                ",\"servoMode\":" + String(servoMode) +
                ",\"targetTemp\":" + String(TARGET_TEMP, 1) +
                ",\"targetHumidity\":" + String(TARGET_HUMIDITY, 1) +
                ",\"logCnt\":" + String(ramLogCount) +
                ",\"logStorage\":" + String(ramLogStorageKb, 1) +
                ",\"bootId\":" + String(bootId) +
                ",\"uptime_ms\":" + String(millis()) +
                ",\"bootStartSector\":" + String(bootStartSector) +
                ",\"currentSector\":" + String(currentSector) +
                ",\"sys\":{\"heapFree\":" + String(ESP.getFreeHeap()) +
                ",\"heapTotal\":81920" +
                ",\"cpu\":" + String(cpuUtil) +
                ",\"flashSize\":" + String(ESP.getFlashChipSize()) +
                ",\"flashTotal\":4194304" +
                "}";
  getLogDataForWeb(json);
  json += "}";
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
      if (isKillOff) {
        stopServoMotion(true);
      } else {
        prepareServoAutoMode();
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
  server.send(200, "application/json", "{\"status\":\"Update started\"}");
  delay(100);
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
    saveSettings();
    server.send(200, "text/plain", "Log interval set to " + String(val/1000) + "s");
  } else if (server.hasArg("saveFlashInterval")) {
    unsigned long val = server.arg("saveFlashInterval").toInt();
    SAVE_FLASH_INTERVAL = val;
    saveSettings();
    server.send(200, "text/plain", "Save to Flash interval set to " + String(val/60000) + "min");
  } else if (server.hasArg("eggTurnInterval")) {
    unsigned long val = server.arg("eggTurnInterval").toInt();
    EGG_TURN_INTERVAL = val;
    saveSettings();
    server.send(200, "text/plain", "Egg turner interval set to " + String(val/3600000) + " hours");
  } else if (server.hasArg("pulseOnTime")) {
    unsigned long val = server.arg("pulseOnTime").toInt();
    PULSE_ON_TIME = val;
    saveSettings();
    server.send(200, "text/plain", "Atomizer pulse set to " + String(val/1000) + "s");
  } else if (server.hasArg("stageType")) {
    String type = server.arg("stageType");
    if (type == "lockdown") {
      applyStageSettings(true);
    } else if (type == "incubation") {
      applyStageSettings(false);
    }
    saveSettings();
    server.send(200, "text/plain", "Stage set to " + type);
  } else {
    String stageType = stageLockdown ? "lockdown" : "incubation";
    String json = "{\"enabled\":" + String(useMockSensor ? "true" : "false") +
                  ",\"autosim\":" + String(autoSimMode ? "true" : "false") +
                  ",\"temp\":" + String(mockTemp, 1) +
                  ",\"hum\":" + String(mockHum, 1) +
                  ",\"logInterval\":" + String(LOG_INTERVAL) +
                  ",\"saveFlashInterval\":" + String(SAVE_FLASH_INTERVAL) +
                  ",\"eggTurnInterval\":" + String(EGG_TURN_INTERVAL) +
                  ",\"pulseOnTime\":" + String(PULSE_ON_TIME) +
                  ",\"stageLockdown\":" + String(stageLockdown ? "true" : "false") +
                  ",\"stageType\":\"" + stageType + "\"" +
                  ",\"targetTemp\":" + String(TARGET_TEMP, 1) +
                  ",\"targetHumidity\":" + String(TARGET_HUMIDITY, 1) + "}";
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
  if (!isServoAutoEnabled()) {
    stopServoMotion(true);
    return;
  }

  if (!servoTurning) {
    if (servoRestPosition == 0) {
      prepareServoAutoMode();
      return;
    }

    if (lastServoTurn == 0) {
      lastServoTurn = millis();
      return;
    }

    if (millis() - lastServoTurn >= EGG_TURN_INTERVAL) {
      servoTurning = true;
      servoTurnStartTime = millis();
      servoTurnStartPosition = servoRestPosition;
      servoTurnTargetPosition = servoRestPosition > 0 ? -1 : 1;
      Serial.print("Egg turn started: ");
      Serial.print(servoTurnStartPosition);
      Serial.print(" -> ");
      Serial.println(servoTurnTargetPosition);
    }
  }

  if (!servoTurning) {
    return;
  }

  unsigned long elapsed = millis() - servoTurnStartTime;
  if (elapsed >= EGG_TURN_DURATION) {
    servoTurning = false;
    servoRestPosition = servoTurnTargetPosition;
    writeServoAngle(servoAngleForPosition(servoRestPosition));
    lastServoTurn = millis();
    Serial.println("Egg turn completed");
    return;
  }

  int startAngle = servoAngleForPosition(servoTurnStartPosition);
  int endAngle = servoAngleForPosition(servoTurnTargetPosition);
  int nextAngle = map(elapsed, 0, EGG_TURN_DURATION, startAngle, endAngle);
  writeServoAngle(nextAngle);
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
  
  randomSeed(analogRead(0) ^ micros());
  bootId = random(1, 1000000);
  
  pinMode(RELAY_HEATER, OUTPUT);
  pinMode(RELAY_ATOMIZER, OUTPUT);
  pinMode(RELAY_FAN, OUTPUT);
  digitalWrite(RELAY_HEATER, LOW);
  digitalWrite(RELAY_ATOMIZER, LOW);
  digitalWrite(RELAY_FAN, LOW);

  initDHT();
  eggServo.attach(SERVO_PIN);
  writeServoAngle(SERVO_CENTER);

  EEPROM.begin(EEPROM_SIZE_BYTES);
  initRecovery();
  initLogging();
  loadSettings();
  bootStartSector = currentSector;
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

  if (shouldSaveToFlash()) {
    saveLogsToFlash();
  }

  rotateEggs();

  if (millis() - lastOtaCheck > 3600000) {
    checkAndUpdateAuto();
    lastOtaCheck = millis();
  }
}
