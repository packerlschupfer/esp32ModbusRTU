/* esp32ModbusRTU

Copyright 2018 Bert Melis

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef esp32ModbusRTU_h
#define esp32ModbusRTU_h

#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32) || defined(ESP_PLATFORM)

// Default configuration
#ifndef QUEUE_SIZE
#define QUEUE_SIZE 16  // Number of requests that can be queued
#endif

#ifndef TIMEOUT_MS
#define TIMEOUT_MS 5000  // Default timeout in milliseconds
#endif

// Maximum limits for memory safety
#ifndef MODBUS_MAX_REGISTERS
#define MODBUS_MAX_REGISTERS 125  // Maximum registers in single request
#endif

#ifndef MODBUS_MAX_COILS  
#define MODBUS_MAX_COILS 2000  // Maximum coils in single request
#endif

#ifndef MODBUS_MAX_MESSAGE_SIZE
#define MODBUS_MAX_MESSAGE_SIZE 256  // Maximum message size
#endif

#include <functional>

extern "C"
{
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
}
#include <HardwareSerial.h>
#include <esp32-hal-gpio.h>

#include "esp32ModbusTypeDefs.h"
#include "ModbusMessage.h"

// C++11 compatible logging configuration
// Define library log tag
#define MODBUS_LOG_TAG "ModbusRTU"

// Check if custom logger is requested
#ifdef USE_CUSTOM_LOGGER
    // When custom logger is enabled, use LogInterface
    // This assumes LogInterface.h is available when USE_CUSTOM_LOGGER is defined
    #include "LogInterface.h"
    #define MODBUS_LOG_E(...) LOG_ERROR(MODBUS_LOG_TAG, __VA_ARGS__)
    #define MODBUS_LOG_W(...) LOG_WARN(MODBUS_LOG_TAG, __VA_ARGS__)
    #define MODBUS_LOG_I(...) LOG_INFO(MODBUS_LOG_TAG, __VA_ARGS__)
    #define MODBUS_LOG_D(...) LOG_DEBUG(MODBUS_LOG_TAG, __VA_ARGS__)
    #define MODBUS_LOG_V(...) LOG_VERBOSE(MODBUS_LOG_TAG, __VA_ARGS__)
#else
    // Fall back to ESP-IDF logging
    #include <esp_log.h>
    #define MODBUS_LOG_E(...) ESP_LOGE(MODBUS_LOG_TAG, __VA_ARGS__)
    #define MODBUS_LOG_W(...) ESP_LOGW(MODBUS_LOG_TAG, __VA_ARGS__)
    #define MODBUS_LOG_I(...) ESP_LOGI(MODBUS_LOG_TAG, __VA_ARGS__)
    #define MODBUS_LOG_D(...) ESP_LOGD(MODBUS_LOG_TAG, __VA_ARGS__)
    #define MODBUS_LOG_V(...) ESP_LOGV(MODBUS_LOG_TAG, __VA_ARGS__)
#endif

// Optional debug logging controlled by library-specific flag
#ifdef MODBUS_RTU_DEBUG
    #define MODBUS_LOG_DEBUG(...) MODBUS_LOG_D(__VA_ARGS__)
#else
    #define MODBUS_LOG_DEBUG(...) // No-op
#endif

class esp32ModbusRTU
{
public:
  explicit esp32ModbusRTU(HardwareSerial *serial, int8_t rtsPin = -1);
  ~esp32ModbusRTU();
  void begin(int coreID = -1);
  bool readCoils(uint8_t slaveAddress, uint16_t address, uint16_t numberCoils);
  bool readDiscreteInputs(uint8_t slaveAddress, uint16_t address, uint16_t numberCoils);
  bool readHoldingRegisters(uint8_t slaveAddress, uint16_t address, uint16_t numberRegisters);
  bool readInputRegisters(uint8_t slaveAddress, uint16_t address, uint16_t numberRegisters);
  bool writeSingleCoil(uint8_t slaveAddress, uint16_t address, bool value);
  bool writeSingleHoldingRegister(uint8_t slaveAddress, uint16_t address, uint16_t data);
  bool writeMultipleCoils(uint8_t slaveAddress, uint16_t address, uint16_t numberCoils, bool *values);
  bool writeMultHoldingRegisters(uint8_t slaveAddress, uint16_t address, uint16_t numberRegisters, uint8_t *data);
  bool readWriteMultipleRegisters(uint8_t slaveAddress, uint16_t readAddress, uint16_t readCount, uint16_t writeAddress, uint16_t writeCount, uint16_t *writeData);
  void onData(esp32Modbus::MBRTUOnData handler);
  void onError(esp32Modbus::MBRTUOnError handler);
  void setTimeOutValue(uint32_t tov);

private:
  bool _addToQueue(esp32ModbusRTUInternals::ModbusRequest *request);
  static void _handleConnection(esp32ModbusRTU *instance);
  void _send(uint8_t *data, uint8_t length);
  esp32ModbusRTUInternals::ModbusResponse *_receive(esp32ModbusRTUInternals::ModbusRequest *request);

private:
  uint32_t TimeOutValue;
  HardwareSerial *_serial;
  uint32_t _lastMillis;
  uint32_t _interval;
  int8_t _rtsPin;
  TaskHandle_t _task;
  QueueHandle_t _queue;
  esp32Modbus::MBRTUOnData _onData;
  esp32Modbus::MBRTUOnError _onError;

  bool _shutdown = false;
};

#endif

#endif
