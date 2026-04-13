#!/bin/bash
# Build script for Egg Incubator ESP8266
# Uses system xtensa toolchain (gcc-xtensa-lx106)

set -e

FRAMEWORK=/root/.platformio/packages/framework-arduinoespressif8266
TOOLCHAIN=/usr/bin
BUILD_DIR=build

mkdir -p $BUILD_DIR

echo "Building Egg Incubator..."

$TOOLCHAIN/xtensa-lx106-elf-g++ \
  -Wall -Wextra \
  -D__ets__ -DICACHE_FLASH \
  -DARDUINO=10808 \
  -DARDUINO_ARCH_ESP8266 \
  -DESP8266 \
  -DNDEBUG \
  -std=gnu++11 \
  -fno-exceptions \
  -fno-rtti \
  -mlongints \
  -nostdlib \
  -u __assert_func \
  -mfix-esp-int-ll \
  -ffunction-sections \
  -fdata-sections \
  -I$FRAMEWORK/cores/esp8266 \
  -I$FRAMEWORK/cores/esp8266/umm_malloc \
  -I$FRAMEWORK/tools/sdk/libc/xtensa_lx106_elf/include \
  -I$FRAMEWORK/libraries/ESP8266WiFi/src \
  -I$FRAMEWORK/libraries/ESP8266WebServer/src \
  -I$FRAMEWORK/libraries/ESP8266HTTPUpdateServer/src \
  -I.pio/libdeps/nodemcuv2/DHT\ sensor\ library \
  -I.pio/libdeps/nodemcuv2/Adafruit\ Unified\ Sensor \
  -I$FRAMEWORK/libraries/Debug \
  -I$FRAMEWORK/libraries/SoftwareSerial \
  -I$FRAMEWORK/libraries/Hash \
  -I$FRAMEWORK/libraries/SPI \
  -I$FRAMEWORK/libraries/Wire \
  -DLWIP_OPEN_SRC \
  -DDEBUG_ESP_PORT=Serial \
  -I$FRAMEWORK/tools/sdk/include \
  -I$FRAMEWORK/tools/sdk/include/coap \
  -I$FRAMEWORK/tools/sdk/include/driver \
  -I$FRAMEWORK/tools/sdk/include/esp8266 \
  -I$FRAMEWORK/tools/sdk/include/esp8266/ccc \
  -I$FRAMEWORK/tools/sdk/include/esp8266/driver \
  -I$FRAMEWORK/tools/sdk/include/esp8266/eagle \
  -I$FRAMEWORK/tools/sdk/include/esp8266/esp \
  -I$FRAMEWORK/tools/sdk/include/esp8266/heap_layout \
  -I$FRAMEWORK/tools/sdk/include/esp8266/libc \
  -I$FRAMEWORK/tools/sdk/include/esp8266/lwip \
  -I$FRAMEWORK/tools/sdk/include/esp8266/lwip/eth \
  -I$FRAMEWORK/tools/sdk/include/esp8266/lwip/include \
  -I$FRAMEWORK/tools/sdk/include/esp8266/lwip/include/compat \
  -I$FRAMEWORK/tools/sdk/include/esp8266/lwip/include/lwip \
  -I$FRAMEWORK/tools/sdk/include/esp8266/lwip/netif \
  -I$FRAMEWORK/tools/sdk/include/esp8266/lwip/apps \
  -I$FRAMEWORK/tools/sdk/include/esp8266/openocd \
  -I$FRAMEWORK/tools/sdk/include/esp8266/ssl \
  -I$FRAMEWORK/tools/sdk/include/esp8266/smartconfig \
  -I$FRAMEWORK/tools/sdk/include/esp8266/spiffs \
  -I$FRAMEWORK/tools/sdk/include/esp8266/umm \
  -I$FRAMEWORK/tools/sdk/ld \
  -I$FRAMEWORK/tools/sdk/ld/eagle \
  -c src/main.cpp -o $BUILD_DIR/main.cpp.o

echo "Linking..."

$TOOLCHAIN/xtensa-lx106-elf-g++ \
  -nostdlib \
  -Wl,--gc-sections \
  -Wl,-static \
  -Wl,--relax \
  -specs=espressif8266_rom.specs \
  -Wl,-EL \
  -o $BUILD_DIR/firmware.elf \
  $BUILD_DIR/main.cpp.o \
  -L$TOOLCHAIN/lib/gcc/xtensa-lx106-elf/13.2.0 \
  -L$TOOLCHAIN/xtensa-lx106-elf/xtensa-lx106-elf \
  -L$FRAMEWORK/tools/sdk/lib \
  -lgcc \
  -lm \
  -lstdc++ \
  -lappgpio \
  -lapp_update \
  -lbootloader \
  -lbss \
  -lbtdiffsym \
  -ldriver \
  -lespnow \
  -lhomekit \
  -lmesh \
  -lminiudp \
  -lnet80211 \
  -lpp \
  -lpsram \
  -lsmartconfig \
  -lwifi \
  -lwpa \
  -lwpa2 \
  -ladv_dnt \
  -lwps \
  -lairkiss \
  -lsmart_types \
  -lwifi_provisioning \
  -lwps_parser \
  -lhttp_parser \
  -lmail \
  -lc \
  -lhal \
  -lhci \
  -lbt \
  -lbtdm \
  -lbt_common \
  -lbt_core \
  -lbt_driver \
  -lbt_host \
  -lbt_interface \
  -lbt_l2cap \
  -lbt_spp \
  -lbt_util \
  -lespnow \
  -lfrc2 \
  -llwip \
  -llwip6 \
  -llwip_536 \
  -llwip_5366 \
  -llwip_pbuf \
  -llwip_api \
  -llwip_dns \
  -llwip_dhcp \
  -llwip_dhcp6 \
  -llwip_netif \
  -llwip_raw_pcb \
  -llwip_sockets \
  -llwip_tcp \
  -llwip_udp \
  -llwip_raw \
  -llwip_core \
  -llwip_core4 \
  -llwip_core6 \
  -lmqtt \
  -lnet80211 \
  -lphy \
  -lpp \
  -lrtc \
  -lslc \
  -lsmartconfig \
  -lspiffs \
  -lulp \
  -lumm_newlib \
  -lupgrade \
  -lfairy \
  -lairkiss \
  -lcrypto \
  -ljson \
  -lupgrade \
  -lutils \
  -lwifimsk \
  -lx509 \
  -lbearssl \
  -lm \
  -lcrypto \
  -lmincrypt \
  -lwpa \
  -lnet80211 \
  -lpp \
  -lespnow \
  -lsmartconfig \
  -lairkiss \
  -ljson \
  -lupgrade \
  -lwpa2 \
  -lesp-tls \
  -lsmartconfig \
  -lmesh

echo "Creating firmware.bin..."

$TOOLCHAIN/xtensa-lx106-elf-objcopy \
  -O binary $BUILD_DIR/firmware.elf $BUILD_DIR/firmware.bin

echo "Done! Firmware: $BUILD_DIR/firmware.bin"
ls -la $BUILD_DIR/firmware.bin