# Changelog

## [0.1.0] - 2024-01-21

### Added
- Modern ESP32 logging system replacing Serial.printf
- Optional custom Logger support via `MODBUS_USE_CUSTOM_LOGGER`
- Configurable task parameters (stack size, priority, name)
- Memory safety limits for coils and registers
- Human-readable error descriptions
- New simple example demonstrating modern features
- Comprehensive documentation (LOGGING.md, IMPROVEMENTS.md)

### Changed
- All logging now uses ESP-IDF log levels (ERROR, WARN, INFO, DEBUG)
- Consolidated configuration constants in header file
- Task stack size increased to 5120 bytes by default
- Queue size reduced to 16 to save memory

### Fixed
- Spelling: Added `SUCCESS` (keeping `SUCCES` for backward compatibility)
- Method name: Added `isSuccess()` (keeping `isSucces()` for backward compatibility)
- Removed duplicate constant definitions
- Fixed duplicate README headers

### Deprecated
- `SUCCES` enum value (use `SUCCESS` instead)
- `isSucces()` method (use `isSuccess()` instead)

## [0.0.2] - Previous version
- Initial release features