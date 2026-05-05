# Agent Instructions for Egg Incubator Project

## Core Principles

1. **Minimal non-side-effect code** - Keep changes focused and avoid unnecessary modifications
2. **Modular mode** - Maintain modular code structure (config.h, dht_sensor.h, wifi_manager.h, logging.h, updates.h)
3. **Always test** - Test in 2 different ways before proceeding
4. **Verify compilation** - Fix any compilation errors before deployment

## Workflow

### 1. Code Changes
- Make minimal changes focused on the specific requirement
- Keep modular structure intact
- Avoid introducing side effects

### 2. Testing (2 Different Ways)
- **Method 1**: Compile and check for errors
- **Method 2**: Verify with static analysis or logic check

### 3. Compilation & Error Fixing
```bash
arduino-cli compile -b esp8266:esp8266:nodemcu eggubator.ino
```
- Fix any compilation errors before proceeding
- Ensure no warnings that could cause issues
- Output: `build_output/eggubator.ino.bin` → copy to `firmware.bin`

### 4. Copy Files to ArduinoDroid (Android)
```bash
cp build_output/eggubator.ino.bin firmware.bin
cp eggubator.ino config.h *.h firmware.bin /storage/emulated/0/ArduinoDroid/sketchbook/
```

## Special Commands

| Command | Action |
|---------|--------|
| `uuu` | Update chat.log file with prompt and response after every prompt |
| `PPP` | 1. Increment FIRMWARE_VERSION in updates.h 2. Commit and push to git repository |
| `ddd` | Deploy the compiled binary via OTA update to ESP device (192.168.100.100) |
| `DDD` | 1. Compile firmware 2. Deploy OTA |

## Example Workflow

```bash
# 1. Make code changes to eggubator.ino or header files

# 2. Compile and verify
arduino-cli compile -b esp8266:esp8266:nodemcu --output-dir build_output eggubator.ino

# 3. Copy output binary to firmware.bin
cp build_output/eggubator.ino.bin firmware.bin

# 4. Deploy via OTA (DDD)
curl -s -F "update=@firmware.bin" http://192.168.100.100/update
```

## Notes

- Always verify compilation succeeds before deployment
- Keep changes minimal to reduce risk of introducing bugs
- Maintain modular structure for maintainability
- Test in multiple ways when implementing new features
