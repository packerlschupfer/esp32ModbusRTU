# PlatformIO Usage Guide

## Avoiding Library Name Conflicts

Due to multiple libraries with the name "esp32ModbusRTU" in the PlatformIO registry, you may see warnings about name conflicts. Here's how to use this library correctly:

## Installation Methods

### Method 1: Direct GitHub Installation (Recommended)
```ini
; platformio.ini
[env:myenv]
lib_deps = 
    https://github.com/bertmelis/esp32ModbusRTU.git#master
```

Or for a specific version/branch:
```ini
lib_deps = 
    https://github.com/bertmelis/esp32ModbusRTU.git#v0.2.0
    ; or use branch name
    https://github.com/bertmelis/esp32ModbusRTU.git#fix/fc2-bytecount
```

### Method 2: Using Library Owner Specification
```ini
lib_deps = 
    bertmelis/esp32ModbusRTU@^0.2.0
```

### Method 3: Local Library Installation
1. Clone this repository to your project's `lib` folder:
```bash
cd your_project
git clone https://github.com/bertmelis/esp32ModbusRTU.git lib/esp32ModbusRTU
```

2. The library will be automatically detected by PlatformIO

### Method 4: Symlink for Development
If you're developing with this library:
```bash
cd your_project/lib
ln -s ~/.platformio/lib/esp32ModbusRTU esp32ModbusRTU
```

## Complete platformio.ini Example

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200

; Use specific GitHub repository to avoid conflicts
lib_deps = 
    https://github.com/bertmelis/esp32ModbusRTU.git#master
    ; LogInterface dependency will be resolved automatically
    
; Optional: Enable custom Logger
build_flags = 
    -D USE_CUSTOM_LOGGER  ; Use custom Logger instead of ESP-IDF
    -D MODBUS_RTU_DEBUG   ; Enable debug logging
```

## Dependencies

This library depends on:
- **LogInterface** - For flexible logging support

When using custom Logger mode (USE_CUSTOM_LOGGER defined), you'll also need:
- **Logger** - The actual Logger implementation

## Verifying Installation

After installation, you can verify the correct version:
```bash
pio pkg list
```

Should show:
```
esp32ModbusRTU @ 0.2.0 (git+https://github.com/bertmelis/esp32ModbusRTU.git)
```

## Troubleshooting

If you still see conflicts:
1. Clear PlatformIO cache: `pio cache clean`
2. Remove .pio folder: `rm -rf .pio`
3. Use explicit GitHub URL in lib_deps
4. Specify the exact version needed

## For Library Developers

If you're forking or modifying this library:
1. Change the library name in library.json to avoid conflicts
2. Update the repository URL
3. Consider publishing under a different name if making significant changes