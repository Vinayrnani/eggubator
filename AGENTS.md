# AGENTS.md — Egg Incubator (ESP8266)

Single-file Arduino firmware for a DHT22-based egg incubator with web UI, flash data logging, and OTA updates.

## Build & Deploy

```bash
# Compile
arduino-cli compile -b esp8266:esp8266:nodemcu -j "$(nproc)" --build-path build/.cache --output-dir build eggubator.ino
cp build/eggubator.ino.bin firmware.bin

# Deploy via OTA (mDNS)
./deploy.sh                          # compile + find IP via ping + curl to /update

# Flash via USB (Termux/OTG)
./flash.sh                           # auto-detect /dev/ttyUSB*, uses esptool.py

# Full release (bump → compile → commit → tag → GitHub Release → OTA)
./rel.sh [VERSION]                   # auto-increment patch if no VERSION arg
./rel-nd.sh [VERSION]                # same without OTA deploy
```

- **`rel.sh`** bumps `updates.h` + `version.txt`, compiles, commits, tags `vX.Y.Z`, creates GitHub Release with `firmware.bin`, then deploys OTA.
- `firmware.bin` is in `.gitignore` but release scripts `git add` it explicitly.

## Hardware Conventions

| Component | Pin | Active |
|-----------|-----|--------|
| Heater    | D1  | LOW = ON |
| Atomizer  | D2  | LOW = ON |
| Fan       | D3  | LOW = ON |
| Servo     | D5  | PWM (544-2450µs) |
| DHT22     | D4  | — |

**Relays are active LOW.** `digitalWrite(pin, HIGH)` = OFF, `LOW` = ON. Getting this wrong can overheat or damage hardware.

Network: mDNS `eggubator.local`, static IP ends in `.72`. Falls back to AP mode (`EGGubator` SSID) if WiFi fails.

## Verification

No automated test runner exists. Verify by:
1. **Compile** — must succeed with zero errors.
2. **Browser** — open `http://eggubator.local/`, check dashboard loads, status JSON at `/status`.
3. **Manual Playwright tests** at `test/playwright/test_*.js` — run with `node test_xxx.js` (requires device reachable).

Use mock mode for hardware-free dev: hit `/settings/api?enable=1&temp=37.5&hum=55` or enable auto-sim at `/settings/api?autosim=1`.

## File Map & Ownership

| File | Purpose |
|------|---------|
| `eggubator.ino` | Main setup/loop, web handlers, auto-control logic |
| `config.h` | Pin defs, WiFi creds, hysteresis thresholds |
| `dht_sensor.h` | DHT22 read + physics simulation (mock/auto-sim) |
| `wifi_manager.h` | WiFi connect with AP fallback + DNS |
| `logging.h` / `.cpp` | Flash circular buffer (256 sectors, 131K entries at `0x200000`) |
| `sat_manager.h` / `.cpp` | Boot session tracking, absolute time recovery across reboots |
| `updates.h` | OTA check + download from GitHub releases |
| `web_ui.h` | All HTML/CSS/JS in one PROGMEM string (Chart.js + Dexie.js) |
| `sector_viewer.h` | Flash hex editor tool |

## Architecture Notes

- **Timing variables** (`LOG_INTERVAL`, `EGG_TURN_INTERVAL`, `PULSE_ON/OFF_TIME`, `TARGET_TEMP/HUMIDITY`) are globals modifiable via web, not compile-time constants.
- **Flash logging**: circular buffer at `0x200000`, 256 sectors × 4096 bytes. Boot ID and sector pointers recovered entirely from flash — no per-boot EEPROM writes. EEPROM used only for settings changes and SAT drift corrections.
- **SAT (Synchronised Absolute Timestamp)**: browser syncs timeline across power cycles via boot table at `/timestamps` (GET + PUT). `startTimestamp` + `getElapsedSeconds()` → incubation day.
- **Servo**: 32 steps × 6°, center at step 15 (90°). Sweep endpoint configurable via `angleAdjustMin/Max`. Stage lockdown (day 18+) disables turning; servo moves to center.
- **Web UI is monolithic** — all HTML/CSS/JS lives inside `web_ui.h` as PROGMEM string literals (`INDEX_HTML`, `SETTINGS_HTML`, `DEXIE_HTML`). Edit raw HTML embedded in C.
- **No external Arduino libraries beyond the ESP8266 core** plus `Servo.h` and `DHT.h`. DHT has a local bit-banged fallback.

## Connectivity Workflow (device unreachable)

1. Browser → `http://eggubator.local`
2. `ping -c 1 eggubator.local` or `arp-scan -l`
3. Try `http://192.168.X.72` (X = subnet from DHCP)
4. If all fail, report unreachable; do not retry automatically.

## Web Endpoints

| Path | Method | Purpose |
|------|--------|---------|
| `/status` | GET | JSON: temp, humidity, device states, version, boot info |
| `/data` | GET | JSON sensor log (pagination via `boot`, `time`, `count` params) |
| `/settings/api` | GET/POST | Get or set mock/autosim, timing, stage, servo angles |
| `/control?device=X&mode=Y` | GET | Override device: `off` = kill, anything else = auto |
| `/settings/clear` | GET | Erase all flash logs, reset boot ID, reboot |
| `/ota/check` | GET | Compare FIRMWARE_VERSION vs GitHub release |
| `/ota/apply` | POST | Download and flash firmware.bin from GitHub |
| `/timestamps` | GET/PUT | SAT boot table sync |
| `/reboot` | GET | `ESP.restart()` |
| `/update` | POST | ESP8266HTTPUpdateServer (used by deploy scripts) |

## Gotchas

- `config.h` has actual WiFi credentials — don't commit changes to it.
- `version.txt` must match `FIRMWARE_VERSION` in `updates.h`.
- `eggubator.ino` uses 112-byte `DeviceSettings` struct saved to EEPROM starting at address 40.
- Servo pin (D5 = GPIO14) is held LOW during boot to suppress SPI noise.
- DHT sensor fallback: if `isnan()`, returns last valid reading; temp/hum validation also requires > 0.
- `firmware.bin` is gitignored but release scripts need it committed. It's OK to stage it explicitly.
- No CI config, no linter, no formatter config found in the repo.
