# Egg Incubator with NodeMCU ESP8266

An IoT-based egg incubator system using NodeMCU ESP8266 WiFi module for remote monitoring and control.

## Features

- Temperature monitoring and control (DHT22 sensor)
- Humidity control  
- WiFi connectivity for remote access
- Web-based UI for monitoring
- Relay control for heating element
- OTA (Over-The-Air) updates support

## Hardware

- NodeMCU ESP8266
- DHT22 Temperature/Humidity Sensor (GPIO D4)
- Relay Module (GPIO D1)
- Heating element
- Power supply

## Build & Flash

### Initial USB Flash
```bash
# Edit WiFi credentials in src/main.cpp first
pio run -t upload -d /dev/ttyUSB0
```

### OTA Update (after initial flash)
```bash
pio run -e nodemcuv2_ota -t upload --upload-port <ESP8266_IP>
```

## Web Interface

After flashing, access the web UI at `http://<ESP8266_IP>/`

## OTA Endpoint

For firmware updates via web browser: `http://<ESP8266_IP>/update`
