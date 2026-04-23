# Egg Incubator Controller

An ESP8266-based automatic egg incubator controller with web interface, temperature/humidity control, mock sensor testing, and OTA updates.

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
- Target: **37.5°C** (±0.5°C hysteresis)
- Automatic heater on/off
- Overheat protection: fan ON when temp > 38°C

### Humidity Control (Pulsating Mode)
- Target: **55% RH** (incubation), **65% RH** (lockdown)
- **2 seconds ON → 10 seconds OFF** cycle (configurable)
- Repeats until target humidity reached
- When humidity is high (> target + 5%), atomizer stays OFF

### Fan Control (Smart Timing)
- ON when heater is ON
- Continues **5 seconds after heater turns OFF**
- ON when atomizer is ON (spreads humidity)
- Continues **5 seconds after atomizer turns OFF**
- OFF only when **both temperature AND humidity are stable**
- ON when temperature > 38°C (overheat protection)

### Egg Turner
- Rotates eggs every **2 hours** (configurable: 15min - 6 hours)
- 10-second sweep animation
- **Automatically disabled during lockdown stage**

### Incubation Stages
| Stage | Days | Temp | Humidity |
|-------|------|------|---------|
| Incubation | 1-18 | 37.5°C | 55% |
| Lockdown | 19-21 | 37.5°C | 65% |

### Data Logging
- Logs to **RAM** (500 entries circular buffer)
- Stores timestamp, temp, humidity, device states
- Save to flash when ~390 entries OR interval reached (whichever first)
- Flash storage: 1MB (256 sectors × 4KB), circular
- **Settings persist in EEPROM** across reboots

### Web Interface
- Live temperature & humidity display
- **Per-device control** (each device: OFF/AUTO toggle)
- Temperature & humidity charts
- **Controls state chart** (heater, atomizer, fan, servo)
- **Uptime display**
- **Incubation stage selector**
- OTA firmware update
- Settings page for timing/mock configuration

### Mock Sensor & Auto-Simulation
- **Mock Sensor Mode**: Set manual temp/hum values for testing
- **Auto Simulation**: Physics-based simulation with:
  - Ambient temp: 32°C, Ambient humidity: 35%
  - Target-following when devices ON
  - Fan cooling effect
  - Noise injection (±0.1°C temp, ±1% humidity)

### Recovery System
- Tracks boot failures in EEPROM
- Auto-enters recovery mode after **3 consecutive failures**
- Web endpoints for manual recovery:
  - `/reboot` - Reboot device
  - `/rollback` - Trigger rollback
  - `/recovery` - Enter recovery mode
  - `/recovery/reset` - Exit recovery mode

## IP Configuration

- Connects to WiFi with DHCP first
- Then sets static IP ending with **`.100`** (e.g., 192.168.1.100)
- Subnet and gateway auto-detected from DHCP response

## Web Interface Endpoints

| Endpoint | Description |
|----------|-------------|
| `/` | Main dashboard |
| `/mock` | Settings & mock configuration |
| `/data` | JSON sensor data with logs |
| `/control?device=X&mode=Y` | Control device (heater/atomizer/fan/servo: off/auto) |
| `/mock/api` | Mock sensor & timing APIs |
| `/vendor/dexie-3.2.7.min.js` | Vendored Dexie asset served from firmware |
| `/ota/check` | Check for updates |
| `/ota/update` | Trigger OTA update |
| `/reboot` | Reboot device |
| `/rollback` | Trigger rollback |
| `/recovery` | Enter recovery mode |
| `/recovery/reset` | Exit recovery mode |

## OTA Updates

### Manual Update
Upload firmware via web interface at `http://<IP>/update`

### Auto Update
1. Host `firmware.bin` at your server
2. Host `version.txt` with version string (e.g., "1.2.3")
3. Update URLs in `updates.h`:
```cpp
#define FIRMWARE_URL "http://your-server/firmware.bin"
#define VERSION_URL "http://your-server/version.txt"
```

## File Structure

```
eggubator/
├── eggubator.ino       # Main sketch (~33KB)
├── config.h            # Configuration constants
├── dht_sensor.h       # DHT22 sensor with mock/simulation
├── wifi_manager.h     # WiFi connection with static IP
├── logging.h          # RAM/flash data logging
├── updates.h          # OTA update & recovery functions
├── web_ui.h           # Shared dashboard/settings HTML served by firmware
├── dexie_asset.h      # Vendored Dexie bundle served locally from firmware
└── firmware.bin       # Compiled firmware (~330KB)
```

## Modular Code

The project is modularized for maintainability:

- **config.h** - Pins, targets, timing constants
- **dht_sensor.h** - DHT22 with mock & auto-simulation
- **wifi_manager.h** - WiFi with auto IP configuration
- **logging.h** - RAM circular buffer + flash persistence
- **updates.h** - OTA updates and boot recovery
- **web_ui.h** - Shared dashboard/settings pages served from firmware
- **dexie_asset.h** - Pinned Dexie asset served locally from firmware

## Build & Flash (Arduino CLI)

```bash
# Compile (NodeMCU 0.9 ESP-12 Module)
arduino-cli compile -b esp8266:esp8266:nodemcu eggubator.ino

# Flash via USB
esptool.py --chip esp8266 --port /dev/ttyUSB0 --baud 115200 write_flash -z \
  --flash_size=4MB \
  --flash_mode=dio \
  --flash_freq=40m \
  0x00000 firmware.bin
```

## Configurable Timing (via /mock page)

| Setting | Options |
|---------|---------|
| Log Interval | 5s - 30s |
| Save to Flash | 60min - 4hrs |
| Atomizer Pulse | 2s - 5s |
| Egg Turner | 15min - 6hrs |

## Version History

| Version | Changes |
|---------|---------|
| **1.3.0** | Chart.js integration: Replace custom canvas charts with Chart.js library from CDN, added data downsampling for performance |
| **1.2.9** | System info: RAM records count, RAM storage (bytes), flash sector, sectors used, local Dexie count |
| **1.2.8** | Log buffer reduced: 500 → 400 entries (~1.2KB RAM savings) |
| **1.2.7** | Dexie library loaded from CDN instead of embedded (~26KB flash savings) |
| **1.2.6** | Smart sync: ticker for large gaps (>200), countdown timer for normal, dynamic interval from /mock/api |
| **1.2.5** | Fix: EEPROM settings persistence - call loadSettings AFTER initRecovery |
| **1.2.4** | EEPROM persistence for settings, flash logging with sector management |
| **1.2.3** | Mock sensor, auto-simulation, per-device control, configurable timing, incubation stages |
| **1.2.2** | Recovery system, uptime display, fixed degree symbol |
| **1.2.1** | Modular code structure, embedded DHT22 (no library) |
| **1.2.0** | Pulsating humidity (3s/10s), fan timing (5s), charts |
| **1.1.0** | Basic control with web interface, OTA |