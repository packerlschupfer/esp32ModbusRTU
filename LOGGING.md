# Logging Configuration

The esp32ModbusRTU library supports two logging modes:

## Default: ESP-IDF Logging

By default, the library uses ESP-IDF's built-in logging system:
- Log messages use the tag "ModbusRTU"
- Control log levels with `esp_log_level_set("ModbusRTU", ESP_LOG_LEVEL)`
- Available levels: ERROR, WARN, INFO, DEBUG, VERBOSE

## Optional: Custom Logger

To use a custom Logger implementation, define `MODBUS_USE_CUSTOM_LOGGER` in your build flags:

```ini
# platformio.ini
build_flags = 
    -D MODBUS_USE_CUSTOM_LOGGER
```

The library expects a `getLogger()` function that returns a Logger instance with the following interface:
```cpp
void log(esp_log_level_t level, const char* tag, const char* format, ...);
```

## Debug Logging

To enable verbose debug logging (packet details), define `MODBUS_RTU_DEBUG`:

```ini
build_flags = 
    -D MODBUS_RTU_DEBUG
```

## Log Messages

The library logs the following by default:
- **ERROR**: Timeout errors, CRC errors, invalid parameters
- **INFO**: Watchdog configuration, task initialization
- **DEBUG**: Detailed protocol messages (only with MODBUS_RTU_DEBUG)