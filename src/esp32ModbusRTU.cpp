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

#include <esp_task_wdt.h>

// Allow stack size to be configured via build flags
#ifndef MODBUS_TASK_STACK_SIZE
#define MODBUS_TASK_STACK_SIZE 5120
#endif

// Allow watchdog to be disabled via build flags
#ifndef MODBUS_DISABLE_WATCHDOG
#define MODBUS_USE_WATCHDOG 1
#else
#define MODBUS_USE_WATCHDOG 0
#endif

// Static task name to prevent corruption
static const char* MODBUS_TASK_NAME = "esp32ModbusRTU";

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
    Serial.printf("[ModbusRTU] ERROR: Failed to create queue!\n");
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
    #if MODBUS_USE_WATCHDOG
    // Try to unsubscribe from watchdog before deleting task
    // Note: This may fail if the task wasn't registered, which is fine
    esp_task_wdt_delete(_task);
    #endif
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
  // Check if queue was created successfully
  if (_queue == nullptr) {
    #ifdef MODBUS_RTU_DEBUG
    Serial.printf("[ModbusRTU] ERROR: Cannot begin - queue is null!\n");
    #endif
    return;
  }
  
  // If rtsPin is >=0, the RS485 adapter needs send/receive toggle
  if (_rtsPin >= 0)
  {
    pinMode(_rtsPin, OUTPUT);
    digitalWrite(_rtsPin, LOW);
  }
  
  BaseType_t taskResult = xTaskCreatePinnedToCore((TaskFunction_t)&_handleConnection, MODBUS_TASK_NAME, MODBUS_TASK_STACK_SIZE, this, 5, &_task, coreID >= 0 ? coreID : NULL);
  
  if (taskResult == pdPASS && _task != nullptr) {
    #ifdef MODBUS_RTU_DEBUG
    Serial.printf("[ModbusRTU] Task created successfully, handle=%p\n", _task);
    #endif
  } else {
    #ifdef MODBUS_RTU_DEBUG
    Serial.printf("[ModbusRTU] Failed to create task! Result=%d\n", taskResult);
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
  ModbusRequest *request = new ModbusRequest0F(slaveAddress, address, numberCoils, values);
  return _addToQueue(request);
}

bool esp32ModbusRTU::writeMultHoldingRegisters(uint8_t slaveAddress, uint16_t address, uint16_t numberRegisters, uint8_t *data)
{
  ModbusRequest *request = new ModbusRequest16(slaveAddress, address, numberRegisters, data);
  return _addToQueue(request);
}

bool esp32ModbusRTU::readWriteMultipleRegisters(uint8_t slaveAddress, uint16_t readAddress, uint16_t readCount, uint16_t writeAddress, uint16_t writeCount, uint16_t *writeData)
{
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
    Serial.printf("[ModbusRTU] _addToQueue: request is null\n");
    #endif
    return false;
  }
  else if (_queue == nullptr)
  {
    #ifdef MODBUS_RTU_DEBUG
    Serial.printf("[ModbusRTU] _addToQueue: queue is null\n");
    #endif
    delete request;
    return false;
  }
  else if (_task == nullptr)
  {
    Serial.printf("[ModbusRTU] _addToQueue: task is null\n");
    delete request;
    return false;
  }
  else if (xQueueSend(_queue, reinterpret_cast<void *>(&request), (TickType_t)0) != pdPASS)
  {
    Serial.printf("[ModbusRTU] _addToQueue: queue send failed\n");
    delete request;
    return false;
  }
  else
  {
    Serial.printf("[ModbusRTU] _addToQueue: successfully queued request\n");
    return true;
  }
}

void esp32ModbusRTU::_handleConnection(esp32ModbusRTU *instance)
{
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
    Serial.printf("[ModbusRTU] Watchdog status check: %d (ESP_OK=%d, ESP_ERR_NOT_FOUND=%d)\n", 
                  status, ESP_OK, ESP_ERR_NOT_FOUND);
    #endif
    
    if (status == ESP_ERR_NOT_FOUND) {
      // Not subscribed yet, so subscribe
      esp_err_t result = esp_task_wdt_add(currentTask);
      if (result == ESP_OK) {
        watchdogActive = true;
        #ifdef MODBUS_RTU_DEBUG
        Serial.printf("[ModbusRTU] Task successfully registered with watchdog\n");
        #endif
      } else {
        #ifdef MODBUS_RTU_DEBUG
        Serial.printf("[ModbusRTU] Failed to register with watchdog: error=%d\n", result);
        #endif
      }
    } else if (status == ESP_OK) {
      // Already subscribed (probably by the main application)
      watchdogActive = true;
      #ifdef MODBUS_RTU_DEBUG
      Serial.printf("[ModbusRTU] Task already registered with watchdog by external code, using existing registration\n");
      #endif
    } else {
      #ifdef MODBUS_RTU_DEBUG
      Serial.printf("[ModbusRTU] Unexpected watchdog status: %d\n", status);
      #endif
    }
  }
  #endif
  
  while (1)
  {
    ModbusRequest *request;
    // Use a timeout instead of portMAX_DELAY to allow periodic watchdog feeding
    if (instance->_queue && pdTRUE == xQueueReceive(instance->_queue, &request, pdMS_TO_TICKS(1000)))
    {
      if (instance->_shutdown)
      {
        delete request;
        continue;
      }

      // block and wait for queued item
      instance->_send(request->getMessage(), request->getSize());
      ModbusResponse *response = instance->_receive(request);
      
      if (response->isSucces())
      {
        if (instance->_onData)
          instance->_onData(response->getSlaveAddress(), response->getFunctionCode(), request->getAddress(), response->getData(), response->getByteCount());
      }
      else
      {
        if (instance->_onError)
          instance->_onError(response->getError());
      }
      delete request;  // object created in public methods
      delete response; // object created in _receive()
      
      #if MODBUS_USE_WATCHDOG
      // Feed watchdog after processing request (only if registered)
      if (watchdogActive) {
        esp_task_wdt_reset();
      }
      #endif
    }
    else
    {
      #if MODBUS_USE_WATCHDOG
      // No message received within timeout, just feed the watchdog (only if registered)
      if (watchdogActive) {
        esp_task_wdt_reset();
      }
      #endif
    }
  }
}

void esp32ModbusRTU::_send(uint8_t *data, uint8_t length)
{
  // Validate inputs
  if (!data || length == 0) {
    Serial.printf("[ModbusRTU] ERROR: _send called with invalid data or length\n");
    return;
  }
  
  while (millis() - _lastMillis < _interval)
    delay(1); // respect _interval
  
  // Debug log
  #ifdef MODBUS_RTU_DEBUG
  Serial.printf("[ModbusRTU] Sending %d bytes to address 0x%02X\n", length, data[0]);
  #endif
  
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
  ModbusResponse *response = new ModbusResponse(request->responseLength(), request);
  while (true)
  {
    while (_serial->available())
    {
      response->add(_serial->read());
    }
    if (response->isComplete())
    {
      _lastMillis = millis();
      break;
    }
    if (millis() - _lastMillis > TimeOutValue)
    {
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
