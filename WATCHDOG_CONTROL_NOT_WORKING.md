# ModbusRTU Watchdog Control Not Working

## Critical Issue

The `setWatchdogEnabled(false)` method doesn't actually disable the ESP-IDF task watchdog, causing system crashes during device initialization retries.

## Problem Description

When calling `modbus.setWatchdogEnabled(false)`, the library logs that the watchdog is disabled but the ESP-IDF task watchdog still triggers after ~10 seconds.

### Test Case
```cpp
// Application code
modbus.setWatchdogEnabled(false);  
// Log shows: [988][loopTask][N] ModbusRTU: Watchdog disabled

// During device init retry (5 second delay)
vTaskDelay(pdMS_TO_TICKS(5000));

// Result: System crashes after 10 seconds
// E (11270) task_wdt: Task watchdog got triggered. The following tasks/users did not reset the watchdog in time:
// E (11270) task_wdt:  - ModbusRTU (CPU 0)
```

## Root Cause

The `setWatchdogEnabled()` method only sets an internal flag but doesn't actually remove the task from the ESP-IDF watchdog timer.

## Required Fix

The library needs to call the ESP-IDF watchdog functions:

```cpp
void esp32ModbusRTU::setWatchdogEnabled(bool enabled) {
    _watchdogEnabled = enabled;
    
    if (enabled) {
        // Add task to watchdog
        esp_task_wdt_add(NULL);
        MODBUS_LOG_D("Watchdog enabled");
    } else {
        // Remove task from watchdog
        esp_task_wdt_delete(NULL);
        MODBUS_LOG_D("Watchdog disabled");
    }
}
```

## Temporary Workaround

Currently, the only workaround is to disable the watchdog at compile time:
```cpp
#define MODBUS_USE_WATCHDOG 0
```

But this disables watchdog protection permanently, which is not ideal for production systems.

## Impact

This bug prevents proper device initialization retry mechanisms, making the library unsuitable for industrial applications where devices may be temporarily unavailable at startup.

## Priority

**CRITICAL** - System crashes prevent normal operation when devices are powered off.