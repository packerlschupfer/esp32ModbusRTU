# Watchdog Control in esp32ModbusRTU

## Overview

The esp32ModbusRTU library now provides enhanced watchdog control to prevent system crashes during device initialization and long operations. The library supports two modes:

1. **Default Mode**: Uses ESP-IDF's built-in watchdog timer
2. **Enhanced Mode**: Integrates with the external Watchdog class library for additional features

## Problem Solved

Previously, the ModbusRTU task could trigger a watchdog timeout when:
- Modbus devices were unavailable during initialization
- Applications implemented retry logic with long delays
- Response timeouts exceeded the watchdog timer period

This caused system crashes that prevented proper device initialization in industrial control systems.

## Solution

### Core Fixes
1. The library now feeds the watchdog periodically (every 500ms) during the `_receive()` method, preventing timeouts during long waits for device responses.
2. The `setWatchdogEnabled()` method now actually registers/unregisters the task with ESP-IDF watchdog, not just sets a flag.

### New API Methods

```cpp
// Enable or disable watchdog for this ModbusRTU instance
// When disabled: unregisters task from ESP-IDF watchdog
// When enabled: registers task with ESP-IDF watchdog
void setWatchdogEnabled(bool enabled);

// Check if watchdog is enabled
bool isWatchdogEnabled() const;
```

## Usage Examples

### Basic Usage (Default Mode)

```cpp
esp32ModbusRTU modbus(&Serial2, 4);  // Serial2, RTS pin 4

// Disable watchdog during initialization
modbus.setWatchdogEnabled(false);

// Initialize devices with retry logic
while (!deviceReady) {
    // Try to initialize device
    if (initializeDevice()) {
        deviceReady = true;
    } else {
        // Long delay is now safe
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

// Re-enable watchdog after initialization
modbus.setWatchdogEnabled(true);
```

### Enhanced Mode with External Watchdog Class

To use the enhanced mode with the external Watchdog class:

1. Add the build flag in `platformio.ini`:
```ini
build_flags = -DMODBUS_USE_EXTERNAL_WATCHDOG
```

2. Ensure the Watchdog library is available in your project

3. Initialize the global Watchdog before using ModbusRTU:
```cpp
// Initialize global watchdog
Watchdog::quickInit(30, true);  // 30 second timeout, panic on trigger

// Create ModbusRTU instance
esp32ModbusRTU modbus(&Serial2, 4);

// The ModbusRTU task will automatically register with the Watchdog
modbus.begin();
```

## Implementation Details

### Watchdog Feeding Strategy
- During `_receive()`: Feeds watchdog every 500ms while waiting for response
- During idle queue wait: Feeds watchdog every 1 second
- After processing requests: Feeds watchdog after each completed operation

### Conditional Compilation
The library automatically detects if the Watchdog class is available:
- If `MODBUS_USE_EXTERNAL_WATCHDOG` is defined: Uses Watchdog class
- Otherwise: Uses ESP-IDF watchdog functions directly

### Thread Safety
All watchdog operations are performed from within the ModbusRTU task context, ensuring thread safety.

## Build Configuration

### Disable Watchdog Completely
```cpp
#define MODBUS_DISABLE_WATCHDOG
```

### Use External Watchdog Class
```cpp
#define MODBUS_USE_EXTERNAL_WATCHDOG
```

## Migration Guide

Existing code continues to work without changes. To take advantage of the new features:

1. **For initialization issues**: Use `setWatchdogEnabled(false)` during init
2. **For enhanced monitoring**: Enable external Watchdog support
3. **For production systems**: Keep watchdog enabled after initialization

## Best Practices

1. **Temporary Disable**: Only disable watchdog during initialization or configuration
2. **Re-enable Promptly**: Always re-enable watchdog after critical operations
3. **Monitor Status**: Use `isWatchdogEnabled()` to verify watchdog state
4. **Test Both Modes**: Verify your application works with and without external Watchdog

## Troubleshooting

### Still Getting Watchdog Timeouts?
1. Verify watchdog is disabled during long operations
2. Check that response timeout (`setTimeOutValue()`) is reasonable
3. Ensure external Watchdog is initialized before ModbusRTU (if using enhanced mode)

### External Watchdog Not Working?
1. Verify `MODBUS_USE_EXTERNAL_WATCHDOG` is defined
2. Check that Watchdog library is in your project dependencies
3. Ensure `Watchdog::quickInit()` is called before ModbusRTU initialization