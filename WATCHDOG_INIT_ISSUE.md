# ModbusRTU Watchdog Issue During Device Initialization

## Problem Description

The ModbusRTU task watchdog triggers a system crash when Modbus devices are unavailable during initialization, preventing proper retry mechanisms for essential hardware.

## Issue Details

### Scenario
1. System initializes with MB8ART (temperature sensors) and RYN4 (relay controller) devices powered off
2. Application implements retry logic to wait for essential devices to become available
3. ModbusRTU task watchdog times out after ~10 seconds during retry attempts
4. System crashes with watchdog timeout

### Error Log
```
[4302][loopTask][E] MB8ART: Device not responsive after 3 attempts
[4302][loopTask][E] MB8ART: Configuration failed
[4303][loopTask][W] MAIN: MB8ART init attempt 1 failed (3312 ms): Unknown error - retrying in 5 seconds...
E (11270) task_wdt: Task watchdog got triggered. The following tasks/users did not reset the watchdog in time:
E (11270) task_wdt:  - ModbusRTU (CPU 0)
E (11270) task_wdt: Tasks currently running:
E (11270) task_wdt: CPU 0: IDLE0
E (11270) task_wdt: CPU 1: IDLE1
E (11270) task_wdt: Aborting.
```

## Root Cause Analysis

1. ModbusRTU task has internal watchdog with 10-second timeout
2. During device initialization retries, the ModbusRTU task may be blocked or waiting
3. Application cannot feed ModbusRTU's internal watchdog from external tasks
4. No API to temporarily disable or extend watchdog during initialization phase

## Use Case Requirements

For industrial control systems (boiler controllers, etc.), certain Modbus devices are ESSENTIAL:
- System must wait indefinitely for critical devices to become available
- Retry mechanisms need to work without triggering watchdog timeouts
- System should not proceed without essential devices (safety requirement)

## Proposed Solutions

### 1. Watchdog Control API (Preferred)
Add methods to control watchdog behavior:
```cpp
class esp32ModbusRTU {
    // Temporarily disable watchdog (e.g., during initialization)
    void disableWatchdog();
    
    // Re-enable watchdog with optional new timeout
    void enableWatchdog(uint32_t timeoutMs = 10000);
    
    // Extend current watchdog timeout
    void extendWatchdogTimeout(uint32_t additionalMs);
    
    // Manual watchdog feed from external task
    void feedWatchdog();
};
```

### 2. Initialization Mode
Add special initialization mode that handles device unavailability:
```cpp
// Initialize with retry-friendly configuration
modbus.beginWithRetry(serial, options);

struct ModbusOptions {
    bool enableWatchdog = true;
    uint32_t watchdogTimeoutMs = 10000;
    uint32_t initTimeoutMs = 60000;  // Longer timeout during init
};
```

### 3. Conditional Compilation
Support build-time configuration:
```cpp
#ifndef MODBUS_USE_WATCHDOG
#define MODBUS_USE_WATCHDOG 1
#endif

#ifndef MODBUS_WATCHDOG_TIMEOUT_MS
#define MODBUS_WATCHDOG_TIMEOUT_MS 10000
#endif
```

### 4. Event-Based Keepalive
Allow external tasks to signal activity:
```cpp
// External task can signal that Modbus operations are ongoing
modbus.signalActivity();
```

## Current Workaround

Application uses short delays (100ms) during retry loops to allow ModbusRTU task to run:
```cpp
// Instead of single long delay
vTaskDelay(pdMS_TO_TICKS(5000));

// Use multiple short delays
for (uint32_t i = 0; i < 50; i++) {
    vTaskDelay(pdMS_TO_TICKS(100));
}
```

This is suboptimal as it doesn't guarantee the ModbusRTU task will feed its watchdog.

## Expected Behavior

1. ModbusRTU should not crash the system during device initialization retries
2. Applications should be able to implement robust retry mechanisms
3. Watchdog should protect against actual hangs, not initialization delays

## Test Case

```cpp
// This should work without crashing
MB8ART sensor(0x01);
while (true) {
    auto result = sensor.initialize();
    if (result.isOk()) break;
    
    LOG_WARN("Sensor init failed, retrying in 5s...");
    vTaskDelay(pdMS_TO_TICKS(5000));  // Should not trigger watchdog
}
```

## Priority

**HIGH** - This prevents proper initialization of industrial control systems where devices may be temporarily unavailable at startup.

## Additional Context

- Issue discovered in ESPlan Boiler Controller project
- Affects systems where Modbus devices are essential for operation
- Current workaround is fragile and doesn't fully prevent crashes

Please consider implementing one of the proposed solutions to allow robust device initialization with proper retry mechanisms.