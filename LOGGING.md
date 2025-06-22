# Logging Configuration

The esp32ModbusRTU library supports flexible zero-overhead logging with C++11 compatibility. It can use either ESP-IDF logging (default) or custom Logger without forcing any dependencies.

## Default: ESP-IDF Logging

By default, the library uses ESP-IDF's built-in logging system:
- Log messages use the tag "ModbusRTU"
- Control log levels with `esp_log_level_set("ModbusRTU", ESP_LOG_LEVEL)`
- Available levels: ERROR, WARN, INFO, DEBUG, VERBOSE
- Zero memory overhead - no external dependencies

Example:
```cpp
#include <esp32ModbusRTU.h>

void setup() {
    // Library will use ESP-IDF logging automatically
    esp32ModbusRTU modbus(&Serial1, 16);
    modbus.begin();
}
```

## Optional: Custom Logger

To route logs through a custom Logger implementation, define `USE_CUSTOM_LOGGER` in your build flags.

### Setup in platformio.ini:
```ini
build_flags = 
    -D USE_CUSTOM_LOGGER  # Enable custom logger support

lib_deps = 
    Logger  # Must include Logger library when using USE_CUSTOM_LOGGER
    esp32ModbusRTU
```

### In your main application:
```cpp
// When USE_CUSTOM_LOGGER is defined, include Logger implementation
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
    -D MODBUS_RTU_DEBUG  # Enable debug messages
```

## Log Messages

The library logs the following:
- **ERROR**: Timeout errors, CRC errors, invalid parameters, allocation failures
- **WARN**: Queue full conditions, unexpected states
- **INFO**: Watchdog configuration, task initialization
- **DEBUG**: Request/response details, state changes (only with MODBUS_RTU_DEBUG)
- **VERBOSE**: Detailed packet bytes (only with MODBUS_RTU_DEBUG)

## C++11 Compatibility

This library is fully C++11 compatible and does not use C++17 features like `__has_include`. The logging system automatically selects the appropriate backend based on the `USE_CUSTOM_LOGGER` flag at compile time.

## Memory Considerations

- **ESP-IDF logging**: No additional memory overhead
- **Custom Logger**: ~17KB for Logger singleton (shared across all libraries)

Choose ESP-IDF logging if you want minimal memory usage, or custom Logger if you need advanced features like log buffering, filtering, or custom output formats.