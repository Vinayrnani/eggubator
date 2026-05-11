# Logging Architecture

## Overview
The EGGubator logging system utilizes a circular buffer implemented directly in the ESP8266 flash memory. This approach allows for persistent data storage while optimizing for flash endurance by minimizing unnecessary erasures.

## Memory Structure
- **Flash Range:** Starts at `0x200000` (`FLASH_LOG_START`).
- **Sectors:** 256 sectors, each 4096 bytes (`LOG_SECTOR_SIZE`).
- **Entries:** Each log entry is a packed struct of 8 bytes (`LogEntry`).
- **Capacity:** 512 entries per sector, totaling 131,072 entries across all sectors.

## Circular Buffer Mechanism
To manage data without erasing the entire flash memory constantly, the system tracks two primary pointers in EEPROM:

1.  **`currentSector`**: Tracks the sector where the system is currently writing new log entries.
2.  **`startSector`**: Tracks the oldest valid sector in the circular buffer.

### Log Advancement
When a sector becomes full (reaches `LOGS_PER_SECTOR`), the system advances to the next sector:
```cpp
currentSector = (currentSector + 1) % FLASH_NUM_SECTORS;
```

### Buffer Wrap-Around
If the `currentSector` advances and equals the `startSector`, it means the buffer is full. To continue logging, the system automatically advances the `startSector`, effectively discarding the oldest data block:
```cpp
if (currentSector == startSector) {
  startSector = (startSector + 1) % FLASH_NUM_SECTORS;
  EEPROM.put(EEPROM_START_SECTOR, startSector);
}
```

### Clearing Logs
The `clearLogs()` function now performs an instantaneous pointer update rather than an intensive flash erasure:
1.  Sets `startSector` to `(currentSector + 1) % FLASH_NUM_SECTORS`.
2.  Sets `currentSector` equal to the new `startSector`.
3.  Resets `currentOffset` to 0.
4.  Updates the pointers in EEPROM.
5.  Erases only the new active sector to provide a clean writing area.

## Advantages
- **Performance:** Clearing logs is nearly instantaneous.
- **Flash Longevity:** Reduces wear by eliminating mass flash erasures.
- **Reliability:** Explicit pointers (`current` and `start`) ensure the system always knows the exact range of valid data, preventing the retrieval of "garbage" logs from old sessions.
- **Robustness:** Using `uint16_t` for sector indexing avoids integer overflow issues and supports future scaling.
