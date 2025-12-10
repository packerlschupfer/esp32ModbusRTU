/* esp32ModbusTypedefs

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

#pragma once

#ifndef esp32Modbus_esp32ModbusTypeDefs_h
#define esp32Modbus_esp32ModbusTypeDefs_h

#include <stdint.h>  // for uint*_t
#include <functional>  // for std::function

namespace esp32Modbus {

enum FunctionCode : uint8_t {
  READ_COIL            = 0x01,
  READ_DISCR_INPUT     = 0x02,
  READ_HOLD_REGISTER   = 0x03,
  READ_INPUT_REGISTER  = 0x04,
  WRITE_COIL           = 0x05,
  WRITE_HOLD_REGISTER  = 0x06,
  WRITE_MULT_COILS     = 0x0F,
  WRITE_MULT_REGISTERS = 0x10,
  READ_WRITE_MULT_REGISTERS = 0x17
};

enum Error : uint8_t {
  SUCCESS               = 0x00,  // Correct spelling
  SUCCES                = 0x00,  // Deprecated: kept for backward compatibility
  ILLEGAL_FUNCTION      = 0x01,
  ILLEGAL_DATA_ADDRESS  = 0x02,
  ILLEGAL_DATA_VALUE    = 0x03,
  SERVER_DEVICE_FAILURE = 0x04,
  ACKNOWLEDGE           = 0x05,
  SERVER_DEVICE_BUSY    = 0x06,
  NEGATIVE_ACKNOWLEDGE  = 0x07,
  MEMORY_PARITY_ERROR   = 0x08,
  TIMEOUT               = 0xE0,
  INVALID_SLAVE         = 0xE1,
  INVALID_FUNCTION      = 0xE2,
  CRC_ERROR             = 0xE3,  // only for Modbus-RTU
  COMM_ERROR            = 0xE4,  // general communication error
  INVALID_PARAMETER     = 0xE5,  // invalid function parameter
  QUEUE_FULL            = 0xE6,  // request queue is full
  MEMORY_ALLOCATION_FAILED = 0xE7,  // memory allocation failed
  INVALID_RESPONSE      = 0xE8   // response validation failed
};

typedef std::function<void(uint16_t, uint8_t, esp32Modbus::FunctionCode, uint8_t*, uint16_t)> MBTCPOnData;
typedef std::function<void(uint8_t, esp32Modbus::FunctionCode, uint16_t, uint8_t*, uint16_t)> MBRTUOnData;
typedef std::function<void(uint16_t, esp32Modbus::Error)> MBTCPOnError;
typedef std::function<void(esp32Modbus::Error)> MBRTUOnError;

// Helper function to get error description
inline const char* getErrorDescription(Error error) {
  switch (error) {
    case SUCCESS: return "Success";
    case ILLEGAL_FUNCTION: return "Illegal function";
    case ILLEGAL_DATA_ADDRESS: return "Illegal data address";
    case ILLEGAL_DATA_VALUE: return "Illegal data value";
    case SERVER_DEVICE_FAILURE: return "Server device failure";
    case ACKNOWLEDGE: return "Acknowledge";
    case SERVER_DEVICE_BUSY: return "Server device busy";
    case NEGATIVE_ACKNOWLEDGE: return "Negative acknowledge";
    case MEMORY_PARITY_ERROR: return "Memory parity error";
    case TIMEOUT: return "Timeout";
    case INVALID_SLAVE: return "Invalid slave address";
    case INVALID_FUNCTION: return "Invalid function";
    case CRC_ERROR: return "CRC error";
    case COMM_ERROR: return "Communication error";
    case INVALID_PARAMETER: return "Invalid parameter";
    case QUEUE_FULL: return "Request queue full";
    case MEMORY_ALLOCATION_FAILED: return "Memory allocation failed";
    case INVALID_RESPONSE: return "Invalid response";
    default: return "Unknown error";
  }
}

/**
 * @brief Priority levels for Modbus requests
 *
 * Lower numeric values = higher priority (processed first)
 *
 * Queue configuration:
 * - EMERGENCY: 4 slots  (emergency shutdown, failsafe)
 * - SENSOR:    8 slots  (temperature/pressure sensor reads)
 * - RELAY:     12 slots (relay commands, mode switches)
 * - STATUS:    4 slots  (verification, diagnostics)
 */
enum ModbusPriority : uint8_t {
  EMERGENCY = 0,  ///< Highest priority - emergency shutdown, failsafe
  SENSOR = 1,     ///< High priority - sensor reads (safety-critical)
  RELAY = 2,      ///< Normal priority - relay commands
  STATUS = 3      ///< Low priority - status/diagnostic reads
};

// Helper function to get priority description
inline const char* getPriorityDescription(ModbusPriority priority) {
  switch (priority) {
    case EMERGENCY: return "EMERGENCY";
    case SENSOR:    return "SENSOR";
    case RELAY:     return "RELAY";
    case STATUS:    return "STATUS";
    default:        return "UNKNOWN";
  }
}

}  // namespace esp32Modbus

#endif
