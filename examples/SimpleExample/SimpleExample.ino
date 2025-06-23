/*
Simple Modbus RTU Example with C++11 Compatible Logging

This example demonstrates basic Modbus RTU communication.
It reads holding registers from a Modbus device.

Logging options:
1. Default (no configuration needed) - Uses ESP-IDF logging
2. Custom Logger - Define USE_CUSTOM_LOGGER in platformio.ini

This library is C++11 compatible and doesn't require C++17 features.

To enable debug logging:
- Define MODBUS_RTU_DEBUG in your build flags
*/

#include <Arduino.h>

// When using custom Logger (define USE_CUSTOM_LOGGER in platformio.ini)
#ifdef USE_CUSTOM_LOGGER
#include <Logger.h>
#include <LogInterfaceImpl.h>
#endif

#include <esp32ModbusRTU.h>

// Create ModbusRTU instance
// Parameters: Serial port, RTS pin (use -1 if not needed)
esp32ModbusRTU modbus(&Serial2, 16);  // Serial2 with pin 16 as RTS

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }
  
  Serial.println("Modbus RTU Example Starting...");
  
  #ifdef USE_CUSTOM_LOGGER
  // Initialize custom Logger if enabled
  Logger::getInstance().init(1024);
  Logger::getInstance().setLogLevel(ESP_LOG_DEBUG);
  Logger::getInstance().enableLogging(true);
  Serial.println("Using custom Logger for ModbusRTU");
  #else
  Serial.println("Using ESP-IDF logging for ModbusRTU");
  #endif
  
  // Configure Serial2 for Modbus communication
  // Parameters: baud rate, config, RX pin, TX pin
  Serial2.begin(9600, SERIAL_8N1, 17, 4);
  
  // Set up data handler - called when valid response is received
  modbus.onData([](uint8_t serverAddress, esp32Modbus::FunctionCode fc, uint16_t address, uint8_t* data, size_t length) {
    Serial.printf("Response from device %d:\n", serverAddress);
    Serial.printf("  Function: 0x%02x\n", fc);
    Serial.printf("  Address: %d\n", address);
    Serial.printf("  Data (%d bytes): ", length);
    
    for (size_t i = 0; i < length; ++i) {
      Serial.printf("%02x ", data[i]);
    }
    Serial.println();
    
    // Example: interpret as 16-bit registers
    if (fc == esp32Modbus::READ_HOLD_REGISTER && length >= 2) {
      for (size_t i = 0; i < length; i += 2) {
        uint16_t value = (data[i] << 8) | data[i + 1];
        Serial.printf("  Register %d = %d (0x%04x)\n", 
                     address + (i/2), value, value);
      }
    }
  });
  
  // Set up error handler
  modbus.onError([](esp32Modbus::Error error) {
    Serial.printf("Modbus error: %s (0x%02x)\n", 
                 esp32Modbus::getErrorDescription(error), error);
  });
  
  // Start ModbusRTU background task
  modbus.begin();
  
  Serial.println("Setup complete. Sending requests every 5 seconds...");
}

void loop() {
  static uint32_t lastMillis = 0;
  
  // Send request every 5 seconds
  if (millis() - lastMillis > 5000) {
    lastMillis = millis();
    
    // Read 10 holding registers starting at address 0 from device ID 1
    Serial.println("\nSending Modbus request...");
    bool success = modbus.readHoldingRegisters(1, 0, 10);
    
    if (!success) {
      Serial.println("Failed to queue request (queue full?)");
    }
  }
  
  // Your other code here...
  delay(10);
}

/* 
Additional examples:

// Read coils (digital outputs)
modbus.readCoils(1, 0, 16);  // Device 1, starting at coil 0, read 16 coils

// Read discrete inputs
modbus.readDiscreteInputs(1, 0, 8);  // Device 1, starting at input 0, read 8 inputs

// Read input registers (analog inputs)
modbus.readInputRegisters(1, 0, 4);  // Device 1, starting at register 0, read 4 registers

// Write single coil
modbus.writeSingleCoil(1, 0, true);  // Device 1, coil 0, turn ON

// Write single register
modbus.writeSingleHoldingRegister(1, 0, 12345);  // Device 1, register 0, value 12345

// Write multiple coils
bool coils[8] = {true, false, true, false, true, false, true, false};
modbus.writeMultipleCoils(1, 0, 8, coils);  // Device 1, starting at coil 0, write 8 coils

// Write multiple registers
uint8_t data[4] = {0x12, 0x34, 0x56, 0x78};  // 2 registers: 0x1234 and 0x5678
modbus.writeMultHoldingRegisters(1, 0, 2, data);  // Device 1, starting at register 0, write 2 registers
*/