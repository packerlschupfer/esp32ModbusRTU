# Recent Improvements to esp32ModbusRTU

## Logging System Modernization
- Replaced all `Serial.printf` calls with modern ESP32 logging macros
- Added optional custom Logger support via `MODBUS_USE_CUSTOM_LOGGER` flag
- Implemented proper log levels (ERROR, WARN, INFO, DEBUG)
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

## Error Handling
- Basic protocol errors (timeout, CRC) are now logged by default
- Verbose debugging remains optional via `MODBUS_RTU_DEBUG`
- Consistent error reporting patterns

## Code Quality
- Removed code duplication
- Fixed README duplicate headers
- Better organized constants and configuration
- Improved documentation

## Build Configuration
To use custom Logger:
```ini
build_flags = -D MODBUS_USE_CUSTOM_LOGGER
```

To enable debug logging:
```ini
build_flags = -D MODBUS_RTU_DEBUG
```

To customize task parameters:
```ini
build_flags = 
    -D MODBUS_TASK_STACK_SIZE=8192
    -D MODBUS_TASK_PRIORITY=10
```