# esp32ModbusRTU Library Dependency Format Issue

## Problem Description

The esp32ModbusRTU library has an incorrect dependency format in `library.json` that causes PlatformIO build failures:

```
Library Manager: Installing {'version': '*', 'platforms': 'espressif32', 'required': False}
VCSBaseException: VCS: Unknown repository type {'version': '*', 'platforms': 'espressif32', 'required': False}
```

## Root Cause

The dependency was specified using object notation instead of the required array format:

**Incorrect format:**
```json
"dependencies": {
  "workspace_Class-Watchdog": {
    "version": "*",
    "platforms": "espressif32",
    "required": false
  }
}
```

**Correct format:**
```json
"dependencies": [
  {
    "name": "workspace_Class-Watchdog",
    "version": "*",
    "platforms": "espressif32",
    "required": false
  }
]
```

## Fix Applied

Initially tried to fix the format, but due to PlatformIO caching and the dependency being optional, removed the dependency entirely:

```json
"dependencies": [],
```

Since the Watchdog integration uses conditional compilation (`#ifdef MODBUS_USE_EXTERNAL_WATCHDOG`), the dependency is truly optional and can be omitted.

## Additional Recommendations

1. Since Watchdog is optional (required: false), consider documenting this in the README
2. Consider using conditional compilation to make the library work without Watchdog
3. For local development, the dependency name `workspace_Class-Watchdog` might not resolve for other users

## PlatformIO Dependency Format Reference

According to PlatformIO documentation, dependencies should be an array of objects when specifying advanced options:
- https://docs.platformio.org/en/latest/manifests/library-json/fields/dependencies.html

Each dependency object must have a `name` field when using the advanced format.