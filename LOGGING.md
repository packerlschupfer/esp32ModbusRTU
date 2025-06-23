# Logging Configuration

The esp32ModbusRTU library supports flexible, production-ready logging with zero overhead in release builds.

## Default: ESP-IDF Logging

By default, the library uses ESP-IDF's built-in logging system:
- Log messages use the tag "ModbusRTU"
- Only ERROR, WARN, and INFO levels are compiled in release builds
- DEBUG and VERBOSE are completely compiled out (zero overhead)
- No external dependencies

Example:
```cpp
#include <esp32ModbusRTU.h>

void setup() {
    // Library will use ESP-IDF logging automatically
    // Only Error, Warn, Info messages in release builds
    esp32ModbusRTU modbus(&Serial1, 16);
    modbus.begin();
}
```

## Debug Logging

To enable debug/verbose logging for development:

```ini
build_flags = 
    -D MODBUS_RTU_DEBUG  # Enable debug messages
```

With debug enabled, you get:
- Protocol-level messages (requests/responses)
- Performance timing information
- Buffer hex dumps
- Detailed state transitions

## Optional: Custom Logger

To route logs through a custom Logger implementation:

### Setup in platformio.ini:
```ini
build_flags = 
    -D USE_CUSTOM_LOGGER  # Enable custom logger support
    -D MODBUS_RTU_DEBUG   # Optional: enable debug

lib_deps = 
    Logger  # Must include Logger library when using USE_CUSTOM_LOGGER
    esp32ModbusRTU
```

### In your main application:
```cpp
// When USE_CUSTOM_LOGGER is defined, include Logger implementation
#include <Logger.h>
#include <LogInterfaceImpl.h>

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

## Log Levels and Messages

### Always Visible (Production)
- **ERROR**: Timeout errors, CRC errors, invalid parameters, allocation failures
- **WARN**: Queue full conditions, unexpected states
- **INFO**: Task initialization, watchdog configuration

### Debug Only (Development) 
These are completely compiled out without `MODBUS_RTU_DEBUG`:
- **DEBUG**: Request/response details, state changes, timing info
- **VERBOSE**: Detailed packet bytes, protocol flow

## Advanced Debug Features

When `MODBUS_RTU_DEBUG` is enabled, additional features become available:

### Protocol Debugging
```
[PROTO] Sending 8 bytes to address 0x01, FC=0x03
[PROTO] Response complete: 19 bytes received
[PROTO] Response timeout after 3000 ms
```

### Buffer Dumps
```
TX (8 bytes):
 01 03 00 00 00 0A C5 CD
RX (19 bytes):
 01 03 14 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 3E E3
```

### Performance Timing
```
[TIMING] Request/Response cycle took 125 ms
```

## Production Best Practices

1. **Release Builds**: Don't define `MODBUS_RTU_DEBUG`
   - Debug code is completely removed at compile time
   - Zero performance overhead
   - Smaller binary size

2. **Development**: Define `MODBUS_RTU_DEBUG`
   - Full protocol visibility
   - Performance profiling
   - Issue diagnosis

3. **Selective Debugging**: Enable debug for specific libraries only
   ```ini
   build_flags = 
       -D MODBUS_RTU_DEBUG      # Debug this library
       ; -D WIFI_MANAGER_DEBUG   # But not this one
   ```

## Memory Considerations

- **ESP-IDF logging**: No additional memory overhead
- **Custom Logger**: ~17KB for Logger singleton (shared across all libraries)
- **Debug strings**: Only included when `MODBUS_RTU_DEBUG` is defined

## Example: Complete Debug Configuration

```ini
[env:debug]
platform = espressif32
board = esp32dev
framework = arduino

build_flags = 
    -D USE_CUSTOM_LOGGER  # Use custom logger
    -D MODBUS_RTU_DEBUG   # Enable all debug features
    
lib_deps = 
    Logger
    esp32ModbusRTU
```

## C++11 Compatibility

This library is fully C++11 compatible and uses standard preprocessor directives for conditional compilation. No C++17 features are required.