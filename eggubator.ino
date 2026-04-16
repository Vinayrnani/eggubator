#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266HTTPClient.h>
#include <Servo.h>

#define DHTPIN D4
#define DHTTYPE DHT22

#define RELAY_HEATER D1
#define RELAY_ATOMIZER D2
#define RELAY_FAN D3
#define SERVO_PIN D5

// Simple DHT22 read without library
float readDHT22() {
  int data[5] = {0, 0, 0, 0, 0};
  unsigned long startTime = millis();
  
  pinMode(DHTPIN, OUTPUT);
  digitalWrite(DHTPIN, LOW);
  delay(18);
  digitalWrite(DHTPIN, HIGH);
  delayMicroseconds(30);
  pinMode(DHTPIN, INPUT);
  
  unsigned long timeout = micros();
  while (digitalRead(DHTPIN) == LOW) {
    if (micros() - timeout > 100) return -1;
  }
  timeout = micros();
  while (digitalRead(DHTPIN) == HIGH) {
    if (micros() - timeout > 100) return -1;
  }
  
  for (int i = 0; i < 40; i++) {
    unsigned long bitStart = micros();
    while (digitalRead(DHTPIN) == LOW) {
      if (micros() - bitStart > 50) break;
    }
    unsigned long bitEnd = micros();
    if (bitEnd - bitStart > 40) data[i / 8] |= (1 << (7 - i % 8));
  }
  
  if (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) {
    return (float)((data[0] << 8) + data[1]) / 10.0;
  }
  return -1;
}

float readHumidity() {
  int data[5] = {0, 0, 0, 0, 0};
  
  pinMode(DHTPIN, OUTPUT);
  digitalWrite(DHTPIN, LOW);
  delay(18);
  digitalWrite(DHTPIN, HIGH);
  delayMicroseconds(30);
  pinMode(DHTPIN, INPUT);
  
  unsigned long timeout = micros();
  while (digitalRead(DHTPIN) == LOW) {
    if (micros() - timeout > 100) return -1;
  }
  timeout = micros();
  while (digitalRead(DHTPIN) == HIGH) {
    if (micros() - timeout > 100) return -1;
  }
  
  for (int i = 0; i < 40; i++) {
    unsigned long bitStart = micros();
    while (digitalRead(DHTPIN) == LOW) {
      if (micros() - bitStart > 50) break;
    }
    unsigned long bitEnd = micros();
    if (bitEnd - bitStart > 40) data[i / 8] |= (1 << (7 - i % 8));
  }
  
  if (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) {
    return (float)((data[2] << 8) + data[3]) / 10.0;
  }
  return -1;
}

const char* ssid = "Sweet Home";
const char* password = "dishoom1234";

const char* firmwareUrl = "http://YOUR_SERVER/firmware.bin";
const char* versionUrl = "http://YOUR_SERVER/version.txt";

const char* firmwareVersion = "1.2.1";

const float TARGET_TEMP = 37.5;
const float TEMP_HYSTERESIS = 0.5;
const float TARGET_HUMIDITY = 60.0;
const float HUMIDITY_HYSTERESIS = 5.0;
const float MAX_SAFE_TEMP = 38.0;

// Pulsating humidity control timing
const unsigned long PULSE_ON_TIME = 3000;
const unsigned long PULSE_OFF_TIME = 10000;

// Fan extension timing
const unsigned long FAN_EXTEND_TIME = 5000;

// Data logging - RAM with EEPROM checkpoint for power failure detection
#include <EEPROM.h>

const unsigned long LOG_INTERVAL = 30000;
const int MAX_LOG_ENTRIES = 100;

struct LogEntry {
  unsigned long timestamp;
  float temperature;
  float humidity;
  bool heaterState;
  bool atomizerState;
  bool fanState;
};

// RAM storage for logging
LogEntry logBuffer[MAX_LOG_ENTRIES];
int logIndex = 0;
bool logFull = false;
unsigned long lastLogTime = 0;

// EEPROM addresses for power failure detection
const int EEPROM_MAGIC = 0;
const int EEPROM_INDEX = 1;
const int EEPROM_CHECK = 2;
const unsigned char MAGIC_VAL = 0x42;
const unsigned char CHECK_VAL = 0xAB;

Servo eggServo;
ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;

float currentTemp = 0;
float currentHumidity = 0;
bool heaterState = false;
bool atomizerState = false;
bool fanState = false;
bool servoEnabled = false;
unsigned long lastReadTime = 0;
unsigned long lastOtaCheck = 0;
unsigned long lastServoTurn = 0;

// Pulsating humidity control state
unsigned long atomizerPulseStart = 0;
bool atomizerPulsing = false;

// Fan timing state
unsigned long heaterLastChanged = 0;
bool heaterWasOn = false;
unsigned long atomizerLastChanged = 0;
bool atomizerWasOn = false;

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
    .slider { width: 100%; margin: 10px 0; }
    .status-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; }
    canvas { background: #fafafa; border: 1px solid #ddd; }
  </style>
</head>
<body>
  <h1>Egg Incubator</h1>
  <div class="card">
    <div class="label">Firmware Version</div>
    <div class="stat" id="version">--</div>
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
      <div class="label">Egg Turner (Servo)</div>
      <div class="stat" id="servo">OFF</div>
      <button class="btn btn-on" onclick="toggleDevice('servo','on')">ON</button>
      <button class="btn btn-off" onclick="toggleDevice('servo','off')">OFF</button>
    </div>
  </div>
  <div class="card">
    <div class="label">Target Settings</div>
    <div>Temp: 37.5°C | Humidity: 60%</div>
  </div>
  <div class="card">
    <div class="label">OTA Update</div>
    <button class="btn btn-auto" onclick="checkOta()">Check for Update</button>
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
        
        document.getElementById('heater').textContent = d.heater ? 'ON' : 'OFF';
        document.getElementById('heater').className = 'stat ' + (d.heater ? 'on' : 'off');
        
        document.getElementById('atomizer').textContent = d.atomizer ? 'ON' : 'OFF';
        document.getElementById('atomizer').className = 'stat ' + (d.atomizer ? 'on' : 'off');
        
        document.getElementById('fan').textContent = d.fan ? 'ON' : 'OFF';
        document.getElementById('fan').className = 'stat ' + (d.fan ? 'on' : 'off');
        
        document.getElementById('servo').textContent = d.servo ? 'ON' : 'OFF';
        document.getElementById('servo').className = 'stat ' + (d.servo ? 'on' : 'off');
        
        // Draw charts if log data available
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
      
      const maxTemp = 40;
      const minTemp = 35;
      
      ctx.beginPath();
      ctx.strokeStyle = '#ff5722';
      ctx.lineWidth = 2;
      
      for (let i = 0; i < logData.length; i++) {
        const x = (i / (logData.length - 1)) * w;
        const temp = logData[i].temp;
        const y = h - ((temp - minTemp) / (maxTemp - minTemp)) * h;
        if (i === 0) ctx.moveTo(x, y);
        else ctx.lineTo(x, y);
      }
      ctx.stroke();
      
      // Draw target line
      const targetY = h - ((37.5 - minTemp) / (maxTemp - minTemp)) * h;
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
      
      const maxHum = 80;
      const minHum = 40;
      
      ctx.beginPath();
      ctx.strokeStyle = '#2196F3';
      ctx.lineWidth = 2;
      
      for (let i = 0; i < logData.length; i++) {
        const x = (i / (logData.length - 1)) * w;
        const hum = logData[i].hum;
        const y = h - ((hum - minHum) / (maxHum - minHum)) * h;
        if (i === 0) ctx.moveTo(x, y);
        else ctx.lineTo(x, y);
      }
      ctx.stroke();
      
      // Draw target line
      const targetY = h - ((60 - minHum) / (maxHum - minHum)) * h;
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

void handleRoot() {
  server.send(200, "text/html", INDEX_HTML);
}

void handleData() {
  String json = "{\"temperature\":" + String(currentTemp) +
               ",\"humidity\":" + String(currentHumidity) +
               ",\"heater\":" + String(heaterState ? "true" : "false") +
               ",\"atomizer\":" + String(atomizerState ? "true" : "false") +
               ",\"fan\":" + String(fanState ? "true" : "false") +
               ",\"servo\":" + String(servoEnabled ? "true" : "false") +
               ",\"version\":\"" + String(firmwareVersion) + "\"";

  // Add log data for charts from filesystem
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
  WiFiClient wifiClient;
  http.begin(wifiClient, versionUrl);
  int httpCode = http.GET();
  
  if (httpCode == 200) {
    String remoteVersion = http.getString();
    remoteVersion.trim();
    bool hasUpdate = (remoteVersion != firmwareVersion);
    String json = "{\"update\":" + String(hasUpdate ? "true" : "false") + ",\"version\":\"" + remoteVersion + "\"}";
    server.send(200, "application/json", json);
  } else {
    server.send(200, "application/json", "{\"update\":false,\"version\":\"error\"}");
  }
  http.end();
}

void handleOtaUpdate() {
  HTTPClient http;
  WiFiClient wifiClient;
  http.begin(wifiClient, firmwareUrl);
  int httpCode = http.GET();
  
  if (httpCode == 200) {
    WiFiClient* stream = http.getStreamPtr();
    size_t contentLength = http.getSize();
    if (Update.begin(contentLength)) {
      if (Update.writeStream(*stream) && Update.end(true)) {
        server.send(200, "application/json", "{\"status\":\"success. Rebooting...\"}");
        delay(1000);
        ESP.restart();
      }
    }
  }
  http.end();
  server.send(200, "application/json", "{\"status\":\"update failed\"}");
}

void checkOtaAuto() {
  static bool otaInProgress = false;
  if (otaInProgress) return;
  
  HTTPClient http;
  WiFiClient wifiClient;
  http.begin(wifiClient, versionUrl);
  int httpCode = http.GET();
  
  if (httpCode == 200) {
    String remoteVersion = http.getString();
    remoteVersion.trim();
    
    if (remoteVersion != firmwareVersion) {
      Serial.print("New firmware: ");
      Serial.println(remoteVersion);
      otaInProgress = true;
      http.end();
      
      http.begin(wifiClient, firmwareUrl);
      httpCode = http.GET();
      
      if (httpCode == 200) {
        WiFiClient* stream = http.getStreamPtr();
        size_t contentLength = http.getSize();
        if (Update.begin(contentLength) && Update.writeStream(*stream) && Update.end(true)) {
          delay(1000);
          ESP.restart();
        }
      }
      http.end();
      otaInProgress = false;
    }
  }
  http.end();
}

void autoControl() {
  if (!isnan(currentTemp) && !isnan(currentHumidity)) {
    unsigned long now = millis();
    
    // Temperature control - turn on heater when cold
    if (currentTemp < TARGET_TEMP - TEMP_HYSTERESIS) {
      heaterState = true;
    } else if (currentTemp > TARGET_TEMP + TEMP_HYSTERESIS) {
      heaterState = false;
    }
    digitalWrite(RELAY_HEATER, heaterState ? HIGH : LOW);
    
    // Track heater state changes for fan timing
    if (heaterState != heaterWasOn) {
      heaterLastChanged = now;
      heaterWasOn = heaterState;
    }
    
    // Pulsating humidity control - 3s ON, 10s OFF
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
    
    // Track atomizer state changes for fan timing
    if (atomizerState != atomizerWasOn) {
      atomizerLastChanged = now;
      atomizerWasOn = atomizerState;
    }
    
    // Determine stability
    bool tempStable = (currentTemp >= TARGET_TEMP - TEMP_HYSTERESIS && 
                     currentTemp <= TARGET_TEMP + TEMP_HYSTERESIS);
    bool humStable = (currentHumidity >= TARGET_HUMIDITY - HUMIDITY_HYSTERESIS && 
                    currentHumidity <= TARGET_HUMIDITY + HUMIDITY_HYSTERESIS);
    
    // Fan control - ON/OFF only (no speed control)
    // ON when: heater ON, heater was ON within 5s, atomizer ON, atomizer was ON within 5s, or overheating
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

// RAM-based logging with EEPROM checkpoint for persistence
void initLogging() {
  EEPROM.begin(512);
  
  // Check for power cycle using magic bytes
  unsigned char magic = EEPROM.read(EEPROM_MAGIC);
  unsigned char check = EEPROM.read(EEPROM_CHECK);
  unsigned char savedIndex = EEPROM.read(EEPROM_INDEX);
  
  if (magic == MAGIC_VAL && check == CHECK_VAL && savedIndex <= MAX_LOG_ENTRIES) {
    // Valid checkpoint found - restore log index
    logIndex = savedIndex;
    logFull = (logIndex >= MAX_LOG_ENTRIES);
    Serial.println("Restored log data from EEPROM");
  } else {
    // No valid checkpoint - start fresh
    logIndex = 0;
    logFull = false;
    Serial.println("Starting fresh log");
  }
  
  Serial.print("Log entries: ");
  Serial.println(logIndex);
}

void saveCheckpoint() {
  // Save checkpoint to EEPROM
  EEPROM.write(EEPROM_MAGIC, MAGIC_VAL);
  EEPROM.write(EEPROM_INDEX, (unsigned char)logIndex);
  EEPROM.write(EEPROM_CHECK, CHECK_VAL);
  EEPROM.commit();
}

void logData() {
  // Store in RAM buffer
  if (logIndex < MAX_LOG_ENTRIES) {
    logBuffer[logIndex].timestamp = millis();
    logBuffer[logIndex].temperature = currentTemp;
    logBuffer[logIndex].humidity = currentHumidity;
    logBuffer[logIndex].heaterState = heaterState;
    logBuffer[logIndex].atomizerState = atomizerState;
    logBuffer[logIndex].fanState = fanState;
    logIndex++;
    if (logIndex >= MAX_LOG_ENTRIES) {
      logIndex = 0;
      logFull = true;
    }
  } else {
    // Circular buffer - overwrite oldest
    logIndex = 0;
    logFull = true;
    logBuffer[logIndex].timestamp = millis();
    logBuffer[logIndex].temperature = currentTemp;
    logBuffer[logIndex].humidity = currentHumidity;
    logBuffer[logIndex].heaterState = heaterState;
    logBuffer[logIndex].atomizerState = atomizerState;
    logBuffer[logIndex].fanState = fanState;
    logIndex++;
  }
  
  // Save checkpoint to EEPROM periodically
  saveCheckpoint();
}

void getLogDataForWeb(String& json) {
  int startIdx = logFull ? logIndex : 0;
  int count = logFull ? MAX_LOG_ENTRIES : logIndex;
  int entriesToShow = min(20, count);
  
  if (entriesToShow > 0) {
    json += ",\"log\":[";
    for (int i = 0; i < entriesToShow; i++) {
      int idx = (startIdx + i) % MAX_LOG_ENTRIES;
      json += "{\"t\":" + String(logBuffer[idx].timestamp) +
             ",\"temp\":" + String(logBuffer[idx].temperature, 1) +
             ",\"hum\":" + String(logBuffer[idx].humidity, 1) +
             ",\"h\":" + String(logBuffer[idx].heaterState ? "true" : "false") +
             ",\"a\":" + String(logBuffer[idx].atomizerState ? "true" : "false") +
             ",\"f\":" + String(logBuffer[idx].fanState ? "true" : "false") + "}";
      if (i < entriesToShow - 1) json += ",";
    }
    json += "]";
  } else {
    json += ",\"log\":[]";
  }
}

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

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  // First connect to get network details
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  // Get network info
  IPAddress localIP = WiFi.localIP();
  IPAddress gatewayIP = WiFi.gatewayIP();
  IPAddress subnetIP = WiFi.subnetMask();
  Serial.print("Got IP: ");
  Serial.println(localIP);
  
  // Now reconnect with static IP ending in .100
  localIP[3] = 100;
  WiFi.config(localIP, gatewayIP, subnetIP);
  WiFi.begin(ssid, password);
  delay(1000);
  
  Serial.print("Static IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("Firmware: ");
  Serial.println(firmwareVersion);

  httpUpdater.setup(&server);
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/control", handleControl);
  server.on("/ota/check", handleOtaCheck);
  server.on("/ota/update", handleOtaUpdate);
  server.begin();

  Serial.println("HTTP server started");
  Serial.println("Pins: Heater=D1, Atomizer=D2, Fan=D3, Servo=D5");
}

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

  // Data logging to filesystem for web interface charts
  if (millis() - lastLogTime >= LOG_INTERVAL) {
    logData();
    lastLogTime = millis();
  }

  if (servoEnabled) {
    rotateEggs();
  }

  if (millis() - lastOtaCheck > 3600000) {
    checkOtaAuto();
    lastOtaCheck = millis();
  }
}