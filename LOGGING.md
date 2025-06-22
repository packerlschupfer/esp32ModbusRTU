# Logging Configuration

The esp32ModbusRTU library uses LogInterface for zero-overhead logging that supports both ESP-IDF and custom Logger implementations.

## Default: ESP-IDF Logging

By default (when `USE_CUSTOM_LOGGER` is not defined), the library uses ESP-IDF's built-in logging:
- Log messages use the tag "ModbusRTU"
- Control log levels with `esp_log_level_set("ModbusRTU", ESP_LOG_LEVEL)`
- Available levels: ERROR, WARN, INFO, DEBUG, VERBOSE
- Zero memory overhead - no Logger singleton created

## Optional: Custom Logger

To route logs through the custom Logger singleton:

### In your platformio.ini:
```ini
build_flags = 
    -D USE_CUSTOM_LOGGER  # Enable for all libraries using LogInterface
```

### In your main application:
```cpp
// Include these ONCE in your main.cpp
#include "Logger.h"
#include "LogInterfaceImpl.h"

// Include the ModbusRTU library
#include <esp32ModbusRTU.h>

void setup() {
    // Initialize Logger once for all libraries
    Logger::getInstance().init(1024);
    Logger::getInstance().setLogLevel(ESP_LOG_DEBUG);
    
    // Now esp32ModbusRTU logs through your Logger
    esp32ModbusRTU modbus(&Serial1, 16);
    modbus.begin();
}
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