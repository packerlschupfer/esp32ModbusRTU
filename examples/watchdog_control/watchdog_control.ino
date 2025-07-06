/*
 * ESP32 ModbusRTU Watchdog Control Example
 * 
 * This example demonstrates how to use the watchdog control features
 * to safely handle device initialization with retry logic.
 */

#include <Arduino.h>
#include <esp32ModbusRTU.h>

// Modbus configuration
#define RX_PIN 16
#define TX_PIN 17
#define RTS_PIN 4

esp32ModbusRTU modbus(&Serial2, RTS_PIN);

// Device configuration
const uint8_t TEMP_SENSOR_ADDR = 0x01;
const uint16_t TEMP_REGISTER = 0x0000;
bool deviceInitialized = false;

void handleData(uint8_t serverAddress, esp32Modbus::FunctionCode fc, uint16_t address, uint8_t* data, size_t length) {
  Serial.printf("Response from %d, FC %d: ", serverAddress, fc);
  for (size_t i = 0; i < length; i++) {
    Serial.printf("%02X ", data[i]);
  }
  Serial.println();
  
  // Mark device as initialized on successful response
  if (serverAddress == TEMP_SENSOR_ADDR) {
    deviceInitialized = true;
  }
}

void handleError(esp32Modbus::Error error) {
  Serial.printf("Modbus error: 0x%02X\n", static_cast<uint8_t>(error));
}

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }
  
  Serial.println("\n\nESP32 ModbusRTU Watchdog Control Example");
  Serial.println("=========================================");
  
  // Configure Serial2 for Modbus
  Serial2.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
  
  // Set callbacks
  modbus.onData(handleData);
  modbus.onError(handleError);
  
  // IMPORTANT: Disable watchdog before device initialization
  Serial.println("\nDisabling watchdog for initialization phase...");
  modbus.setWatchdogEnabled(false);
  
  // Start Modbus task
  modbus.begin();
  
  // Initialize device with retry logic
  int retryCount = 0;
  const int MAX_RETRIES = 10;
  
  Serial.println("\nAttempting to initialize temperature sensor...");
  
  while (!deviceInitialized && retryCount < MAX_RETRIES) {
    retryCount++;
    Serial.printf("Initialization attempt %d/%d...\n", retryCount, MAX_RETRIES);
    
    // Try to read from device
    modbus.readHoldingRegisters(TEMP_SENSOR_ADDR, TEMP_REGISTER, 1);
    
    // Wait for response with a long timeout
    // This would normally trigger watchdog, but we disabled it
    unsigned long startTime = millis();
    while (!deviceInitialized && (millis() - startTime < 5000)) {
      delay(100);
    }
    
    if (!deviceInitialized) {
      Serial.println("Device not responding, waiting 5 seconds before retry...");
      // Long delay is safe because watchdog is disabled
      delay(5000);
    }
  }
  
  // Re-enable watchdog after initialization
  Serial.println("\nRe-enabling watchdog for normal operation...");
  modbus.setWatchdogEnabled(true);
  
  if (deviceInitialized) {
    Serial.println("Device initialized successfully!");
  } else {
    Serial.println("Failed to initialize device after all retries.");
  }
  
  Serial.println("\nEntering main loop...");
}

void loop() {
  static unsigned long lastRead = 0;
  
  // Read temperature every 2 seconds
  if (millis() - lastRead > 2000) {
    lastRead = millis();
    
    if (deviceInitialized) {
      Serial.println("Reading temperature...");
      modbus.readHoldingRegisters(TEMP_SENSOR_ADDR, TEMP_REGISTER, 1);
    }
  }
  
  // Check watchdog status
  static unsigned long lastStatus = 0;
  if (millis() - lastStatus > 10000) {
    lastStatus = millis();
    Serial.printf("Watchdog is %s\n", 
                  modbus.isWatchdogEnabled() ? "enabled" : "disabled");
  }
  
  delay(10);
}