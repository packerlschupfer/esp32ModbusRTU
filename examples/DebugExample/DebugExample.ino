/**
 * Debug Example for esp32ModbusRTU
 * 
 * This example demonstrates the advanced debug features available
 * when MODBUS_RTU_DEBUG is defined.
 * 
 * Debug features include:
 * - Protocol-level tracing
 * - Buffer hex dumps
 * - Performance timing
 * - Detailed error information
 */

// To see debug output, define MODBUS_RTU_DEBUG in your platformio.ini:
// build_flags = -D MODBUS_RTU_DEBUG

#include <esp32ModbusRTU.h>

// Configuration
#define RX_PIN 16  // RO
#define TX_PIN 17  // DI  
#define RTS_PIN 4  // DE/RE
#define SLAVE_ADDR 1

// Create ModbusRTU instance
esp32ModbusRTU modbus(&Serial2, RTS_PIN);

// Statistics
struct {
    uint32_t requests = 0;
    uint32_t successes = 0;
    uint32_t errors = 0;
    uint32_t timeouts = 0;
    uint32_t crcErrors = 0;
} stats;

void handleData(uint8_t serverAddress, esp32Modbus::FunctionCode fc, uint16_t address, uint8_t* data, size_t length) {
    stats.successes++;
    
    Serial.println("\n=== MODBUS RESPONSE RECEIVED ===");
    Serial.printf("Server: 0x%02X, Function: 0x%02X, Address: 0x%04X\n", 
                  serverAddress, static_cast<uint8_t>(fc), address);
    
    switch (fc) {
        case esp32Modbus::FunctionCode::READ_HOLDING_REGISTERS:
        case esp32Modbus::FunctionCode::READ_INPUT_REGISTERS:
            Serial.println("Register values:");
            for (size_t i = 0; i < length; i += 2) {
                uint16_t value = (data[i] << 8) | data[i + 1];
                Serial.printf("  [%04X] = %u (0x%04X)\n", 
                            address + (i/2), value, value);
            }
            break;
            
        case esp32Modbus::FunctionCode::READ_COILS:
        case esp32Modbus::FunctionCode::READ_DISCRETE_INPUTS:
            Serial.println("Coil/Input values:");
            for (size_t i = 0; i < length; i++) {
                for (int bit = 0; bit < 8 && (i * 8 + bit) < length * 8; bit++) {
                    bool value = (data[i] >> bit) & 0x01;
                    Serial.printf("  [%04X] = %s\n", 
                                address + (i * 8 + bit), 
                                value ? "ON" : "OFF");
                }
            }
            break;
            
        default:
            Serial.println("Data bytes:");
            for (size_t i = 0; i < length; i++) {
                Serial.printf("  [%02d] = 0x%02X\n", i, data[i]);
            }
    }
}

void handleError(esp32Modbus::Error error) {
    stats.errors++;
    
    switch (error) {
        case esp32Modbus::Error::TIMEOUT:
            stats.timeouts++;
            break;
        case esp32Modbus::Error::CRC:
            stats.crcErrors++;
            break;
        default:
            break;
    }
    
    Serial.printf("\n!!! MODBUS ERROR: %s (0x%02X) !!!\n", 
                  esp32Modbus::getErrorDescription(error), 
                  static_cast<uint8_t>(error));
}

void printStats() {
    Serial.println("\n--- Statistics ---");
    Serial.printf("Total Requests: %u\n", stats.requests);
    Serial.printf("Successful:     %u (%.1f%%)\n", 
                  stats.successes, 
                  stats.requests > 0 ? (100.0 * stats.successes / stats.requests) : 0);
    Serial.printf("Errors:         %u\n", stats.errors);
    Serial.printf("  Timeouts:     %u\n", stats.timeouts);
    Serial.printf("  CRC Errors:   %u\n", stats.crcErrors);
    Serial.printf("  Other:        %u\n", stats.errors - stats.timeouts - stats.crcErrors);
}

void printDebugInfo() {
    Serial.println("\n=== DEBUG FEATURES ===");
    #ifdef MODBUS_RTU_DEBUG
        Serial.println("MODBUS_RTU_DEBUG is ENABLED");
        Serial.println("You will see:");
        Serial.println("- [PROTO] Protocol messages");
        Serial.println("- TX/RX buffer hex dumps");
        Serial.println("- [TIMING] Performance measurements");
    #else
        Serial.println("MODBUS_RTU_DEBUG is DISABLED");
        Serial.println("To enable debug output, add to platformio.ini:");
        Serial.println("build_flags = -D MODBUS_RTU_DEBUG");
    #endif
    
    Serial.println("\nCurrent log level:");
    Serial.printf("- ESP-IDF log level: %d\n", esp_log_level_get("ModbusRTU"));
}

void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(10); }
    
    Serial.println("\n=================================");
    Serial.println("ESP32 ModbusRTU Debug Example");
    Serial.println("=================================");
    
    // Configure ESP-IDF log level
    esp_log_level_set("*", ESP_LOG_INFO);      // Default level
    esp_log_level_set("ModbusRTU", ESP_LOG_VERBOSE); // Maximum for ModbusRTU
    
    printDebugInfo();
    
    // Configure serial for Modbus
    Serial2.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
    
    // Set callbacks
    modbus.onData(handleData);
    modbus.onError(handleError);
    
    // Configure timeout
    modbus.setTimeOutValue(1000);  // 1 second for testing
    
    // Start Modbus task
    modbus.begin();
    
    Serial.println("\nReady! Commands:");
    Serial.println("  h - Read holding registers");
    Serial.println("  i - Read input registers");
    Serial.println("  c - Read coils");
    Serial.println("  d - Read discrete inputs");
    Serial.println("  s - Show statistics");
    Serial.println("  ? - Show debug info");
}

void loop() {
    if (Serial.available()) {
        char cmd = Serial.read();
        // Clear any extra characters
        while (Serial.available()) Serial.read();
        
        bool queued = false;
        stats.requests++;
        
        switch (cmd) {
            case 'h':
            case 'H':
                Serial.println("\n>>> Reading 10 holding registers from address 0...");
                queued = modbus.readHoldingRegisters(SLAVE_ADDR, 0, 10);
                break;
                
            case 'i':
            case 'I':
                Serial.println("\n>>> Reading 10 input registers from address 100...");
                queued = modbus.readInputRegisters(SLAVE_ADDR, 100, 10);
                break;
                
            case 'c':
            case 'C':
                Serial.println("\n>>> Reading 16 coils from address 0...");
                queued = modbus.readCoils(SLAVE_ADDR, 0, 16);
                break;
                
            case 'd':
            case 'D':
                Serial.println("\n>>> Reading 16 discrete inputs from address 0...");
                queued = modbus.readDiscreteInputs(SLAVE_ADDR, 0, 16);
                break;
                
            case 's':
            case 'S':
                stats.requests--; // Don't count this as a request
                printStats();
                break;
                
            case '?':
                stats.requests--; // Don't count this as a request
                printDebugInfo();
                break;
                
            default:
                stats.requests--; // Don't count invalid commands
                Serial.println("Unknown command. Use h/i/c/d/s/?");
        }
        
        if (queued) {
            Serial.println("Request queued successfully");
        } else if (cmd != 's' && cmd != 'S' && cmd != '?') {
            Serial.println("Failed to queue request (queue full?)");
            stats.requests--;
            stats.errors++;
        }
    }
    
    delay(10);
}