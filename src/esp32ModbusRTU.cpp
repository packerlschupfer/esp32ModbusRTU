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

#include "esp32ModbusRTU.h"

#if defined ARDUINO_ARCH_ESP32

#if !defined(MODBUS_DISABLE_WATCHDOG)
#include <esp_task_wdt.h>
#endif

// Optional external watchdog support
#ifdef MODBUS_USE_EXTERNAL_WATCHDOG
  #include "Watchdog.h"
  #define WATCHDOG_CLASS_AVAILABLE 1
#else
  #define WATCHDOG_CLASS_AVAILABLE 0
#endif

// Task configuration
#ifndef MODBUS_TASK_STACK_SIZE
#define MODBUS_TASK_STACK_SIZE 5120
#endif

#ifndef MODBUS_TASK_PRIORITY
#define MODBUS_TASK_PRIORITY 5
#endif

#ifndef MODBUS_TASK_NAME
#define MODBUS_TASK_NAME "ModbusRTU"
#endif


// Allow watchdog to be disabled via build flags
#ifdef MODBUS_DISABLE_WATCHDOG
  #define MODBUS_USE_WATCHDOG 0
  #warning "ModbusRTU: Watchdog support disabled by MODBUS_DISABLE_WATCHDOG"
#else
  #define MODBUS_USE_WATCHDOG 1
#endif

// Note: Safety limits are now defined in esp32ModbusRTU.h


using namespace esp32ModbusRTUInternals; // NOLINT

// Define static member for global watchdog state
bool esp32ModbusRTU::_globalWatchdogActive = false;

esp32ModbusRTU::esp32ModbusRTU(HardwareSerial *serial, int8_t rtsPin) : TimeOutValue(TIMEOUT_MS),
                                                                        _serial(serial),
                                                                        _lastMillis(0),
                                                                        _interval(0),
                                                                        _rtsPin(rtsPin),
                                                                        _task(nullptr),
                                                                        _shutdown(false)
{
  // Create 4 priority queues
  _queues[esp32Modbus::EMERGENCY] = xQueueCreate(EMERGENCY_QUEUE_SIZE, sizeof(ModbusRequest *));
  _queues[esp32Modbus::SENSOR] = xQueueCreate(SENSOR_QUEUE_SIZE, sizeof(ModbusRequest *));
  _queues[esp32Modbus::RELAY] = xQueueCreate(RELAY_QUEUE_SIZE, sizeof(ModbusRequest *));
  _queues[esp32Modbus::STATUS] = xQueueCreate(STATUS_QUEUE_SIZE, sizeof(ModbusRequest *));

  // Check if all queues were created successfully
  for (int i = 0; i < 4; i++) {
    if (_queues[i] == nullptr) {
      #ifdef MODBUS_RTU_DEBUG
      MODBUS_LOG_E("Failed to create priority queue %d!", i);
      #endif
    }
  }
}

esp32ModbusRTU::~esp32ModbusRTU()
{
  _shutdown = true;

  // Clear all priority queues
  for (int i = 0; i < 4; i++) {
    if (_queues[i] != nullptr) {
      xQueueReset(_queues[i]);
    }
  }

  // We may be processing a modbus request, then queues will be empty so we add another to know
  // that we are not processing a real modbus request
  readDiscreteInputs(0, 0, 0);

  // Wait until we have processed outstanding modbus requests in all queues
  bool queuesEmpty = false;
  while (!queuesEmpty) {
    queuesEmpty = true;
    for (int i = 0; i < 4; i++) {
      if (_queues[i] != nullptr && uxQueueMessagesWaiting(_queues[i]) > 0) {
        queuesEmpty = false;
        break;
      }
    }
    if (!queuesEmpty) {
      delay(1);
    }
  }

  // Delete task if it exists
  if (_task != nullptr) {
    // Note: We don't unsubscribe from watchdog here because:
    // 1. The task registered itself using xTaskGetCurrentTaskHandle()
    // 2. We only have the task handle from xTaskCreate, which might differ
    // 3. The task will be deleted anyway, removing it from watchdog
    vTaskDelete(_task);
    _task = nullptr;
  }

  // Delete all priority queues
  for (int i = 0; i < 4; i++) {
    if (_queues[i] != nullptr) {
      vQueueDelete(_queues[i]);
      _queues[i] = nullptr;
    }
  }
}

void esp32ModbusRTU::begin(int coreID /* = -1 */)
{
  // Log watchdog configuration
  #ifdef MODBUS_DISABLE_WATCHDOG
    MODBUS_LOG_D("Watchdog handling DISABLED by build flag");
  #else
    MODBUS_LOG_D("Watchdog handling ENABLED (MODBUS_USE_WATCHDOG=%d)", MODBUS_USE_WATCHDOG);
  #endif
  
  // Check if all queues were created successfully
  bool allQueuesCreated = true;
  for (int i = 0; i < 4; i++) {
    if (_queues[i] == nullptr) {
      allQueuesCreated = false;
      #ifdef MODBUS_RTU_DEBUG
      MODBUS_LOG_E("Cannot begin - queue[%d] is null!", i);
      #endif
    }
  }
  if (!allQueuesCreated) {
    return;
  }
  
  // If rtsPin is >=0, the RS485 adapter needs send/receive toggle
  if (_rtsPin >= 0)
  {
    pinMode(_rtsPin, OUTPUT);
    digitalWrite(_rtsPin, LOW);
  }
  
  BaseType_t taskResult = xTaskCreatePinnedToCore((TaskFunction_t)&_handleConnection, MODBUS_TASK_NAME, MODBUS_TASK_STACK_SIZE, this, MODBUS_TASK_PRIORITY, &_task, coreID >= 0 ? coreID : NULL);
  
  if (taskResult == pdPASS && _task != nullptr) {
    #ifdef MODBUS_RTU_DEBUG
    MODBUS_LOG_D("Task created successfully, handle=%p", _task);
    #endif
  } else {
    #ifdef MODBUS_RTU_DEBUG
    MODBUS_LOG_E("Failed to create task! Result=%d", taskResult);
    #endif
  }
  
  // silent interval is at least 3.5x character time
  _interval = 40000 / _serial->baudRate(); // 4 * 1000 * 10 / baud
  if (_interval == 0)
    _interval = 1; // minimum of 1msec interval
}

bool esp32ModbusRTU::readCoils(uint8_t slaveAddress, uint16_t address, uint16_t numberCoils)
{
  ModbusRequest *request = new ModbusRequest01(slaveAddress, address, numberCoils);
  return _addToQueue(request);
}

bool esp32ModbusRTU::readDiscreteInputs(uint8_t slaveAddress, uint16_t address, uint16_t numberCoils)
{
  ModbusRequest *request = new ModbusRequest02(slaveAddress, address, numberCoils);
  return _addToQueue(request);
}
bool esp32ModbusRTU::readHoldingRegisters(uint8_t slaveAddress, uint16_t address, uint16_t numberRegisters)
{
  ModbusRequest *request = new ModbusRequest03(slaveAddress, address, numberRegisters);
  return _addToQueue(request);
}

bool esp32ModbusRTU::readInputRegisters(uint8_t slaveAddress, uint16_t address, uint16_t numberRegisters)
{

  ModbusRequest *request = new ModbusRequest04(slaveAddress, address, numberRegisters);
  return _addToQueue(request);
}

bool esp32ModbusRTU::writeSingleCoil(uint8_t slaveAddress, uint16_t address, bool value)
{
  ModbusRequest *request = new ModbusRequest05(slaveAddress, address, value);
  return _addToQueue(request);
}

bool esp32ModbusRTU::writeSingleHoldingRegister(uint8_t slaveAddress, uint16_t address, uint16_t data)
{
  ModbusRequest *request = new ModbusRequest06(slaveAddress, address, data);
  return _addToQueue(request);
}

bool esp32ModbusRTU::writeMultipleCoils(uint8_t slaveAddress, uint16_t address, uint16_t numberCoils, bool *values)
{
  // Validate parameters
  if (numberCoils == 0 || numberCoils > MODBUS_MAX_COILS || values == nullptr) {
    #ifdef MODBUS_RTU_DEBUG
    MODBUS_LOG_E("writeMultipleCoils: Invalid parameters (coils=%d, max=%d)", numberCoils, MODBUS_MAX_COILS);
    #endif
    return false;
  }
  
  ModbusRequest *request = new ModbusRequest0F(slaveAddress, address, numberCoils, values);
  return _addToQueue(request);
}

bool esp32ModbusRTU::writeMultHoldingRegisters(uint8_t slaveAddress, uint16_t address, uint16_t numberRegisters, uint8_t *data)
{
  // Validate parameters
  if (numberRegisters == 0 || numberRegisters > MODBUS_MAX_REGISTERS || data == nullptr) {
    #ifdef MODBUS_RTU_DEBUG
    MODBUS_LOG_E("writeMultHoldingRegisters: Invalid parameters (registers=%d, max=%d)", numberRegisters, MODBUS_MAX_REGISTERS);
    #endif
    return false;
  }
  
  ModbusRequest *request = new ModbusRequest16(slaveAddress, address, numberRegisters, data);
  return _addToQueue(request);
}

bool esp32ModbusRTU::readWriteMultipleRegisters(uint8_t slaveAddress, uint16_t readAddress, uint16_t readCount, uint16_t writeAddress, uint16_t writeCount, uint16_t *writeData)
{
  // Validate parameters
  if (readCount == 0 || readCount > MODBUS_MAX_REGISTERS || 
      writeCount == 0 || writeCount > MODBUS_MAX_REGISTERS || 
      writeData == nullptr) {
    #ifdef MODBUS_RTU_DEBUG
    MODBUS_LOG_E("readWriteMultipleRegisters: Invalid parameters (read=%d, write=%d, max=%d)", 
                  readCount, writeCount, MODBUS_MAX_REGISTERS);
    #endif
    return false;
  }
  
  ModbusRequest *request = new ModbusRequest17(slaveAddress, readAddress, readCount, writeAddress, writeCount, writeData);
  return _addToQueue(request);
}

// ===== Priority API implementations =====

bool esp32ModbusRTU::readCoilsWithPriority(uint8_t slaveAddress, uint16_t address, uint16_t numberCoils, esp32Modbus::ModbusPriority priority)
{
  ModbusRequest *request = new ModbusRequest01(slaveAddress, address, numberCoils);
  request->setPriority(priority);
  return _addToQueue(request);
}

bool esp32ModbusRTU::readDiscreteInputsWithPriority(uint8_t slaveAddress, uint16_t address, uint16_t numberCoils, esp32Modbus::ModbusPriority priority)
{
  ModbusRequest *request = new ModbusRequest02(slaveAddress, address, numberCoils);
  request->setPriority(priority);
  return _addToQueue(request);
}

bool esp32ModbusRTU::readHoldingRegistersWithPriority(uint8_t slaveAddress, uint16_t address, uint16_t numberRegisters, esp32Modbus::ModbusPriority priority)
{
  ModbusRequest *request = new ModbusRequest03(slaveAddress, address, numberRegisters);
  request->setPriority(priority);
  return _addToQueue(request);
}

bool esp32ModbusRTU::readInputRegistersWithPriority(uint8_t slaveAddress, uint16_t address, uint16_t numberRegisters, esp32Modbus::ModbusPriority priority)
{
  ModbusRequest *request = new ModbusRequest04(slaveAddress, address, numberRegisters);
  request->setPriority(priority);
  return _addToQueue(request);
}

bool esp32ModbusRTU::writeSingleCoilWithPriority(uint8_t slaveAddress, uint16_t address, bool value, esp32Modbus::ModbusPriority priority)
{
  ModbusRequest *request = new ModbusRequest05(slaveAddress, address, value);
  request->setPriority(priority);
  return _addToQueue(request);
}

bool esp32ModbusRTU::writeSingleHoldingRegisterWithPriority(uint8_t slaveAddress, uint16_t address, uint16_t data, esp32Modbus::ModbusPriority priority)
{
  ModbusRequest *request = new ModbusRequest06(slaveAddress, address, data);
  request->setPriority(priority);
  return _addToQueue(request);
}

bool esp32ModbusRTU::writeMultipleCoilsWithPriority(uint8_t slaveAddress, uint16_t address, uint16_t numberCoils, bool *values, esp32Modbus::ModbusPriority priority)
{
  ModbusRequest *request = new ModbusRequest0F(slaveAddress, address, numberCoils, values);
  request->setPriority(priority);
  return _addToQueue(request);
}

bool esp32ModbusRTU::writeMultHoldingRegistersWithPriority(uint8_t slaveAddress, uint16_t address, uint16_t numberRegisters, uint8_t *data, esp32Modbus::ModbusPriority priority)
{
  ModbusRequest *request = new ModbusRequest16(slaveAddress, address, numberRegisters, data);
  request->setPriority(priority);
  return _addToQueue(request);
}

bool esp32ModbusRTU::readWriteMultipleRegistersWithPriority(uint8_t slaveAddress, uint16_t readAddress, uint16_t readCount, uint16_t writeAddress, uint16_t writeCount, uint16_t *writeData, esp32Modbus::ModbusPriority priority)
{
  // Validate parameters
  if (readCount == 0 || readCount > MODBUS_MAX_REGISTERS ||
      writeCount == 0 || writeCount > MODBUS_MAX_REGISTERS ||
      writeData == nullptr) {
    #ifdef MODBUS_RTU_DEBUG
    MODBUS_LOG_E("readWriteMultipleRegistersWithPriority: Invalid parameters (read=%d, write=%d, max=%d)",
                  readCount, writeCount, MODBUS_MAX_REGISTERS);
    #endif
    return false;
  }

  ModbusRequest *request = new ModbusRequest17(slaveAddress, readAddress, readCount, writeAddress, writeCount, writeData);
  request->setPriority(priority);
  return _addToQueue(request);
}

void esp32ModbusRTU::onData(esp32Modbus::MBRTUOnData handler)
{
  _onData = handler;
}

void esp32ModbusRTU::onError(esp32Modbus::MBRTUOnError handler)
{
  _onError = handler;
}

bool esp32ModbusRTU::_addToQueue(ModbusRequest *request)
{
  if (!request)
  {
    #ifdef MODBUS_RTU_DEBUG
    MODBUS_LOG_E("_addToQueue: request is null");
    #endif
    return false;
  }

  // Get priority from request
  esp32Modbus::ModbusPriority priority = request->getPriority();
  uint8_t queueIndex = static_cast<uint8_t>(priority);

  // Validate priority
  if (queueIndex >= 4)
  {
    #ifdef MODBUS_RTU_DEBUG
    MODBUS_LOG_E("_addToQueue: invalid priority %d", queueIndex);
    #endif
    delete request;
    return false;
  }

  // Check if priority queue exists
  if (_queues[queueIndex] == nullptr)
  {
    #ifdef MODBUS_RTU_DEBUG
    MODBUS_LOG_E("_addToQueue: queue[%d] is null", queueIndex);
    #endif
    delete request;
    return false;
  }

  // Check if task exists
  if (_task == nullptr)
  {
    #ifdef MODBUS_RTU_DEBUG
    MODBUS_LOG_E("_addToQueue: task is null");
    #endif
    delete request;
    return false;
  }

  // Enqueue into appropriate priority queue
  if (xQueueSend(_queues[queueIndex], reinterpret_cast<void *>(&request), (TickType_t)0) != pdPASS)
  {
    #ifdef MODBUS_RTU_DEBUG
    MODBUS_LOG_E("_addToQueue: queue[%d] send failed (priority: %s)",
                 queueIndex, esp32Modbus::getPriorityDescription(priority));
    #endif
    delete request;
    return false;
  }

  // Success - no need to log every successful queue operation
  return true;
}

ModbusRequest* esp32ModbusRTU::_dequeueByPriority()
{
  ModbusRequest* request = nullptr;

  // Check queues in priority order (EMERGENCY=0 first, STATUS=3 last)
  for (int priority = 0; priority < 4; priority++) {
    if (_queues[priority] != nullptr) {
      if (xQueueReceive(_queues[priority], &request, 0) == pdTRUE) {
        #ifdef MODBUS_RTU_DEBUG
        MODBUS_LOG_D("Dequeued request from priority %s queue",
                     esp32Modbus::getPriorityDescription(static_cast<esp32Modbus::ModbusPriority>(priority)));
        #endif
        return request;  // Found request in this priority
      }
    }
  }

  return nullptr;  // No requests in any queue
}

void esp32ModbusRTU::_handleConnection(esp32ModbusRTU *instance)
{
  // Debug: Log once at task start
  static bool taskStartLogged = false;
  if (!taskStartLogged) {
    taskStartLogged = true;
    #if MODBUS_USE_WATCHDOG
    MODBUS_LOG_D("Task starting WITH watchdog support");
    #else
    MODBUS_LOG_D("Task starting WITHOUT watchdog support (disabled)");
    #endif
  }
  
  #if MODBUS_USE_WATCHDOG
  // Static flag to track initialization
  static bool watchdogInitialized = false;
  
  // Only check/register once at task startup
  if (!watchdogInitialized) {
    watchdogInitialized = true;
    
    // Only register if watchdog is enabled
    if (instance->_watchdogEnabled) {
    
    #if WATCHDOG_CLASS_AVAILABLE
      // Use external Watchdog class
      if (Watchdog::isGloballyInitialized()) {
        if (Watchdog::quickRegister(MODBUS_TASK_NAME)) {
          _globalWatchdogActive = true;
          MODBUS_LOG_D("Task registered with external Watchdog class");
        } else {
          MODBUS_LOG_E("Failed to register with external Watchdog class");
        }
      } else {
        MODBUS_LOG_D("External Watchdog not initialized, skipping registration");
      }
    #else
      // Use ESP-IDF watchdog directly
      // First check if we're already subscribed
      TaskHandle_t currentTask = xTaskGetCurrentTaskHandle();
      esp_err_t status = esp_task_wdt_status(currentTask);
      
      #ifdef MODBUS_RTU_DEBUG
      MODBUS_LOG_D("Watchdog status check: %d (ESP_OK=%d, ESP_ERR_NOT_FOUND=%d)", 
                    status, ESP_OK, ESP_ERR_NOT_FOUND);
      #endif
      
      if (status == ESP_ERR_NOT_FOUND) {
        // Not subscribed yet, so subscribe
        esp_err_t result = esp_task_wdt_add(currentTask);
        if (result == ESP_OK) {
          _globalWatchdogActive = true;
          #ifdef MODBUS_RTU_DEBUG
          MODBUS_LOG_D("Task successfully registered with watchdog");
          #endif
        } else {
          #ifdef MODBUS_RTU_DEBUG
          MODBUS_LOG_E("Failed to register with watchdog: error=%d", result);
          #endif
        }
      } else if (status == ESP_OK) {
        // Already subscribed (probably by the main application)
        _globalWatchdogActive = true;
        #ifdef MODBUS_RTU_DEBUG
        MODBUS_LOG_D("Task already registered with watchdog by external code, using existing registration");
        #endif
      } else {
        #ifdef MODBUS_RTU_DEBUG
        MODBUS_LOG_W("Unexpected watchdog status: %d", status);
        #endif
      }
    #endif
    }
  }
  #endif
  
  while (!instance->_shutdown)
  {
    ModbusRequest *request = nullptr;

    // Try to dequeue from priority queues (non-blocking check)
    request = instance->_dequeueByPriority();

    // If we got a request, process it
    if (request != nullptr)
    {
      if (instance->_shutdown)
      {
        delete request;
        break;  // Exit the loop on shutdown
      }

      // block and wait for queued item
      MODBUS_TIME_START();
      instance->_send(request->getMessage(), request->getSize());
      ModbusResponse *response = instance->_receive(request);
      MODBUS_TIME_END("Request/Response cycle");
      
      if (response->isSuccess())
      {
        if (instance->_onData)
          instance->_onData(response->getSlaveAddress(), response->getFunctionCode(), request->getAddress(), response->getData(), response->getByteCount());
      }
      else
      {
        // Log basic protocol errors (always visible)
        esp32Modbus::Error error = response->getError();
        MODBUS_LOG_E("Modbus error from address 0x%02X: %s (0x%02X)", 
                     request->getSlaveAddress(), 
                     esp32Modbus::getErrorDescription(error), 
                     static_cast<uint8_t>(error));
        
        if (instance->_onError)
          instance->_onError(error);
      }
      delete request;  // object created in public methods
      delete response; // object created in _receive()
      
      #if MODBUS_USE_WATCHDOG
      // Feed watchdog after processing request (only if registered and not shutting down)
      if (_globalWatchdogActive && !instance->_shutdown && instance->_watchdogEnabled) {
        #if WATCHDOG_CLASS_AVAILABLE
          Watchdog::quickFeed();
        #else
          esp_task_wdt_reset();
        #endif
      }
      #endif
    }
    else
    {
      // No requests available in any priority queue - wait before checking again
      vTaskDelay(pdMS_TO_TICKS(100));  // 100ms delay to avoid busy-waiting

      #if MODBUS_USE_WATCHDOG
      // No message received, feed the watchdog (only if registered and not shutting down)
      if (_globalWatchdogActive && !instance->_shutdown && instance->_watchdogEnabled) {
        #if WATCHDOG_CLASS_AVAILABLE
          Watchdog::quickFeed();
        #else
          esp_task_wdt_reset();
        #endif
      }
      #endif
    }
  }
  
  // Task is exiting due to shutdown
  #if MODBUS_USE_WATCHDOG
  if (_globalWatchdogActive && instance->_watchdogEnabled) {
    // Try to unsubscribe cleanly before task exits
    #if WATCHDOG_CLASS_AVAILABLE
      Watchdog& watchdog = Watchdog::getInstance();
      watchdog.unregisterCurrentTask();
    #else
      TaskHandle_t currentTask = xTaskGetCurrentTaskHandle();
      esp_task_wdt_delete(currentTask);
    #endif
  }
  #endif
  
  // Delete ourselves
  vTaskDelete(NULL);
}

void esp32ModbusRTU::_send(uint8_t *data, uint8_t length)
{
  // Validate inputs
  if (!data || length == 0) {
    MODBUS_LOG_E("_send called with invalid data or length");
    return;
  }
  
  while (millis() - _lastMillis < _interval)
    delay(1); // respect _interval
  
  // Debug logging with buffer dump
  MODBUS_LOG_PROTO("Sending %d bytes to address 0x%02X, FC=0x%02X", length, data[0], data[1]);
  MODBUS_DUMP_BUFFER("TX", data, length);
  
  // Toggle rtsPin to TX mode
  if (_rtsPin >= 0)
    digitalWrite(_rtsPin, HIGH);
  _serial->write(data, length);
  _serial->flush();

  // CRITICAL: Wait for last byte to physically transmit before switching to RX
  // flush() only waits for TX buffer to empty to UART, not for physical transmission.
  // At 9600 baud: 1 char = 10 bits / 9600 = 1.04ms. Add margin for safety.
  // Calculate based on actual baud rate: (10 bits * 1000000us) / baud + 500us margin
  uint32_t charTimeUs = (10 * 1000000UL) / _serial->baudRate();
  delayMicroseconds(charTimeUs + 500);  // Last char time + 500us margin

  // Toggle rtsPin to RX mode
  if (_rtsPin >= 0)
    digitalWrite(_rtsPin, LOW);
  _lastMillis = millis();
}

// Adjust timeout on MODBUS - some slaves require longer/allow for shorter times
void esp32ModbusRTU::setTimeOutValue(uint32_t tov)
{
  if (tov)
    TimeOutValue = tov;
}

// Control watchdog behavior
void esp32ModbusRTU::setWatchdogEnabled(bool enabled)
{
  _watchdogEnabled = enabled;
  
  #if MODBUS_USE_WATCHDOG
    // If we have a task handle, actually register/unregister with watchdog
    if (_task != nullptr) {
      TaskHandle_t taskHandle = _task;
      
      #if WATCHDOG_CLASS_AVAILABLE
        if (enabled) {
          // Register with external Watchdog if available
          if (Watchdog::isGloballyInitialized()) {
            // Note: This won't work from different task context with external Watchdog
            // The task itself needs to register, so we just set the flag
            MODBUS_LOG_D("Watchdog enabled (external Watchdog class)");
          }
        } else {
          // Unregister from external Watchdog
          Watchdog& watchdog = Watchdog::getInstance();
          watchdog.unregisterTaskByHandle(taskHandle, MODBUS_TASK_NAME);
          MODBUS_LOG_D("Watchdog disabled (external Watchdog class)");
        }
      #else
        // Use ESP-IDF watchdog directly
        if (enabled) {
          // Check if already registered
          esp_err_t status = esp_task_wdt_status(taskHandle);
          if (status == ESP_ERR_NOT_FOUND) {
            // Not registered, so register
            esp_err_t result = esp_task_wdt_add(taskHandle);
            if (result == ESP_OK) {
              _globalWatchdogActive = true;
              MODBUS_LOG_D("Watchdog enabled (registered with ESP-IDF)");
            } else {
              MODBUS_LOG_E("Failed to enable watchdog: error=%d", result);
            }
          } else {
            MODBUS_LOG_D("Watchdog enabled (already registered)");
          }
        } else {
          // Unregister from ESP-IDF watchdog
          esp_err_t result = esp_task_wdt_delete(taskHandle);
          if (result == ESP_OK) {
            _globalWatchdogActive = false;
            MODBUS_LOG_D("Watchdog disabled (unregistered from ESP-IDF)");
          } else if (result == ESP_ERR_NOT_FOUND) {
            _globalWatchdogActive = false;
            MODBUS_LOG_D("Watchdog disabled (was not registered)");
          } else {
            MODBUS_LOG_E("Failed to disable watchdog: error=%d", result);
          }
        }
      #endif
    } else {
      MODBUS_LOG_D("Watchdog %s (task not yet started)", enabled ? "will be enabled" : "will be disabled");
    }
  #else
    MODBUS_LOG_D("Watchdog support not compiled in");
  #endif
}

bool esp32ModbusRTU::isWatchdogEnabled() const
{
  return _watchdogEnabled;
}

ModbusResponse *esp32ModbusRTU::_receive(ModbusRequest *request)
{
  // Get expected response length and validate
  size_t responseLen = request->responseLength();
  if (responseLen > MODBUS_MAX_MESSAGE_SIZE) {
    responseLen = MODBUS_MAX_MESSAGE_SIZE;
  }
  
  ModbusResponse *response = new ModbusResponse(responseLen, request);
  uint32_t lastWatchdogFeed = millis();
  
  while (true)
  {
    while (_serial->available())
    {
      response->add(_serial->read());
    }
    if (response->isComplete())
    {
      _lastMillis = millis();
      MODBUS_LOG_PROTO("Response complete: %d bytes received", response->getSize());
      MODBUS_DUMP_BUFFER("RX", response->getData(), response->getSize());
      break;
    }
    if (millis() - _lastMillis > TimeOutValue)
    {
      MODBUS_LOG_PROTO("Response timeout after %lu ms", TimeOutValue);
      break;
    }
    
    // Feed watchdog periodically during long waits
    if (_watchdogEnabled && (millis() - lastWatchdogFeed > 500))
    {
      #if WATCHDOG_CLASS_AVAILABLE
        Watchdog::quickFeed();
      #elif MODBUS_USE_WATCHDOG
        if (_globalWatchdogActive) {
          esp_task_wdt_reset();
        }
      #endif
      lastWatchdogFeed = millis();
    }
    
    delay(1); // small delay to prevent CPU hogging
  }
  return response;
}

#elif defined ESP32MODBUSRTU_TEST

#else

#pragma message "no suitable platform"

#endif
