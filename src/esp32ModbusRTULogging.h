#ifndef ESP32MODBUS_LOGGING_H
#define ESP32MODBUS_LOGGING_H

#define MODBUS_LOG_TAG "ModbusRTU"

// Define log levels based on debug flag
#ifdef MODBUS_RTU_DEBUG
    // Debug mode: Show all levels
    #define MODBUS_LOG_LEVEL_E ESP_LOG_ERROR
    #define MODBUS_LOG_LEVEL_W ESP_LOG_WARN
    #define MODBUS_LOG_LEVEL_I ESP_LOG_INFO
    #define MODBUS_LOG_LEVEL_D ESP_LOG_DEBUG
    #define MODBUS_LOG_LEVEL_V ESP_LOG_VERBOSE
#else
    // Release mode: Only Error, Warn, Info
    #define MODBUS_LOG_LEVEL_E ESP_LOG_ERROR
    #define MODBUS_LOG_LEVEL_W ESP_LOG_WARN
    #define MODBUS_LOG_LEVEL_I ESP_LOG_INFO
    #define MODBUS_LOG_LEVEL_D ESP_LOG_NONE  // Suppress
    #define MODBUS_LOG_LEVEL_V ESP_LOG_NONE  // Suppress
#endif

// Route to custom logger or ESP-IDF
#ifdef USE_CUSTOM_LOGGER
    #include <Logger.h>
    #define MODBUS_LOG_E(...) Logger::getInstance().log(MODBUS_LOG_LEVEL_E, MODBUS_LOG_TAG, __VA_ARGS__)
    #define MODBUS_LOG_W(...) Logger::getInstance().log(MODBUS_LOG_LEVEL_W, MODBUS_LOG_TAG, __VA_ARGS__)
    #define MODBUS_LOG_I(...) Logger::getInstance().log(MODBUS_LOG_LEVEL_I, MODBUS_LOG_TAG, __VA_ARGS__)
    #define MODBUS_LOG_D(...) Logger::getInstance().log(MODBUS_LOG_LEVEL_D, MODBUS_LOG_TAG, __VA_ARGS__)
    #define MODBUS_LOG_V(...) Logger::getInstance().log(MODBUS_LOG_LEVEL_V, MODBUS_LOG_TAG, __VA_ARGS__)
#else
    // ESP-IDF logging with compile-time suppression
    #include <esp_log.h>
    #define MODBUS_LOG_E(...) ESP_LOGE(MODBUS_LOG_TAG, __VA_ARGS__)
    #define MODBUS_LOG_W(...) ESP_LOGW(MODBUS_LOG_TAG, __VA_ARGS__)
    #define MODBUS_LOG_I(...) ESP_LOGI(MODBUS_LOG_TAG, __VA_ARGS__)
    #ifdef MODBUS_RTU_DEBUG
        #define MODBUS_LOG_D(...) ESP_LOGD(MODBUS_LOG_TAG, __VA_ARGS__)
        #define MODBUS_LOG_V(...) ESP_LOGV(MODBUS_LOG_TAG, __VA_ARGS__)
    #else
        #define MODBUS_LOG_D(...) ((void)0)
        #define MODBUS_LOG_V(...) ((void)0)
    #endif
#endif

// Feature-specific debug flags
#ifdef MODBUS_RTU_DEBUG
    #define MODBUS_DEBUG_PROTOCOL   // Protocol-level debugging
    #define MODBUS_DEBUG_TIMING     // Performance timing
    #define MODBUS_DEBUG_BUFFER     // Buffer dumps
#endif

// Protocol debug logging
#ifdef MODBUS_DEBUG_PROTOCOL
    #define MODBUS_LOG_PROTO(...) MODBUS_LOG_D("[PROTO] " __VA_ARGS__)
#else
    #define MODBUS_LOG_PROTO(...) ((void)0)
#endif

// Buffer dump macro
#ifdef MODBUS_DEBUG_BUFFER
    #define MODBUS_DUMP_BUFFER(msg, buf, len) do { \
        MODBUS_LOG_D("%s (%d bytes):", msg, len); \
        for (int i = 0; i < len; i++) { \
            if (i % 16 == 0 && i > 0) { \
                MODBUS_LOG_D("  [%02d] = 0x%02X", i, buf[i]); \
            } else { \
                printf(" %02X", buf[i]); \
                if (i == len - 1) printf("\n"); \
            } \
        } \
    } while(0)
#else
    #define MODBUS_DUMP_BUFFER(msg, buf, len) ((void)0)
#endif

// Performance timing macros
#ifdef MODBUS_DEBUG_TIMING
    #define MODBUS_TIME_START() unsigned long _modbus_start = millis()
    #define MODBUS_TIME_END(msg) MODBUS_LOG_D("[TIMING] %s took %lu ms", msg, millis() - _modbus_start)
#else
    #define MODBUS_TIME_START() ((void)0)
    #define MODBUS_TIME_END(msg) ((void)0)
#endif

#endif // ESP32MODBUS_LOGGING_H