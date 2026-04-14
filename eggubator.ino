#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266HTTPClient.h>
#include <DHT.h>

#define DHTPIN D4
#define DHTTYPE DHT22
#define RELAYPIN D1

const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

const char* firmwareUrl = "http://YOUR_SERVER/firmware.bin";
const char* versionUrl = "http://YOUR_SERVER/version.txt";

const char* firmwareVersion = "1.0.0";

const float TARGET_TEMP = 37.5;
const float TEMP_HYSTERESIS = 0.5;

DHT dht(DHTPIN, DHTTYPE);
ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;

float currentTemp = 0;
float currentHumidity = 0;
bool relayState = false;
unsigned long lastReadTime = 0;
unsigned long lastOtaCheck = 0;

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
    .btn { padding: 10px 20px; margin: 5px; border: none; border-radius: 5px; cursor: pointer; }
    .btn-on { background: #4CAF50; color: white; }
    .btn-off { background: #f44336; color: white; }
    .progress { width: 100%; height: 20px; background: #ddd; border-radius: 5px; }
    .progress-bar { height: 100%; background: #4CAF50; border-radius: 5px; width: 0%; }
  </style>
</head>
<body>
  <h1>Egg Incubator</h1>
  <div class="card">
    <div class="label">Current Version</div>
    <div class="stat" id="version">1.0.0</div>
  </div>
  <div class="card">
    <div class="label">Temperature</div>
    <div class="stat" id="temp">--°C</div>
  </div>
  <div class="card">
    <div class="label">Humidity</div>
    <div class="stat" id="hum">--%</div>
  </div>
  <div class="card">
    <div class="label">Heater Status</div>
    <div class="stat" id="relay">OFF</div>
    <button class="btn btn-on" onclick="toggleRelay('on')">ON</button>
    <button class="btn btn-off" onclick="toggleRelay('off')">OFF</button>
  </div>
  <div class="card">
    <div class="label">Target Temperature</div>
    <div class="stat">37.5°C</div>
  </div>
  <div class="card">
    <div class="label">OTA Update</div>
    <button class="btn btn-on" onclick="checkOta()">Check for Update</button>
    <div class="progress"><div class="progress-bar" id="progress"></div></div>
    <div id="otaStatus"></div>
  </div>
  <script>
    function updateData() {
      fetch('/data').then(r => r.json()).then(d => {
        document.getElementById('version').textContent = d.version;
        document.getElementById('temp').textContent = d.temperature.toFixed(1) + '°C';
        document.getElementById('hum').textContent = d.humidity.toFixed(1) + '%';
        document.getElementById('relay').textContent = d.relay ? 'ON' : 'OFF';
        document.getElementById('relay').className = 'stat ' + (d.relay ? 'on' : 'off');
      });
    }
    function toggleRelay(state) {
      fetch('/relay?state=' + state).then(() => updateData());
    }
    function checkOta() {
      document.getElementById('otaStatus').textContent = 'Checking for update...';
      fetch('/ota/check').then(r => r.json()).then(d => {
        document.getElementById('otaStatus').textContent = d.status;
        if (d.update) {
          document.getElementById('otaStatus').textContent = 'Update available: ' + d.version + '. Downloading...';
          fetch('/ota/update').then(r => r.json()).then(r => {
            document.getElementById('progress').style.width = '100%';
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
               ",\"relay\":" + String(relayState ? "true" : "false") +
               ",\"version\":\"" + String(firmwareVersion) + "\"}";
  server.send(200, "application/json", json);
}

void handleRelay() {
  if (server.hasArg("state")) {
    String state = server.arg("state");
    relayState = (state == "on");
    digitalWrite(RELAYPIN, relayState ? HIGH : LOW);
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
    
    bool hasUpdate = false;
    if (remoteVersion != firmwareVersion) {
      hasUpdate = true;
    }
    
    String json = "{\"update\":" + String(hasUpdate ? "true" : "false") +
                ",\"version\":\"" + remoteVersion + "\"}";
    server.send(200, "application/json", json);
  } else {
    server.send(200, "application/json", "{\"update\":false,\"version\":\"error\"}");
  }
  http.end();
}

void handleOtaUpdate() {
  server.send(200, "text/plain", "Starting OTA update...");
  
  HTTPClient http;
  WiFiClient wifiClient;
  http.begin(wifiClient, firmwareUrl);
  int httpCode = http.GET();
  
  if (httpCode == 200) {
    WiFiClient* stream = http.getStreamPtr();
    size_t contentLength = http.getSize();
    
    if (Update.begin(contentLength)) {
      size_t written = Update.writeStream(*stream);
      if (Update.end(true)) {
        Serial.println("OTA update complete. Rebooting...");
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
      Serial.print("New firmware available: ");
      Serial.println(remoteVersion);
      Serial.println("Starting auto-update...");
      
      otaInProgress = true;
      http.end();
      
      http.begin(wifiClient, firmwareUrl);
      httpCode = http.GET();
      
      if (httpCode == 200) {
        WiFiClient* stream = http.getStreamPtr();
        size_t contentLength = http.getSize();
        
        if (Update.begin(contentLength)) {
          size_t written = Update.writeStream(*stream);
          if (Update.end(true)) {
            Serial.println("Auto-update complete. Rebooting...");
            delay(1000);
            ESP.restart();
          }
        }
      }
      http.end();
      otaInProgress = false;
    }
  }
  http.end();
}

void setup() {
  Serial.begin(115200);
  pinMode(RELAYPIN, OUTPUT);
  digitalWrite(RELAYPIN, LOW);

  dht.begin();

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("Firmware version: ");
  Serial.println(firmwareVersion);

  httpUpdater.setup(&server);
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/relay", handleRelay);
  server.on("/ota/check", handleOtaCheck);
  server.on("/ota/update", handleOtaUpdate);
  server.begin();

  Serial.println("HTTP server started");
  Serial.println("OTA endpoints:");
  Serial.println("  /ota/check - Check for updates");
  Serial.println("  /ota/update - Manual update");
}

void loop() {
  server.handleClient();

  if (millis() - lastReadTime > 2000) {
    float t = dht.readTemperature();
    float h = dht.readHumidity();

    if (!isnan(t) && !isnan(h)) {
      currentTemp = t;
      currentHumidity = h;

      if (currentTemp < TARGET_TEMP - TEMP_HYSTERESIS) {
        relayState = true;
      } else if (currentTemp > TARGET_TEMP + TEMP_HYSTERESIS) {
        relayState = false;
      }
      digitalWrite(RELAYPIN, relayState ? HIGH : LOW);
    }

    lastReadTime = millis();
  }

  if (millis() - lastOtaCheck > 3600000) {
    checkOtaAuto();
    lastOtaCheck = millis();
  }
}