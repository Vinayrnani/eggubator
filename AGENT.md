# Agent Instructions for Egg Incubator Project

## Core Principles

1. **Minimal non-side-effect code** - Keep changes focused and avoid unnecessary modifications
2. **Modular mode** - Maintain modular code structure (config.h, dht_sensor.h, wifi_manager.h, logging.h, updates.h)
3. **Always test** - Test in 2 different ways before proceeding
4. **Verify compilation** - Fix any compilation errors before deployment
5. **Verify deployment** - Access web page to confirm changes work correctly

## Testing Requirements

### 2 Different Testing Methods:

1. **Method 1**: Compile and check for errors/warnings
   ```bash
   arduino-cli compile -b esp8266:esp8266:nodemcu eggubator.ino
   ```

2. **Method 2**: After OTA deployment, verify via web interface
   - Access http://192.168.100.100/ (or current IP)
   - Check that changes are reflected correctly
   - Test functionality manually through the UI

## Workflow

### 1. Code Changes
- Make minimal changes focused on the specific requirement
- Keep modular structure intact
- Avoid introducing side effects

### 2. Compilation
```bash
arduino-cli compile -b esp8266:esp8266:nodemcu eggubator.ino
```
- Fix any compilation errors before proceeding
- Ensure no warnings that could cause issues
- Output: `build_output/eggubator.ino.bin`

### 3. Copy to ArduinoDroid
```bash
cp build_output/eggubator.ino.bin /sdcard/ArduinoDroid/sketchbook/firmware.bin
```

### 4. Deploy OTA
```bash
curl -F "firmware=@/sdcard/ArduinoDroid/sketchbook/firmware.bin" http://192.168.100.100/update
```

### 5. Verify Deployment (Required!)
After OTA update completes, ALWAYS verify:
1. Wait for ESP to reboot (~10 seconds)
2. Access web interface to check changes are working
3. Test the specific feature that was modified
4. Check serial monitor if available for any errors

### 6. Update README
Document any new features or changes in README.md

### 7. Update chat.log
Append this session's prompts and responses to chat.log:
```
### User: [prompt]

**[Action/Response]:** [what was done]
```


## Special Commands

| Command | Action |
|---------|--------|
| `uuu` | Update chat.log file with this session's prompts and responses |
| `PPP` | Commit and push to git repository |
| `ddd` | Deploy: Compile → Copy → Deploy OTA → Verify → Update README → Update chat.log |
| `DDD` | Same as ddd (full workflow) |

### DDD Command Details

**ddd** performs:
1. Compile firmware: `arduino-cli compile -b esp8266:esp8266:nodemcu eggubator.ino`
2. Copy to ArduinoDroid: `cp build_output/eggubator.ino.bin /sdcard/ArduinoDroid/sketchbook/firmware.bin`
3. Deploy OTA: `curl -F "firmware=@/sdcard/ArduinoDroid/sketchbook/firmware.bin" http://192.168.100.100/update`
4. Verify: Access web page to confirm deployment success
5. Update README: Document any new features or changes
6. Update chat.log: Record all prompts and responses from this session

**Important**: After running ddd/DDD, ALWAYS verify by accessing the web interface to ensure the ESP received and is running the new firmware correctly.

## Example Workflow

```bash
# 1. Make code changes to eggubator.ino or header files

# 2. Compile and verify (Method 1)
arduino-cli compile -b esp8266:esp8266:nodemcu eggubator.ino

# 3. Copy to ArduinoDroid
cp build_output/eggubator.ino.bin /sdcard/ArduinoDroid/sketchbook/firmware.bin

# 4. Deploy via OTA
curl -F "firmware=@/sdcard/ArduinoDroid/sketchbook/firmware.bin" http://192.168.100.100/update

# 5. Verify deployment (Method 2) - REQUIRED!
# Wait ~10s for ESP to reboot, then access:
# - http://192.168.100.100/ - Main page
# - http://192.168.100.100/mock - Mock settings page
# Check that changes are reflected correctly
```

## ESP Device Info

- **IP Address**: 192.168.100.100
- **OTA Endpoint**: http://192.168.100.100/update
- **Web Interface**: http://192.168.100.100/
- **Cloudflare Tunnel**: https://assured-coat-put-unlimited.trycloudflare.com (temporary, may change)

## Notes

- Always verify compilation succeeds before deployment
- ALWAYS verify deployment by accessing the web interface
- Test functionality through the UI after deployment
- Keep changes minimal to reduce risk of introducing bugs
- Maintain modular structure for maintainability
- Test in multiple ways when implementing new features
