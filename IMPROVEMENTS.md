# Recent Improvements to esp32ModbusRTU

## v0.4.0 - Production-Ready Debug Logging
- Implemented production-ready logging pattern
- Debug/Verbose logs completely compiled out without MODBUS_RTU_DEBUG flag
- Added advanced debug features:
  - Protocol-level tracing
  - Buffer hex dumps
  - Performance timing macros
- Created dedicated logging header (esp32ModbusRTULogging.h)
- Zero overhead in production builds

## v0.3.0 - C++11 Compatible Logging
- Made logging system fully C++11 compatible
- Removed hard dependency on LogInterface
- Library now compiles without any external dependencies by default
- LogInterface only required when USE_CUSTOM_LOGGER is defined
- Improved flexibility for embedded systems with older toolchains

## v0.2.0 - Zero-Overhead Logging with LogInterface
- Migrated to LogInterface for automatic ESP-IDF/Logger selection
- No forced Logger instantiation (~17KB memory saved)
- Application controls logging backend via USE_CUSTOM_LOGGER flag

## v0.1.0 - Initial Logging System Modernization
- Replaced all `Serial.printf` calls with modern ESP32 logging macros
- Added optional custom Logger support
- Implemented proper log levels (ERROR, WARN, INFO, DEBUG, VERBOSE)
- Created comprehensive logging documentation

## Configuration Improvements
- Made task stack size configurable (`MODBUS_TASK_STACK_SIZE`)
- Made task priority configurable (`MODBUS_TASK_PRIORITY`) 
- Made task name configurable (`MODBUS_TASK_NAME`)
- Consolidated all configuration constants in header file

## Memory Safety
- Added bounds checking for coils (max 2000) and registers (max 125)
- Added message size limits (max 256 bytes)
- Improved null pointer checks throughout
- Better validation of input parameters
- Added allocation size guards

## Error Handling
- Basic protocol errors (timeout, CRC) are now logged by default
- Verbose debugging remains optional via `MODBUS_RTU_DEBUG`
- Consistent error reporting patterns
- Human-readable error descriptions

## Code Quality
- Removed code duplication
- Fixed API naming (isSuccess, SUCCESS constants)
- Better organized constants and configuration
- Improved documentation
- Added comprehensive examples

## Build Configuration

### Default (ESP-IDF logging, no dependencies):
```ini
lib_deps = esp32ModbusRTU
```

### With Custom Logger:
```ini
build_flags = -D USE_CUSTOM_LOGGER
lib_deps = 
    Logger
    esp32ModbusRTU
```

### Enable debug logging:
```ini
build_flags = -D MODBUS_RTU_DEBUG
```

### Customize task parameters:
```ini
build_flags = 
    -D MODBUS_TASK_STACK_SIZE=8192
    -D MODBUS_TASK_PRIORITY=10
```

## C++11 Compatibility

The library is now fully C++11 compatible and doesn't use any C++17 features:
- No `__has_include` preprocessor directive
- No `if constexpr` or other C++17 constructs
- Works with ESP-IDF 3.x and 4.x
- Compatible with older Arduino-ESP32 cores