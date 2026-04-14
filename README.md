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

## Build & Flash (Arduino CLI)

```bash
# Install Arduino CLI
curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR=/usr/local/bin sh

# Add ESP8266 core
arduino-cli config add board_manager.additional_urls https://arduino.esp8266.com/stable/package_esp8266com_index.json
arduino-cli core install esp8266:esp8266

# Install DHT library
arduino-cli lib install "DHT sensor library"

# Compile
arduino-cli compile -b esp8266:esp8266:nodemcuv2 eggubator.ino
```

### Flash via USB
```bash
arduino-cli upload -b esp8266:esp8266:nodemcuv2 -p /dev/ttyUSB0 eggubator.ino
```

### OTA Update (after initial flash)
```bash
arduino-cli upload -b esp8266:esp8266:nodemcuv2 -p <ESP8266_IP> --protocol espota eggubator.ino
```

## Web Interface

After flashing, access the web UI at `http://<ESP8266_IP>/`

## OTA Endpoint

For firmware updates via web browser: `http://<ESP8266_IP>/update`
