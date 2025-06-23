# Migration Plan: Fixed Logger to Optional Logger Support

This document provides a step-by-step guide for migrating libraries that currently use a fixed custom logger to support optional logger usage, allowing the library to work both with and without the custom logger.

## Overview

The goal is to make libraries usable by others who don't have your custom Logger, while still allowing you to use it when available.

## Step 1: Identify Current Logger Usage

Search for all logger usage in the library:
```bash
# Find all logger calls
grep -r "LOG_" src/
grep -r "getLogger" src/
grep -r "Logger::" src/
grep -r "#include.*Logger" src/
```

Document the patterns found:
- Direct logger calls (e.g., `LOG_ERROR`, `LOG_INFO`)
- Logger includes
- Logger initialization

## Step 2: Create Logging Abstraction Layer

### 2.1 Add Conditional Logging Macros to Main Header

In your main library header file (e.g., `MyLibrary.h`), add:

```cpp
// Logging configuration using LogInterface (recommended)
#include <LogInterface.h>

#define MYLIB_LOG_TAG "MyLibrary"
#define MYLIB_LOG_E(...) LOG_ERROR(MYLIB_LOG_TAG, __VA_ARGS__)
#define MYLIB_LOG_W(...) LOG_WARN(MYLIB_LOG_TAG, __VA_ARGS__)
#define MYLIB_LOG_I(...) LOG_INFO(MYLIB_LOG_TAG, __VA_ARGS__)
#define MYLIB_LOG_D(...) LOG_DEBUG(MYLIB_LOG_TAG, __VA_ARGS__)
#define MYLIB_LOG_V(...) LOG_VERBOSE(MYLIB_LOG_TAG, __VA_ARGS__)

// Note: LogInterface automatically handles both ESP-IDF and custom Logger
// based on whether USE_CUSTOM_LOGGER is defined by the application
```

Replace:
- `USE_CUSTOM_LOGGER` with your library-specific flag (e.g., `MYLIB_USE_CUSTOM_LOGGER`)
- `MYLIB_LOG_*` with your library prefix
- `"MyLibrary"` with your library name

## Step 3: Replace Logger Calls

### 3.1 Simple Replacements

Replace all direct logger calls:

```cpp
// Before:
LOG_ERROR(TAG, "Error message: %s", error);
LOG_INFO(TAG, "Info message");

// After:
MYLIB_LOG_E("Error message: %s", error);
MYLIB_LOG_I("Info message");
```

### 3.2 Conditional Debug Logging

For verbose debug logging that should be optional:

```cpp
// Before:
LOG_DEBUG(TAG, "Sending %d bytes", length);

// After:
#ifdef MYLIB_DEBUG
MYLIB_LOG_D("Sending %d bytes", length);
#endif
```

## Step 4: Remove Direct Logger Dependencies

### 4.1 Remove Logger Includes

Remove from source files:
```cpp
// Remove these lines from .cpp files:
#include "Logger.h"
#include "LoggingMacros.h"
```

### 4.2 Remove Logger Initialization

Remove any logger initialization or configuration:
```cpp
// Remove:
Logger& logger = getLogger();
logger.setLogLevel(ESP_LOG_DEBUG);
```

## Step 5: Update Build Configuration

### 5.1 Create Build Flags Documentation

Create a `BUILDING.md` or add to README:

```markdown
## Build Configuration

### Using Custom Logger
To use with custom Logger library:
```ini
build_flags = 
    -D MYLIB_USE_CUSTOM_LOGGER
    -I ../path/to/logger/src
```

### Debug Logging
To enable debug logging:
```ini
build_flags = 
    -D MYLIB_DEBUG
```
```

## Step 6: Testing Strategy

### 6.1 Test Without Custom Logger
```ini
# platformio.ini
[env:no_logger]
platform = espressif32
framework = arduino
lib_deps = 
    ./
```

### 6.2 Test With Custom Logger
```ini
[env:with_logger]
platform = espressif32
framework = arduino
build_flags = 
    -D MYLIB_USE_CUSTOM_LOGGER
lib_deps = 
    ./
    ../path/to/Logger
```

## Step 7: Update Examples

Create examples showing both usage patterns:

### Example without Logger
```cpp
#include <MyLibrary.h>

void setup() {
  Serial.begin(115200);
  
  // Library will use ESP_LOG* internally
  MyLibrary lib;
  lib.begin();
}
```

### Example with Logger
```cpp
// Enable custom logger before including library
#define MYLIB_USE_CUSTOM_LOGGER

#include <Logger.h>
#include <MyLibrary.h>

void setup() {
  // Initialize your logger
  Logger& logger = getLogger();
  logger.init();
  
  // Library will use your logger
  MyLibrary lib;
  lib.begin();
}
```

## Step 8: Migration Checklist

- [ ] Identified all logger usage patterns
- [ ] Created logging macros in main header
- [ ] Replaced all LOG_* calls with MYLIB_LOG_*
- [ ] Removed direct Logger includes from source files
- [ ] Removed logger initialization code
- [ ] Added conditional compilation for debug logs
- [ ] Updated documentation
- [ ] Created examples for both modes
- [ ] Tested without custom logger
- [ ] Tested with custom logger
- [ ] Updated version number
- [ ] Created changelog entry

## Common Patterns to Handle

### Pattern 1: Logger in Class Members
```cpp
// Before:
class MyClass {
private:
    Logger& logger = getLogger();
};

// After:
class MyClass {
    // Remove logger member, use macros directly
};
```

### Pattern 2: Conditional Logging
```cpp
// Before:
if (logger.getLogLevel() >= ESP_LOG_DEBUG) {
    // expensive operation
    LOG_DEBUG(TAG, "Details: %s", getExpensiveDetails());
}

// After:
#ifdef MYLIB_DEBUG
    // expensive operation
    MYLIB_LOG_D("Details: %s", getExpensiveDetails());
#endif
```

### Pattern 3: Dynamic Log Tags
```cpp
// Before:
LOG_ERROR(className, "Error in %s", methodName);

// After:
MYLIB_LOG_E("[%s] Error in %s", className, methodName);
```

## Benefits of This Approach

1. **Backward Compatible**: Existing code using logger continues to work
2. **Wider Adoption**: Others can use library without your logger
3. **Flexible**: Users can choose their logging solution
4. **Performance**: No overhead when logging is disabled
5. **Maintainable**: Single set of logging macros to maintain

## Example Libraries Successfully Migrated

- esp32ModbusRTU - Modbus communication library
- [Add your libraries here as you migrate them]