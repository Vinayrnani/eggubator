# Makefile for Egg Incubator ESP8266

ARDUINO_VERSION = 3.30102
FRAMEWORK_DIR = /root/.platformio/packages/framework-arduinoespressif8266
CC = xtensa-lx106-elf-g++
CXX = xtensa-lx106-elf-g++

CFLAGS = -Wall -Wextra -mlongints -D__ets__ -DICACHE_FLASH -DARDUINO=10808 -DARDUINO_ARCH_ESP8266 -DESP8266_CORE_MAJOR=3 -DESP8266_CORE_MINOR=1 -DNDEBUG '-DPLATFORMIO=60119' -std=gnu99 -fno-tree-sra -fno-devirtual-method -nostdlib -u __assert_func -mfix-esp-int-ll -ffunction-sections -fdata-sections
CPPFLAGS = -fno-exceptions -fno-rtti

SDK_LIBS = -lwpa -lsmartconfig -lairkiss -lpp -lnet80211 -llwip_open -lwifimsk -lespnow -ljson -lupgrade -lstdc++ -lm

SRC = src/main.cpp
BUILD_DIR = build
FW = $(BUILD_DIR)/firmware.bin

include $(FRAMEWORK_DIR)/Arduino.mk

$(FW): $(SRC)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) -I$(FRAMEWORK_DIR)/cores/esp8266 -I$(FRAMEWORK_DIR)/libraries/ESP8266WiFi/src -I$(FRAMEWORK_DIR)/libraries/ESP8266WebServer/src -I$(FRAMEWORK_DIR)/libraries/ESP8266HTTPUpdateServer/src $(SRC) -o $(FW) $(SDK_LIBS)

flash: $(FW)
	esptool.py --chip esp8266 write_flash 0x00000 $(FW)

ota:
	@echo "OTA update ready - use Arduino IDE or platformio run --target upload --upload-port IP_ADDRESS"