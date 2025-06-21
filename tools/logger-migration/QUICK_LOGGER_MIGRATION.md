# Quick Logger Migration Guide

## 1. Add to Main Header (e.g., MyLibrary.h)

```cpp
// Logging configuration
#ifdef MYLIB_USE_CUSTOM_LOGGER
  #include "Logger.h"
  #define MYLIB_LOG_TAG "MyLibrary"
  #define MYLIB_LOG_E(...) getLogger().log(ESP_LOG_ERROR, MYLIB_LOG_TAG, __VA_ARGS__)
  #define MYLIB_LOG_W(...) getLogger().log(ESP_LOG_WARN, MYLIB_LOG_TAG, __VA_ARGS__)
  #define MYLIB_LOG_I(...) getLogger().log(ESP_LOG_INFO, MYLIB_LOG_TAG, __VA_ARGS__)
  #define MYLIB_LOG_D(...) getLogger().log(ESP_LOG_DEBUG, MYLIB_LOG_TAG, __VA_ARGS__)
#else
  #include <esp_log.h>
  #define MYLIB_LOG_TAG "MyLibrary"
  #define MYLIB_LOG_E(...) ESP_LOGE(MYLIB_LOG_TAG, __VA_ARGS__)
  #define MYLIB_LOG_W(...) ESP_LOGW(MYLIB_LOG_TAG, __VA_ARGS__)
  #define MYLIB_LOG_I(...) ESP_LOGI(MYLIB_LOG_TAG, __VA_ARGS__)
  #define MYLIB_LOG_D(...) ESP_LOGD(MYLIB_LOG_TAG, __VA_ARGS__)
#endif
```

## 2. Search & Replace Commands

```bash
# Find all logger usage
grep -rn "LOG_ERROR\|LOG_WARN\|LOG_INFO\|LOG_DEBUG" src/

# Common replacements (adjust for your patterns):
# LOG_ERROR(TAG, ...) → MYLIB_LOG_E(...)
# LOG_WARN(TAG, ...) → MYLIB_LOG_W(...)
# LOG_INFO(TAG, ...) → MYLIB_LOG_I(...)
# LOG_DEBUG(TAG, ...) → MYLIB_LOG_D(...)
```

## 3. Remove from Source Files

```cpp
// Remove these lines:
#include "Logger.h"
#include "LoggingMacros.h"
Logger& logger = getLogger();
```

## 4. Wrap Debug Logs (Optional)

```cpp
#ifdef MYLIB_DEBUG
MYLIB_LOG_D("Debug info: %d", value);
#endif
```

## 5. Update platformio.ini for Your Project

```ini
# To use with your custom logger:
build_flags = 
    -D MYLIB_USE_CUSTOM_LOGGER
    
# To enable debug logging:
build_flags = 
    -D MYLIB_DEBUG
```

## 6. Test Both Modes

```bash
# Test without logger
pio run -e test_no_logger

# Test with logger  
pio run -e test_with_logger
```

## Quick Sed Commands (Use with Caution!)

```bash
# Example for simple cases (ALWAYS backup first!):
# Replace LOG_ERROR(TAG, with MYLIB_LOG_E(
sed -i 's/LOG_ERROR(TAG, /MYLIB_LOG_E(/g' src/*.cpp

# Replace LOG_INFO(TAG, with MYLIB_LOG_I(  
sed -i 's/LOG_INFO(TAG, /MYLIB_LOG_I(/g' src/*.cpp

# Remove Logger includes
sed -i '/#include "Logger.h"/d' src/*.cpp
sed -i '/#include "LoggingMacros.h"/d' src/*.cpp
```

## Naming Conventions

| Library | Flag | Prefix | Tag |
|---------|------|--------|-----|
| esp32ModbusRTU | MODBUS_USE_CUSTOM_LOGGER | MODBUS_LOG_* | "ModbusRTU" |
| MyTempSensor | TEMPSENSOR_USE_CUSTOM_LOGGER | TEMP_LOG_* | "TempSensor" |
| MyWiFiManager | WIFIMGR_USE_CUSTOM_LOGGER | WIFI_LOG_* | "WiFiManager" |

## Commit Message Template

```
feat: Add optional custom logger support

- Replace direct logger calls with conditional macros
- Add support for both custom logger and ESP-IDF logging
- Maintain backward compatibility
- Update examples and documentation

This allows the library to be used without requiring
the custom Logger dependency.
```