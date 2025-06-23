# Migration Guide: v0.2.0 to v0.3.0

This guide helps you migrate from esp32ModbusRTU v0.2.0 (LogInterface dependency) to v0.3.0 (C++11 compatible, no hard dependencies).

## What Changed?

### v0.2.0 Approach
- Required LogInterface as a dependency
- Always included LogInterface.h
- Assumed LogInterface was available

### v0.3.0 Approach (C++11 Compatible)
- No hard dependencies
- LogInterface only required when USE_CUSTOM_LOGGER is defined
- Fully C++11 compatible (no C++17 features)
- Works with older toolchains

## Migration Steps

### 1. Update Your platformio.ini

**Before (v0.2.0):**
```ini
lib_deps = 
    Logger
    LogInterface  ; Was required
    esp32ModbusRTU
```

**After (v0.3.0) - Using ESP-IDF logging:**
```ini
lib_deps = 
    esp32ModbusRTU  ; No other dependencies needed
```

**After (v0.3.0) - Using Custom Logger:**
```ini
build_flags = -D USE_CUSTOM_LOGGER

lib_deps = 
    Logger  ; Only needed when USE_CUSTOM_LOGGER is defined
    esp32ModbusRTU
```

### 2. Update Your Code

The library API hasn't changed, but the initialization might be different:

**Before (v0.2.0):**
```cpp
// LogInterface was always available
#include <Logger.h>
#include <LogInterfaceImpl.h>
#include <esp32ModbusRTU.h>

void setup() {
    Logger::getInstance().init(1024);
    // ...
}
```

**After (v0.3.0) - ESP-IDF logging (recommended for minimal memory):**
```cpp
#include <esp32ModbusRTU.h>

void setup() {
    // No Logger initialization needed
    // Library uses ESP-IDF logging automatically
    
    esp32ModbusRTU modbus(&Serial2, 16);
    modbus.begin();
}
```

**After (v0.3.0) - Custom Logger (when you need advanced features):**
```cpp
// Define this in platformio.ini instead: build_flags = -D USE_CUSTOM_LOGGER
#ifdef USE_CUSTOM_LOGGER
#include <Logger.h>
#include <LogInterfaceImpl.h>
#endif

#include <esp32ModbusRTU.h>

void setup() {
    #ifdef USE_CUSTOM_LOGGER
    Logger::getInstance().init(1024);
    Logger::getInstance().setLogLevel(ESP_LOG_DEBUG);
    #endif
    
    esp32ModbusRTU modbus(&Serial2, 16);
    modbus.begin();
}
```

## Benefits of v0.3.0

1. **No Forced Dependencies**: Library works standalone with ESP-IDF
2. **C++11 Compatible**: Works with older toolchains and embedded systems
3. **Memory Efficient**: No Logger overhead unless explicitly requested
4. **Flexible**: Choose logging backend based on your needs

## Troubleshooting

### Compilation Errors about LogInterface

If you get errors about missing LogInterface:
1. Make sure you're not defining USE_CUSTOM_LOGGER unless you have Logger library
2. Clean your build: `pio run -t clean`
3. Update library: `pio update`

### No Log Output

If using ESP-IDF logging:
```cpp
// Set log level for ModbusRTU
esp_log_level_set("ModbusRTU", ESP_LOG_DEBUG);
```

If using custom Logger:
```cpp
// Make sure USE_CUSTOM_LOGGER is defined in build flags
// Initialize Logger in your setup()
Logger::getInstance().init(1024);
Logger::getInstance().enableLogging(true);
```

## Summary

The main change is that LogInterface is now optional. For most users, the default ESP-IDF logging is sufficient and uses zero additional memory. Only enable custom Logger if you need its advanced features.