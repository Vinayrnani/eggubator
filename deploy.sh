#!/bin/bash
set -e

echo "Compiling firmware..."
mkdir -p build
arduino-cli compile -b esp8266:esp8266:nodemcu -j 0 --build-path build/.cache --output-dir build eggubator.ino

echo "Copying firmware to firmware.bin..."
cp build/eggubator.ino.bin firmware.bin

echo "Finding IP for eggubator.local..."
IP=$(ping -c 1 eggubator.local | grep -oE '[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+' | head -n 1)

if [ -z "$IP" ]; then
    echo "Could not find IP for eggubator.local"
    exit 1
fi

echo "Found IP: $IP"
echo "Deploying to $IP via OTA..."
curl -X POST -F "image=@firmware.bin" http://$IP/update

echo -e "\nDeployment complete."
