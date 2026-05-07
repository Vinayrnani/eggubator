# Egg Incubator Controller

An ESP8266-based automatic egg incubator controller with web interface, temperature/humidity control, and OTA updates.

## Hardware Setup

### Components
- **ESP8266 NodeMCU** - Main controller
- **DHT22** - Temperature & humidity sensor (pin D4)
- **Relay Module** (3-channel)
  - Heater relay (pin D1)
  - Atomizer/ultrasonic mist maker relay (pin D2)
  - Fan relay (pin D3)
- **Servo MG996R** - Egg turner (pin D5)
- **Power Supply** - 5V/2A for ESP8266, 12V for heater/atomizer

### Wiring
| Pin | Device | Connection |
|-----|--------|------------|
| D1 | Relay 1 | Heater (60W incandescent bulb) |
| D2 | Relay 2 | Atomizer (humidity) |
| D3 | Relay 3 | Fan (heat radiation + air circulation) |
| D4 | DHT22 | Temperature/Humidity sensor |
| D5 | Servo | Egg turning motor |

## Features

### Temperature Control
- Target: 37.5°C (±0.5°C hysteresis)
- Automatic heater on/off
- Overheat protection: fan ON when temp > 38°C

### Humidity Control (Pulsating Mode)
- Target: 60% RH
- **3 seconds ON → 10 seconds OFF** cycle
- Repeats until target humidity reached
- When humidity is high (> target + 5%), atomizer stays OFF

### Fan Control (Smart Timing)
- ON when heater is ON
- Continues **5 seconds after heater turns OFF**
- ON when atomizer is ON (spreads humidity)
- Continues **5 seconds after atomizer turns OFF**
- OFF only when **both temperature AND humidity are stable**
- ON when temperature > 38°C (overheat protection)

### Data Logging
- Logs to **EEPROM** (persists across power cycles)
- Stores last **100 entries** with timestamp, temp, humidity, device states
- Auto-purges oldest when full

### Web Interface
- Live temperature & humidity display
- Device controls (manual ON/OFF)
- **Temperature & humidity charts** (last 20 readings)
- **Uptime display**
- OTA firmware update

### Recovery System
- Tracks boot failures in EEPROM
- Auto-enters recovery mode after **3 consecutive failures**
- Web endpoints for manual recovery:
  - `/reboot` - Reboot device
  - `/rollback` - Reset boot counter
  - `/recovery` - Enter recovery mode
  - `/recovery/reset` - Exit recovery mode

## IP Configuration
- Connects to WiFi with DHCP first
- Then sets static IP ending with **`.100`** (e.g., 192.168.1.100)
- Subnet and gateway auto-detected from DHCP response

### Connectivity Workflow
If the device is not reachable, follow this sequence exactly:
1.  **Attempt mDNS**: Try opening `http://eggubator.local` in your browser.
2.  **Network Discovery**: If mDNS fails, use network tools (e.g., `ping -c 1 eggubator.local`, or `arp-scan -l`) to find the device.
3.  **Static IP**: If discovery fails, attempt connection directly via the static IP: `http://192.168.100.10`.
4.  **Stop & Report**: If none of the above work, the device is unreachable. Inform the user and stop; do not attempt further automated retries.


## Web Interface Endpoints

| Endpoint | Description |
|----------|-------------|
| `/` | Main web interface |
| `/data` | JSON sensor data with logs |
| `/control?device=X&state=Y` | Control devices (heater/atomizer/fan/servo) |
| `/ota/check` | Check for updates |
| `/ota/update` | Trigger OTA update |
| `/reboot` | Reboot device |
| `/rollback` | Reset boot counter |
| `/recovery` | Enter recovery mode |
| `/recovery/reset` | Exit recovery mode |

## OTA Updates

### Manual Update
Upload firmware via web interface at `http://<IP>/update`

### Auto Update
1. Host `firmware.bin` at your server
2. Host `version.txt` with version string (e.g., "1.2.2")
3. Update URLs in `updates.h`:
```cpp
#define FIRMWARE_URL "http://your-server/firmware.bin"
#define VERSION_URL "http://your-server/version.txt"
```

## File Structure

```
eggubator/
├── eggubator.ino       # Main sketch (~16KB)
├── config.h            # Configuration constants
├── dht_sensor.h       # DHT22 sensor functions (no library needed)
├── wifi_manager.h     # WiFi connection with static IP
├── logging.h          # EEPROM data logging
├── updates.h          # OTA update & recovery functions
└── firmware.bin       # Compiled firmware (~330KB)
```

## Modular Code

The project is modularized for maintainability:

- **config.h** - All constants (pins, targets, timing)
- **dht_sensor.h** - DHT22 reading without external library
- **wifi_manager.h** - WiFi connection with auto IP configuration
- **logging.h** - EEPROM persistence for data logging
- **updates.h** - OTA updates and boot recovery system

## Build & Flash (Arduino CLI)

**Note:** This project uses Arduino CLI for compilation. Arduino IDE is not supported.

### Compile (NodeMCU 0.9 ESP-12 Module)
```bash
arduino-cli compile -b esp8266:esp8266:nodemcu eggubator.ino
```

### Flash via USB
```bash
esptool.py --chip esp8266 --port /dev/ttyUSB0 --baud 115200 write_flash -z \
  --flash_size=4MB \
  --flash_mode=dio \
  --flash_freq=40m \
  0x00000 firmware.bin
```

### Update WiFi Credentials
Edit `wifi_manager.h`:
```cpp
#define WIFI_SSID "YourNetworkName"
#define WIFI_PASSWORD "YourPassword"
```

## Version History

| Version | Changes |
|---------|---------|
| **1.2.2** | Recovery system, uptime display, fixed degree symbol |
| **1.2.1** | Modular code structure, embedded DHT22 (no library) |
| **1.2.0** | Pulsating humidity (3s/10s), fan timing (5s), charts |
| **1.1.0** | Basic control with web interface, OTA |
