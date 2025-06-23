/**
 * C++11 Compatible Logging Example for esp32ModbusRTU
 * 
 * This example demonstrates how to use the library with both
 * ESP-IDF logging (default) and custom Logger logging.
 * 
 * The library is C++11 compatible and doesn't require any
 * C++17 features like __has_include.
 */

// Choose ONE of these configurations:
// Configuration 1: Use ESP-IDF logging (default, no dependencies)
// Configuration 2: Use custom Logger (uncomment line below)
// #define USE_CUSTOM_LOGGER

// When using custom Logger, include these BEFORE the library
#ifdef USE_CUSTOM_LOGGER
#include <Logger.h>
#include <LogInterfaceImpl.h>
#endif

#include <esp32ModbusRTU.h>

// Configuration
#define RX_PIN 16  // RO
#define TX_PIN 17  // DI  
#define RTS_PIN 4  // DE/RE
#define SLAVE_ADDR 1
#define REGISTER_ADDR 0
#define NUM_REGISTERS 10

// Create ModbusRTU instance
esp32ModbusRTU modbus(&Serial2, RTS_PIN);

// Error code to string helper
const char* getErrorString(esp32Modbus::Error error) {
    switch (error) {
        case esp32Modbus::Error::SUCCESS: return "Success";
        case esp32Modbus::Error::TIMEOUT: return "Timeout";
        case esp32Modbus::Error::SYNTAX: return "Invalid syntax";
        case esp32Modbus::Error::CRC: return "CRC error";
        case esp32Modbus::Error::EXCEPTION: return "Exception";
        case esp32Modbus::Error::LENGTH: return "Invalid length";
        case esp32Modbus::Error::SERVER_ADDR: return "Server address mismatch";
        case esp32Modbus::Error::QUEUE_FULL: return "Queue full";
        default: return "Unknown error";
    }
}

void handleData(uint8_t serverAddress, esp32Modbus::FunctionCode fc, uint8_t* data, size_t length) {
    Serial.printf("Received data from %u, FC %u:\n", serverAddress, static_cast<uint8_t>(fc));
    
    if (fc == esp32Modbus::FunctionCode::READ_HOLDING_REGISTERS) {
        for (size_t i = 0; i < length; i += 2) {
            uint16_t value = (data[i] << 8) | data[i + 1];
            Serial.printf("  Register[%u] = %u\n", i/2, value);
        }
    }
}

void handleError(esp32Modbus::Error error) {
    Serial.printf("Error: %s\n", getErrorString(error));
}

void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(10); }
    
    Serial.println("\nESP32 ModbusRTU C++11 Logging Example");
    Serial.println("=====================================");
    
    #ifdef USE_CUSTOM_LOGGER
        Serial.println("Using Custom Logger");
        
        // Initialize custom logger
        Logger& logger = Logger::getInstance();
        logger.init(1024);  // 1KB buffer
        logger.setLogLevel(ESP_LOG_DEBUG);
        logger.enableLogging(true);
        logger.setColorLogging(true);
        
        // Test logger
        logger.log(ESP_LOG_INFO, "Main", "Custom Logger initialized");
    #else
        Serial.println("Using ESP-IDF Logging (default)");
        
        // Configure ESP-IDF log level for ModbusRTU
        esp_log_level_set("ModbusRTU", ESP_LOG_DEBUG);
        esp_log_level_set("*", ESP_LOG_INFO);  // Default for others
        
        // Test ESP-IDF logging
        ESP_LOGI("Main", "ESP-IDF logging configured");
    #endif
    
    // Configure serial for Modbus
    Serial2.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
    
    // Set callbacks
    modbus.onData(handleData);
    modbus.onError(handleError);
    
    // Set timeout (optional)
    modbus.setTimeOutValue(3000);  // 3 seconds
    
    // Start Modbus task
    modbus.begin();
    
    Serial.println("\nModbus RTU initialized");
    Serial.println("The library will log using the selected logging system");
    Serial.println("Press any key to read holding registers...\n");
}

void loop() {
    // Wait for user input
    if (Serial.available()) {
        // Clear input buffer
        while (Serial.available()) {
            Serial.read();
        }
        
        Serial.println("Reading holding registers...");
        
        // Read holding registers
        if (!modbus.readHoldingRegisters(SLAVE_ADDR, REGISTER_ADDR, NUM_REGISTERS)) {
            Serial.println("Failed to queue request (queue full?)");
        }
    }
    
    // For custom logger, you could periodically flush logs
    #ifdef USE_CUSTOM_LOGGER
    static unsigned long lastFlush = 0;
    if (millis() - lastFlush > 1000) {
        lastFlush = millis();
        // Logger auto-flushes, but you could force it if needed
        // Logger::getInstance().flush();
    }
    #endif
    
    delay(100);
}