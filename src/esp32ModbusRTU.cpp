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

esp32ModbusRTU::esp32ModbusRTU(HardwareSerial *serial, int8_t rtsPin) : TimeOutValue(TIMEOUT_MS),
                                                                        _serial(serial),
                                                                        _lastMillis(0),
                                                                        _interval(0),
                                                                        _rtsPin(rtsPin),
                                                                        _task(nullptr),
                                                                        _queue(nullptr),
                                                                        _shutdown(false)
{
  _queue = xQueueCreate(QUEUE_SIZE, sizeof(ModbusRequest *));
  if (_queue == nullptr) {
    #ifdef MODBUS_RTU_DEBUG
    MODBUS_LOG_E("Failed to create queue!");
    #endif
  }
}

esp32ModbusRTU::~esp32ModbusRTU()
{
  _shutdown = true;

  // Only proceed if queue was successfully created
  if (_queue != nullptr) {
    // Clear the queue
    xQueueReset(_queue);

    // We may be processing a modbus request, then the queue will be empty so we add another to know
    // that we are not processing a real modbus request
    readDiscreteInputs(0, 0, 0);

    // Wait until we have processed an outstanding modbus com request if present
    while (uxQueueMessagesWaiting(_queue) > 0)
    {
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

  // Delete queue if it exists
  if (_queue != nullptr) {
    vQueueDelete(_queue);
    _queue = nullptr;
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
  
  // Check if queue was created successfully
  if (_queue == nullptr) {
    #ifdef MODBUS_RTU_DEBUG
    MODBUS_LOG_E("Cannot begin - queue is null!");
    #endif
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
  else if (_queue == nullptr)
  {
    #ifdef MODBUS_RTU_DEBUG
    MODBUS_LOG_E("_addToQueue: queue is null");
    #endif
    delete request;
    return false;
  }
  else if (_task == nullptr)
  {
    #ifdef MODBUS_RTU_DEBUG
    MODBUS_LOG_E("_addToQueue: task is null");
    #endif
    delete request;
    return false;
  }
  else if (xQueueSend(_queue, reinterpret_cast<void *>(&request), (TickType_t)0) != pdPASS)
  {
    #ifdef MODBUS_RTU_DEBUG
    MODBUS_LOG_E("_addToQueue: queue send failed");
    #endif
    delete request;
    return false;
  }
  else
  {
    // Success - no need to log every successful queue operation
    return true;
  }
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
  // Static flags to track initialization
  static bool watchdogInitialized = false;
  static bool watchdogActive = false;
  
  // Only check/register once at task startup
  if (!watchdogInitialized) {
    watchdogInitialized = true;
    
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
        watchdogActive = true;
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
      watchdogActive = true;
      #ifdef MODBUS_RTU_DEBUG
      MODBUS_LOG_D("Task already registered with watchdog by external code, using existing registration");
      #endif
    } else {
      #ifdef MODBUS_RTU_DEBUG
      MODBUS_LOG_W("Unexpected watchdog status: %d", status);
      #endif
    }
  }
  #endif
  
  while (!instance->_shutdown)
  {
    ModbusRequest *request;
    // Use a timeout instead of portMAX_DELAY to allow periodic watchdog feeding
    if (instance->_queue && pdTRUE == xQueueReceive(instance->_queue, &request, pdMS_TO_TICKS(1000)))
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
      if (watchdogActive && !instance->_shutdown) {
        esp_task_wdt_reset();
      }
      #endif
    }
    else
    {
      #if MODBUS_USE_WATCHDOG
      // No message received within timeout, just feed the watchdog (only if registered and not shutting down)
      if (watchdogActive && !instance->_shutdown) {
        esp_task_wdt_reset();
      }
      #endif
    }
  }
  
  // Task is exiting due to shutdown
  #if MODBUS_USE_WATCHDOG
  if (watchdogActive) {
    // Try to unsubscribe cleanly before task exits
    TaskHandle_t currentTask = xTaskGetCurrentTaskHandle();
    esp_task_wdt_delete(currentTask);
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
  
  // Toggle rtsPin, if necessary
  if (_rtsPin >= 0)
    digitalWrite(_rtsPin, HIGH);
  _serial->write(data, length);
  _serial->flush();
  // Toggle rtsPin, if necessary
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

ModbusResponse *esp32ModbusRTU::_receive(ModbusRequest *request)
{
  // Get expected response length and validate
  size_t responseLen = request->responseLength();
  if (responseLen > MODBUS_MAX_MESSAGE_SIZE) {
    responseLen = MODBUS_MAX_MESSAGE_SIZE;
  }
  
  ModbusResponse *response = new ModbusResponse(responseLen, request);
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
    delay(1); // take care of watchdog
  }
  return response;
}

#elif defined ESP32MODBUSRTU_TEST

#else

#pragma message "no suitable platform"

#endif
