/*
LogInterface Test Example

This example demonstrates how esp32ModbusRTU works with LogInterface
for zero-overhead logging.

Compile this example in two ways:
1. Without USE_CUSTOM_LOGGER - Uses ESP-IDF logging
2. With USE_CUSTOM_LOGGER - Uses custom Logger singleton
*/

#include <Arduino.h>

// Uncomment to use custom Logger instead of ESP-IDF
// #define USE_CUSTOM_LOGGER

#ifdef USE_CUSTOM_LOGGER
  // When using custom Logger, include these in your main app
  #include <Logger.h>
  #include <LogInterfaceImpl.h>
#endif

// Include ModbusRTU - it will automatically use the right logging
#include <esp32ModbusRTU.h>

// Create ModbusRTU instance
esp32ModbusRTU modbus(&Serial2, 16);

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }
  
  // Show memory before initialization
  uint32_t beforeHeap = ESP.getFreeHeap();
  Serial.printf("\n=== LogInterface Test ===\n");
  Serial.printf("Free heap before init: %d bytes\n", beforeHeap);
  
  #ifdef USE_CUSTOM_LOGGER
    Serial.println("Mode: Using custom Logger");
    
    // Initialize Logger singleton
    Logger& logger = Logger::getInstance();
    logger.init(1024);
    logger.setLogLevel(ESP_LOG_DEBUG);
    logger.enableLogging(true);
    
    // Configure specific log levels if needed
    logger.setTagLevel("ModbusRTU", ESP_LOG_DEBUG);
    
    uint32_t afterLogger = ESP.getFreeHeap();
    Serial.printf("Free heap after Logger init: %d bytes\n", afterLogger);
    Serial.printf("Logger memory usage: %d bytes\n", beforeHeap - afterLogger);
  #else
    Serial.println("Mode: Using ESP-IDF logging (zero overhead)");
    
    // Configure ESP-IDF log level for ModbusRTU
    esp_log_level_set("ModbusRTU", ESP_LOG_DEBUG);
  #endif
  
  // Configure Serial2 for Modbus
  Serial2.begin(9600, SERIAL_8N1, 17, 4);
  
  // Set up handlers
  modbus.onData([](uint8_t serverAddress, esp32Modbus::FunctionCode fc, 
                   uint16_t address, uint8_t* data, size_t length) {
    Serial.printf("\nReceived response from device %d\n", serverAddress);
    Serial.printf("Function code: 0x%02x\n", fc);
    Serial.printf("Data address: %d\n", address);
    Serial.printf("Data length: %d bytes\n", length);
  });
  
  modbus.onError([](esp32Modbus::Error error) {
    Serial.printf("\nModbus error: %s (0x%02x)\n", 
                  esp32Modbus::getErrorDescription(error), error);
  });
  
  // Start ModbusRTU
  modbus.begin();
  
  uint32_t afterModbus = ESP.getFreeHeap();
  Serial.printf("Free heap after Modbus init: %d bytes\n", afterModbus);
  
  #ifdef USE_CUSTOM_LOGGER
    Serial.printf("Total memory used: %d bytes\n", beforeHeap - afterModbus);
  #else
    Serial.printf("ModbusRTU memory usage: %d bytes (no Logger overhead)\n", 
                  beforeHeap - afterModbus);
  #endif
  
  Serial.println("\nSetup complete. Sending test request in 2 seconds...");
}

void loop() {
  static uint32_t lastRequest = 0;
  
  // Send a test request every 5 seconds
  if (millis() - lastRequest > 5000) {
    lastRequest = millis();
    
    Serial.println("\n--- Sending Modbus request ---");
    
    // Try to read 10 holding registers from device 1, starting at address 0
    if (modbus.readHoldingRegisters(1, 0, 10)) {
      Serial.println("Request queued successfully");
    } else {
      Serial.println("Failed to queue request (queue full?)");
    }
    
    // The library will log internally based on the logging mode
    // Check your serial output to see the difference between modes
  }
  
  delay(10);
}

/*
Expected output comparison:

WITHOUT USE_CUSTOM_LOGGER:
- Uses ESP-IDF format: "I (12345) ModbusRTU: Message"
- No Logger singleton created
- Minimal memory usage

WITH USE_CUSTOM_LOGGER:
- Uses Logger format (as configured)
- Logger singleton uses ~17KB
- All logs go through Logger with its features
*/