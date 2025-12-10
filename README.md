# esp32ModbusRTU

[![Build Status](https://travis-ci.com/bertmelis/esp32ModbusRTU.svg?branch=master)](https://travis-ci.com/bertmelis/esp32ModbusRTU)

A non-blocking Modbus RTU client (master) library for ESP32 with modern logging, enhanced error handling, and memory safety features.

## Recent Improvements (v0.4.0)
- Production-ready logging with debug compiled out in release builds
- Advanced debug features: protocol tracing, buffer dumps, performance timing
- C++11 compatible flexible logging system
- Zero-overhead logging - uses ESP-IDF by default
- Optional custom Logger support via USE_CUSTOM_LOGGER flag
- No forced dependencies
- See [CHANGELOG.md](CHANGELOG.md) for details

-  Modbus Client aka Master for ESP32
-  built for the [Arduino framework for ESP32](https://github.com/espressif/arduino-esp32)
-  non blocking API. Blocking code is in a seperate task
-  only RS485 half duplex (optionally using a GPIO as RTS (DE/RS)) is implemented
-  function codes implemented:
  -  read coils (01)
  -  read discrete inputs (02)
  -  read holding registers (03)
  -  read input registers (04)
  -  write single coil (05)
  -  write single register (06)
  -  write multiple coils (15/0x0F)
  -  write multiple registers (16/0x10)
  -  read/write multiple registers (23/0x17)
-  similar API as my [esp32ModbusTCP](https://github.com/bertmelis/esp32ModbusTCP) implementation

## Logging

The library supports production-ready logging with zero overhead in release builds:

- **Production**: Only ERROR, WARN, INFO levels (debug compiled out)
- **Development**: Define `MODBUS_RTU_DEBUG` for full protocol visibility
- **Custom Logger**: Define `USE_CUSTOM_LOGGER` to route through custom Logger
- **Advanced Debug**: Buffer dumps, timing info, protocol tracing

See [LOGGING.md](LOGGING.md) for configuration details.

## Watchdog Configuration

This library includes ESP32 Task Watchdog Timer (TWDT) support. By default, the library will:
1. Register its task with the watchdog on startup
2. Feed the watchdog periodically during operation

If your application manages watchdog registration externally or you experience "task already subscribed" errors, you can disable the library's watchdog handling by defining:

```cpp
#define MODBUS_DISABLE_WATCHDOG
```

This must be defined before including the library headers.

## Developement status

I have this library in use myself with quite some uptime (only using FC3 -read holding registers- though).

## Recent Improvements

-  Enhanced error handling with new error types (INVALID_PARAMETER, QUEUE_FULL, MEMORY_ALLOCATION_FAILED, INVALID_RESPONSE)
-  Memory safety improvements with bounds checking
-  Response validation (slave address and function code verification)
-  Configurable task priority via MODBUS_TASK_PRIORITY
-  Constants for magic numbers improving code clarity
-  Watchdog timer support with disable option
-  Custom logger support

## Configuration Options

### Build Flags

-  `QUEUE_SIZE` - Request queue size (default: 16)
-  `TIMEOUT_MS` - Default timeout in milliseconds (default: 5000)
-  `MODBUS_TASK_STACK_SIZE` - Task stack size (default: 5120)
-  `MODBUS_TASK_PRIORITY` - Task priority (default: 5)
-  `MODBUS_MAX_COILS` - Maximum coils in single request (default: 2000)
-  `MODBUS_MAX_REGISTERS` - Maximum registers in single request (default: 125)
-  `MODBUS_DISABLE_WATCHDOG` - Disable watchdog timer support
-  `USE_CUSTOM_LOGGER` - Use custom Logger singleton (define in your application, not in library)
-  `MODBUS_RTU_DEBUG` - Enable debug logging

Things to do:

-  Unit testing for ModbusMessage
-  Add automatic retry mechanism
-  Implement broadcast support (address 0)
-  Add connection state management
-  Add statistics/metrics collection

## Installation

The easiest, and my preferred one, method to install this library is to use Platformio.
[https://platformio.org/lib/show/5902/esp32ModbusTCP/installation](https://platformio.org/lib/show/5902/esp32ModbusTCP/installation)

Alternatively, you can use Arduino IDE for developement.
[https://www.arduino.cc/en/guide/libraries](https://www.arduino.cc/en/guide/libraries)

Arduino framework for ESP32 v1.0.1 (January 2019) or higher is needed. Previous versions contain a bug where the last byte of the messages are truncated when using DE/RE
Arduino framework for ESP32 ([https://github.com/espressif/arduino-esp32](https://github.com/espressif/arduino-esp32))

## Example hardware:

```ASCII
   3.3V --------------+-----/\/\/\/\---+
                      |       680      |
              +-------x-------+        |
    17 <------| RO            |        |
              |              B|--------+-------------------------
    16 --+--->| DE  MAX3485   |        |                   \  /
         |    |               |        +-/\/\/\/\-+    RS-485 side
         +--->| /RE           |             120   |        /  \
              |              A|-------------------+---------------
     4 -------| DI            |                   |
              +-------x-------+                   |
                      |                           |
                      +-----/\/\/\/\--------------+
                      |       680
                      +----------------/\/\/\/\------------------ GND
                      |                  100
                     ---
```

The biasing resistors may not be neccesary for your setup. The GND connection
is connected via a 100 Ohms resistor to limit possible ground loop currents.

## Usage

The API is quite lightweight. It takes minimal 3 steps to get going.

First create the ModbusRTU object. The constructor takes two arguments: HardwareSerial object and pin number of DE/RS.

```C++
esp32ModbusRTU myModbus(&Serial, DE_PIN);
```

Next add a onData callback. This can be any function object. Here it uses a simple function pointer.

```C++

void handleData(uint8_t serverAddress, esp32Modbus::FunctionCode fc, uint8_t* data, size_t length) {
  Serial.printf("received data: id: %u, fc %u\ndata: 0x", serverAddress, fc);
  for (uint8_t i = 0; i < length; ++i) {
    Serial.printf("%02x", data[i]);
  }
  Serial.print("\n");
}

// in setup()
myModbus.onData(handleData);
```

Optionally you can attach an onError callback. Again, any function object is possible.

```C++
// in setup()
myModbus.onError([](esp32Modbus::Error error) {
  Serial.printf("error: 0x%02x\n\n", static_cast<uint8_t>(error));
});
```

After setup, start the modbusmaster:

```C++
// in setup()
myModbus.begin();
```

Now ModbusRTU is setup, you can start reading or writing. The arguments depend on the function used. Obviously, serverID, address and length are always required. The length is specified in words, not in bytes!

```C++
myModbus.readInputRegisters(0x01, 52, 2);  // serverId, address + length
```

The requests are places in a queue. The function returns immediately and doesn't wait for the server to respond.
Communication methods return a boolean value so you can check if the command was successful.

## Configuration

The request queue holds maximum 20 items. So a 21st request will fail until the queue has an empty spot. You can change the queue size in the header file or by using a compiler flag:

```C++
#define QUEUE_SIZE 20
```

The waiting time before a timeout error is returned can also be changed by a `#define` variable:

```C++
#define TIMEOUT_MS 5000
```

## Issues

Please file a Github issue ~~if~~ when you find a bug. You can also use the issue tracker for feature requests.

## Extra

For modbus-TCP, check out [esp32ModbusTCP](https://github.com/bertmelis/esp32ModbusTCP)
