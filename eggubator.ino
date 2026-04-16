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

// Global variables
float currentTemp = 0;
float currentHumidity = 0;
bool heaterState = false;
bool atomizerState = false;
bool fanState = false;
bool servoEnabled = false;
unsigned long lastReadTime = 0;
unsigned long lastOtaCheck = 0;
unsigned long lastServoTurn = 0;

// Control state variables
unsigned long atomizerPulseStart = 0;
bool atomizerPulsing = false;
unsigned long heaterLastChanged = 0;
bool heaterWasOn = false;
unsigned long atomizerLastChanged = 0;
bool atomizerWasOn = false;

// Log buffer (defined in logging.h)
LogEntry logBuffer[MAX_LOG_ENTRIES];
int logIndex = 0;
bool logFull = false;
unsigned long lastLogTime = 0;

// Web server
ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;
Servo eggServo;

// ============================================
// WEB INTERFACE HTML
// ============================================
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Egg Incubator</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial; margin: 20px; background: #f5f5f5; }
    .card { background: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); margin-bottom: 20px; }
    h1 { color: #333; }
    .stat { font-size: 24px; margin: 10px 0; }
    .label { color: #666; font-size: 14px; }
    .on { color: #4CAF50; }
    .off { color: #f44336; }
    .btn { padding: 10px 20px; margin: 5px; border: none; border-radius: 5px; cursor: pointer; font-size: 14px; }
    .btn-on { background: #4CAF50; color: white; }
    .btn-off { background: #f44336; color: white; }
    .btn-auto { background: #2196F3; color: white; }
    .status-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; }
    canvas { background: #fafafa; border: 1px solid #ddd; }
  </style>
</head>
<body>
  <h1>Egg Incubator</h1>
  <div class="card">
    <div class="label">Firmware</div>
    <div class="stat" id="version">--</div>
    <div class="label">Uptime</div>
    <div class="stat" id="uptime">--</div>
  </div>
  <div class="card status-grid">
    <div>
      <div class="label">Temperature</div>
      <div class="stat" id="temp">--°C</div>
    </div>
    <div>
      <div class="label">Humidity</div>
      <div class="stat" id="hum">--%</div>
    </div>
  </div>
  <div class="card status-grid">
    <div>
      <div class="label">Heater</div>
      <div class="stat" id="heater">OFF</div>
      <button class="btn btn-on" onclick="toggleDevice('heater','on')">ON</button>
      <button class="btn btn-off" onclick="toggleDevice('heater','off')">OFF</button>
    </div>
    <div>
      <div class="label">Atomizer</div>
      <div class="stat" id="atomizer">OFF</div>
      <button class="btn btn-on" onclick="toggleDevice('atomizer','on')">ON</button>
      <button class="btn btn-off" onclick="toggleDevice('atomizer','off')">OFF</button>
    </div>
    <div>
      <div class="label">Fan</div>
      <div class="stat" id="fan">OFF</div>
      <button class="btn btn-on" onclick="toggleDevice('fan','on')">ON</button>
      <button class="btn btn-off" onclick="toggleDevice('fan','off')">OFF</button>
    </div>
    <div>
      <div class="label">Egg Turner</div>
      <div class="stat" id="servo">OFF</div>
      <button class="btn btn-on" onclick="toggleDevice('servo','on')">ON</button>
      <button class="btn btn-off" onclick="toggleDevice('servo','off')">OFF</button>
    </div>
  </div>
    <div class="card">
    <div class="label">Target Settings</div>
    <div id="targets">Temp: 37.5°C | Humidity: 60%</div>
  </div>
  <div class="card">
    <div class="label">OTA Update</div>
    <button class="btn btn-auto" onclick="checkOta()">Check Update</button>
    <div id="otaStatus"></div>
  </div>
  <div class="card">
    <div class="label">Temperature Chart</div>
    <canvas id="tempChart" width="100%" height="150"></canvas>
  </div>
  <div class="card">
    <div class="label">Humidity Chart</div>
    <canvas id="humChart" width="100%" height="150"></canvas>
  </div>
  <script>
    function updateData() {
      fetch('/data').then(r => r.json()).then(d => {
        document.getElementById('version').textContent = d.version;
        document.getElementById('temp').textContent = d.temperature.toFixed(1) + '°C';
        document.getElementById('hum').textContent = d.humidity.toFixed(1) + '%';
        document.getElementById('uptime').textContent = d.uptime;
        document.getElementById('heater').textContent = d.heater ? 'ON' : 'OFF';
        document.getElementById('heater').className = 'stat ' + (d.heater ? 'on' : 'off');
        document.getElementById('atomizer').textContent = d.atomizer ? 'ON' : 'OFF';
        document.getElementById('atomizer').className = 'stat ' + (d.atomizer ? 'on' : 'off');
        document.getElementById('fan').textContent = d.fan ? 'ON' : 'OFF';
        document.getElementById('fan').className = 'stat ' + (d.fan ? 'on' : 'off');
        document.getElementById('servo').textContent = d.servo ? 'ON' : 'OFF';
        document.getElementById('servo').className = 'stat ' + (d.servo ? 'on' : 'off');
        if (d.log && d.log.length > 0) {
          drawTempChart(d.log);
          drawHumChart(d.log);
        }
      });
    }
    function drawTempChart(logData) {
      const canvas = document.getElementById('tempChart');
      const ctx = canvas.getContext('2d');
      const w = canvas.width = canvas.offsetWidth;
      const h = canvas.height = 150;
      ctx.clearRect(0, 0, w, h);
      if (!logData || logData.length < 2) return;
      const maxTemp = 40, minTemp = 35;
      ctx.beginPath();
      ctx.strokeStyle = '#ff5722';
      ctx.lineWidth = 2;
      for (let i = 0; i < logData.length; i++) {
        const x = (i / (logData.length - 1)) * w;
        const y = h - ((logData[i].temp - minTemp) / (maxTemp - minTemp)) * h;
        if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
      }
      ctx.stroke();
      const targetY = h - ((TARGET_TEMP - minTemp) / (maxTemp - minTemp)) * h;
      ctx.beginPath();
      ctx.strokeStyle = '#4CAF50';
      ctx.setLineDash([5, 5]);
      ctx.moveTo(0, targetY);
      ctx.lineTo(w, targetY);
      ctx.stroke();
      ctx.setLineDash([]);
    }
    function drawHumChart(logData) {
      const canvas = document.getElementById('humChart');
      const ctx = canvas.getContext('2d');
      const w = canvas.width = canvas.offsetWidth;
      const h = canvas.height = 150;
      ctx.clearRect(0, 0, w, h);
      if (!logData || logData.length < 2) return;
      const maxHum = 80, minHum = 40;
      ctx.beginPath();
      ctx.strokeStyle = '#2196F3';
      ctx.lineWidth = 2;
      for (let i = 0; i < logData.length; i++) {
        const x = (i / (logData.length - 1)) * w;
        const y = h - ((logData[i].hum - minHum) / (maxHum - minHum)) * h;
        if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
      }
      ctx.stroke();
      const targetY = h - ((TARGET_HUMIDITY - minHum) / (maxHum - minHum)) * h;
      ctx.beginPath();
      ctx.strokeStyle = '#4CAF50';
      ctx.setLineDash([5, 5]);
      ctx.moveTo(0, targetY);
      ctx.lineTo(w, targetY);
      ctx.stroke();
      ctx.setLineDash([]);
    }
    function toggleDevice(device, state) {
      fetch('/control?device=' + device + '&state=' + state).then(() => updateData());
    }
    function checkOta() {
      document.getElementById('otaStatus').textContent = 'Checking...';
      fetch('/ota/check').then(r => r.json()).then(d => {
        document.getElementById('otaStatus').textContent = d.update ? 'Update: ' + d.version : 'Up to date';
        if(d.update) {
          fetch('/ota/update').then(r => r.json()).then(r => {
            document.getElementById('otaStatus').textContent = r.status;
          });
        }
      });
    }
    setInterval(updateData, 2000);
    updateData();
  </script>
</body>
</html>
)rawliteral";

// ============================================
// WEB SERVER HANDLERS
// ============================================
void handleRoot() {
  server.send(200, "text/html; charset=utf-8", INDEX_HTML);
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
               ",\"uptime\":\"" + uptimeStr + "\"";
  getLogDataForWeb(json);
  json += "}";
  server.send(200, "application/json", json);
}

void handleControl() {
  if (server.hasArg("device") && server.hasArg("state")) {
    String device = server.arg("device");
    String state = server.arg("state");
    bool isOn = (state == "on");
    
    if (device == "heater") {
      heaterState = isOn;
      digitalWrite(RELAY_HEATER, heaterState ? HIGH : LOW);
    } else if (device == "atomizer") {
      atomizerState = isOn;
      digitalWrite(RELAY_ATOMIZER, atomizerState ? HIGH : LOW);
    } else if (device == "fan") {
      fanState = isOn;
      digitalWrite(RELAY_FAN, fanState ? HIGH : LOW);
    } else if (device == "servo") {
      servoEnabled = isOn;
    }
  }
  server.send(200, "text/plain", "OK");
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

// ============================================
// AUTO CONTROL LOGIC
// ============================================
void autoControl() {
  if (!isnan(currentTemp) && !isnan(currentHumidity)) {
    unsigned long now = millis();
    
    // Temperature control
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
    
    // Pulsating humidity control (3s ON, 10s OFF)
    if (currentHumidity < TARGET_HUMIDITY) {
      if (!atomizerPulsing) {
        atomizerState = true;
        digitalWrite(RELAY_ATOMIZER, HIGH);
        atomizerPulseStart = now;
        atomizerPulsing = true;
      } else if (now - atomizerPulseStart >= PULSE_ON_TIME) {
        atomizerState = false;
        digitalWrite(RELAY_ATOMIZER, LOW);
        atomizerPulsing = false;
      }
    } else {
      if (atomizerState) {
        atomizerState = false;
        digitalWrite(RELAY_ATOMIZER, LOW);
        atomizerPulsing = false;
      }
    }
    
    if (atomizerState != atomizerWasOn) {
      atomizerLastChanged = now;
      atomizerWasOn = atomizerState;
    }
    
    // Determine stability
    bool tempStable = (currentTemp >= TARGET_TEMP - TEMP_HYSTERESIS && 
                     currentTemp <= TARGET_TEMP + TEMP_HYSTERESIS);
    bool humStable = (currentHumidity >= TARGET_HUMIDITY - HUMIDITY_HYSTERESIS && 
                    currentHumidity <= TARGET_HUMIDITY + HUMIDITY_HYSTERESIS);
    
    // Fan control
    bool withinHeaterWindow = (!heaterState && (now - heaterLastChanged < FAN_EXTEND_TIME));
    bool withinAtomizerWindow = (!atomizerState && (now - atomizerLastChanged < FAN_EXTEND_TIME));
    
    if (heaterState || withinHeaterWindow || atomizerState || withinAtomizerWindow || 
        currentTemp > MAX_SAFE_TEMP) {
      fanState = true;
    } else if (tempStable && humStable) {
      fanState = false;
    }
    digitalWrite(RELAY_FAN, fanState ? HIGH : LOW);
  }
}

// ============================================
// EGG TURNER
// ============================================
void rotateEggs() {
  static int servoPos = 0;
  static bool sweeping = false;
  
  if (servoEnabled && (millis() - lastServoTurn > 7200000)) {
    if (!sweeping) {
      for (servoPos = 0; servoPos <= 180; servoPos += 10) {
        eggServo.write(servoPos);
        delay(50);
      }
      sweeping = true;
    } else {
      for (servoPos = 180; servoPos >= 0; servoPos -= 10) {
        eggServo.write(servoPos);
        delay(50);
      }
      sweeping = false;
    }
    lastServoTurn = millis();
    Serial.println("Eggs rotated");
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

  eggServo.attach(SERVO_PIN);

  initLogging();
  initRecovery();
  connectWiFi();
  markBootSuccess();

  Serial.print("Firmware: ");
  Serial.println(FIRMWARE_VERSION);

  // Setup web server
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/control", handleControl);
server.on("/ota/check", handleOtaCheck);
  server.on("/ota/update", handleOtaUpdate);
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

  if (millis() - lastReadTime > 2000) {
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
    logData(currentTemp, currentHumidity, heaterState, atomizerState, fanState);
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
