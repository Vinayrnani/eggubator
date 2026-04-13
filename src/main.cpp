#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <DHT.h>

#define DHTPIN D4
#define DHTTYPE DHT22
#define RELAYPIN D1

const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

const float TARGET_TEMP = 37.5;
const float TEMP_HYSTERESIS = 0.5;

DHT dht(DHTPIN, DHTTYPE);
ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;

float currentTemp = 0;
float currentHumidity = 0;
bool relayState = false;
unsigned long lastReadTime = 0;

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
  </style>
</head>
<body>
  <h1>🥚 Egg Incubator</h1>
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
  <script>
    function updateData() {
      fetch('/data').then(r => r.json()).then(d => {
        document.getElementById('temp').textContent = d.temperature.toFixed(1) + '°C';
        document.getElementById('hum').textContent = d.humidity.toFixed(1) + '%';
        document.getElementById('relay').textContent = d.relay ? 'ON' : 'OFF';
        document.getElementById('relay').className = 'stat ' + (d.relay ? 'on' : 'off');
      });
    }
    function toggleRelay(state) {
      fetch('/relay?state=' + state).then(() => updateData());
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
               ",\"relay\":" + String(relayState ? "true" : "false") + "}";
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

  httpUpdater.setup(&server);
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/relay", handleRelay);
  server.begin();

  Serial.println("HTTP server started");
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
}