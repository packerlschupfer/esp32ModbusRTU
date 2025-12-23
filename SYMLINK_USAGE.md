# Using esp32ModbusRTU with Symlinks

## Correct Symlink Usage

When developing with a local copy of esp32ModbusRTU, use symlinks to avoid version conflicts:

```ini
; platformio.ini
[env:myproject]
platform = espressif32
board = esp32dev
framework = arduino

lib_deps = 
    esp32ModbusRTU=symlink://~/.platformio/lib/esp32ModbusRTU
    ; Other dependencies...
```

## Troubleshooting Version Conflicts

If you see warnings about "esp32ModbusRTU @ ^0.1.0" when using a symlink:

### 1. Check for Duplicate Entries
Make sure you don't have multiple esp32ModbusRTU entries:
```ini
; WRONG - Don't do this
lib_deps = 
    esp32ModbusRTU @ ^0.1.0  ; Remove this!
    esp32ModbusRTU=symlink://~/.platformio/lib/esp32ModbusRTU
```

### 2. Check Other Libraries
Another library might depend on esp32ModbusRTU. Check:
```bash
# In your project directory
grep -r "esp32ModbusRTU" .pio/libdeps/*/library.json
```

### 3. Clear PlatformIO Cache
```bash
pio cache clean
rm -rf .pio
pio run
```

### 4. Check Global Libraries
```bash
pio pkg list -g
```

### 5. Force Symlink Priority
Add to platformio.ini:
```ini
lib_ldf_mode = chain+
lib_compat_mode = strict
```

## Recommended Project Structure

```
YourProject/
├── src/
│   └── main.cpp
├── lib/
│   └── esp32ModbusRTU -> ~/.platformio/lib/esp32ModbusRTU
└── platformio.ini
```

Or use lib_extra_dirs:
```ini
[env:myproject]
lib_extra_dirs = 
    ~/.platformio/lib
lib_deps = 
    esp32ModbusRTU  ; Will find it in lib_extra_dirs
```

## For CI/CD or Team Development

Create a libs.ini file:
```ini
; libs.ini - Local library paths
[env]
lib_deps = 
    ${common.lib_deps}
    esp32ModbusRTU=symlink://~/.platformio/lib/esp32ModbusRTU
```

Then in platformio.ini:
```ini
[common]
lib_deps = 
    ; Other dependencies

[env:myproject]
extends = common
extra_configs = libs.ini  ; Only if libs.ini exists
```

This way team members without local libraries can still build the project.