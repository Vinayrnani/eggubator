# Flash Saving Architecture

## Overview
The EGGubator flash management system utilizes a circular buffer implemented directly in the ESP8266 flash memory. This architecture prioritizes **flash endurance** by eliminating mass sector erasures, treating the flash memory as a continuous ring buffer using explicit start/end pointers stored in EEPROM.

## Memory Structure
- **Flash Range:** Starts at `0x200000` (`FLASH_LOG_START`).
- **Sectors:** 256 sectors, each 4096 bytes (`LOG_SECTOR_SIZE`).
- **Capacity:** 512 entries per sector, totaling 131,072 entries.

## Circular Buffer Mechanism
To manage data without erasing flash memory unnecessarily, the system tracks two primary pointers in EEPROM:

1.  **`currentSector`**: The active sector where the system is writing new log entries.
2.  **`startSector`**: The oldest valid sector in the circular buffer.

### Log Advancement
When a sector becomes full (reaches `LOGS_PER_SECTOR`), the system advances to the next sector:
```cpp
currentSector = (currentSector + 1) % FLASH_NUM_SECTORS;
```

### Buffer Wrap-Around
If the `currentSector` advances and equals the `startSector`, the buffer is full. To continue logging, the system automatically advances the `startSector`, effectively discarding the oldest data block without a full erase:
```cpp
if (currentSector == startSector) {
  startSector = (startSector + 1) % FLASH_NUM_SECTORS;
  EEPROM.put(EEPROM_START_SECTOR, startSector);
}
```

### Clearing Logs (New "Batch")
The `clearLogs()` function now performs an instantaneous pointer update rather than an intensive flash erasure. It defines a new "batch" boundary by advancing the `startSector` and `currentSector` to a fresh, clean sector, ensuring subsequent reads only process valid, current data.

## Boot Session Indexing
- The system maintains a dynamic `bootIndex` in RAM, which tracks the start position (sector and offset) of every boot session.
- Unlike previous versions which had a fixed limit (e.g., 32 sessions), the index is now dynamically allocated and grows as needed to accommodate all boot sessions available in the flash memory.
- During initialization, the system scans forward from the `startSector` to reconstruct this index, ensuring all historical data remains accessible regardless of the number of reboots.

## Advantages
- **Performance:** Clearing logs is nearly instantaneous.
- **Flash Longevity:** Significantly reduces wear by eliminating mass flash erasures.
- **Data Integrity:** Explicit pointers (`current` and `start`) ensure the system always knows the exact range of valid data, preventing the retrieval of extraneous "garbage" logs from old sessions.

