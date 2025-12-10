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
#define QUEUE_SIZE 16  // Legacy: kept for backward compatibility
#endif

// Priority queue sizes (total: 28 slots)
#ifndef EMERGENCY_QUEUE_SIZE
#define EMERGENCY_QUEUE_SIZE 4  // Emergency shutdown, failsafe
#endif

#ifndef SENSOR_QUEUE_SIZE
#define SENSOR_QUEUE_SIZE 8  // Temperature/pressure sensor reads
#endif

#ifndef RELAY_QUEUE_SIZE
#define RELAY_QUEUE_SIZE 12  // Relay commands, mode switches (2 mode switches = 10 cmds)
#endif

#ifndef STATUS_QUEUE_SIZE
#define STATUS_QUEUE_SIZE 4  // Status/diagnostic reads
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

// Logging configuration
#include "esp32ModbusRTULogging.h"

class esp32ModbusRTU
{
public:
  explicit esp32ModbusRTU(HardwareSerial *serial, int8_t rtsPin = -1);
  ~esp32ModbusRTU();
  void begin(int coreID = -1);

  // ===== Legacy API (backward compatible) =====
  // These methods use default RELAY priority
  bool readCoils(uint8_t slaveAddress, uint16_t address, uint16_t numberCoils);
  bool readDiscreteInputs(uint8_t slaveAddress, uint16_t address, uint16_t numberCoils);
  bool readHoldingRegisters(uint8_t slaveAddress, uint16_t address, uint16_t numberRegisters);
  bool readInputRegisters(uint8_t slaveAddress, uint16_t address, uint16_t numberRegisters);
  bool writeSingleCoil(uint8_t slaveAddress, uint16_t address, bool value);
  bool writeSingleHoldingRegister(uint8_t slaveAddress, uint16_t address, uint16_t data);
  bool writeMultipleCoils(uint8_t slaveAddress, uint16_t address, uint16_t numberCoils, bool *values);
  bool writeMultHoldingRegisters(uint8_t slaveAddress, uint16_t address, uint16_t numberRegisters, uint8_t *data);
  bool readWriteMultipleRegisters(uint8_t slaveAddress, uint16_t readAddress, uint16_t readCount, uint16_t writeAddress, uint16_t writeCount, uint16_t *writeData);

  // ===== Priority API (new) =====
  // These methods allow specifying request priority
  bool readCoilsWithPriority(uint8_t slaveAddress, uint16_t address, uint16_t numberCoils, esp32Modbus::ModbusPriority priority);
  bool readDiscreteInputsWithPriority(uint8_t slaveAddress, uint16_t address, uint16_t numberCoils, esp32Modbus::ModbusPriority priority);
  bool readHoldingRegistersWithPriority(uint8_t slaveAddress, uint16_t address, uint16_t numberRegisters, esp32Modbus::ModbusPriority priority);
  bool readInputRegistersWithPriority(uint8_t slaveAddress, uint16_t address, uint16_t numberRegisters, esp32Modbus::ModbusPriority priority);
  bool writeSingleCoilWithPriority(uint8_t slaveAddress, uint16_t address, bool value, esp32Modbus::ModbusPriority priority);
  bool writeSingleHoldingRegisterWithPriority(uint8_t slaveAddress, uint16_t address, uint16_t data, esp32Modbus::ModbusPriority priority);
  bool writeMultipleCoilsWithPriority(uint8_t slaveAddress, uint16_t address, uint16_t numberCoils, bool *values, esp32Modbus::ModbusPriority priority);
  bool writeMultHoldingRegistersWithPriority(uint8_t slaveAddress, uint16_t address, uint16_t numberRegisters, uint8_t *data, esp32Modbus::ModbusPriority priority);
  bool readWriteMultipleRegistersWithPriority(uint8_t slaveAddress, uint16_t readAddress, uint16_t readCount, uint16_t writeAddress, uint16_t writeCount, uint16_t *writeData, esp32Modbus::ModbusPriority priority);

  void onData(esp32Modbus::MBRTUOnData handler);
  void onError(esp32Modbus::MBRTUOnError handler);
  void setTimeOutValue(uint32_t tov);
  
  // Watchdog control methods
  void setWatchdogEnabled(bool enabled);
  bool isWatchdogEnabled() const;

private:
  bool _addToQueue(esp32ModbusRTUInternals::ModbusRequest *request);
  esp32ModbusRTUInternals::ModbusRequest* _dequeueByPriority();  // Dequeue from highest priority queue
  static void _handleConnection(esp32ModbusRTU *instance);
  void _send(uint8_t *data, uint8_t length);
  esp32ModbusRTUInternals::ModbusResponse *_receive(esp32ModbusRTUInternals::ModbusRequest *request);

  // Static member to track watchdog registration state across methods
  static bool _globalWatchdogActive;

private:
  uint32_t TimeOutValue;
  HardwareSerial *_serial;
  uint32_t _lastMillis;
  uint32_t _interval;
  int8_t _rtsPin;
  TaskHandle_t _task;
  QueueHandle_t _queues[4];  // Priority queues: [EMERGENCY, SENSOR, RELAY, STATUS]
  esp32Modbus::MBRTUOnData _onData;
  esp32Modbus::MBRTUOnError _onError;

  bool _shutdown = false;
  bool _watchdogEnabled = true;
};

#endif

#endif
