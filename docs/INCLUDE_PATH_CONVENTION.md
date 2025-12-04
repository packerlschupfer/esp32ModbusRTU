# Include Path Convention

## Important: Libraries Should Use Angle Brackets

When including headers from other libraries, always use angle brackets:

✅ **Correct:**
```cpp
#include <LogInterface.h>  // Looks in library paths
#include <Logger.h>        // Looks in library paths
```

❌ **Wrong:**
```cpp
#include "LogInterface.h"  // Looks in current directory first
#include "Logger.h"        // May fail in some build configurations
```

## Why This Matters

1. **Build Reliability**: Angle brackets ensure the compiler looks in the correct library paths
2. **PlatformIO Compatibility**: PlatformIO's Library Dependency Finder works better with angle brackets
3. **Cross-Project Usage**: Libraries can be used in different projects without path issues

## Examples in This Library

### In esp32ModbusRTULogging.h:
```cpp
#ifdef USE_CUSTOM_LOGGER
    #include <LogInterface.h>  // Correct - angle brackets
    // ... logging macros
#endif
```

### In Application Code (examples):
```cpp
#ifdef USE_CUSTOM_LOGGER
#include <Logger.h>           // Library headers use angle brackets
#include <LogInterfaceImpl.h> // Library headers use angle brackets
#endif

#include <esp32ModbusRTU.h>   // Library header uses angle brackets
```

## Summary

- **Libraries** → Always use `<header.h>` for external dependencies
- **Applications** → Use `<header.h>` for libraries, `"header.h"` for local files

This convention ensures maximum compatibility and reliability across different build environments.

## Implementation Status

✅ **All libraries have been updated** to follow this convention as of 2025-06-23.

For detailed status and verification information, see [`~/.platformio/lib/INCLUDE_CONVENTION_STATUS.md`](../../INCLUDE_CONVENTION_STATUS.md)