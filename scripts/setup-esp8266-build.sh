#!/usr/bin/env bash
set -euo pipefail

# Setup ESP8266 compile environment and build firmware variants.
# Intended for x86_64 hosts. This script uses PlatformIO to download
# ESP8266 core and build both nodemcuv2 and nodemcuv2_ota variants.

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

echo "[ESP8266 Build] Ensuring Python3 and PlatformIO..."
if ! command -v python3 >/dev/null 2>&1; then
  echo "Python3 is required but not found. Install Python3 and re-run." >&2
  exit 1
fi

if ! command -v pio >/dev/null 2>&1; then
  echo "PlatformIO not found. Installing via pip..."
  python3 -m pip install --break-system-packages --user platformio
  export PATH="$HOME/.local/bin:$PATH"
fi

echo "[ESP8266 Build] Installing ESP8266 platform..."
pio platform install espressif8266

echo "[ESP8266 Build] Building nodemcuv2..."
pio run -e nodemcuv2

echo "[ESP8266 Build] Building nodemcuv2_ota..."
pio run -e nodemcuv2_ota

echo "Done. Artifacts will be in .pio/build/<env>/firmware.bin" 
echo "You can upload via USB or OTA as configured in platformio.ini."
