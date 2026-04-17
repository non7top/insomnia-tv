// Copyright 2026 insomniaTV Contributors. All rights reserved.

#ifndef SRC_HAL_IIRDRIVER_H_
#define SRC_HAL_IIRDRIVER_H_

#include <Arduino.h>
#include <stdint.h>

namespace InsomniaTV {

// Abstract interface for IR transmit/receive hardware.
// Implementations wrap IRremoteESP8266 for ESP32 or mock for native tests.
class IIrDriver {
public:
  virtual ~IIrDriver() = default;

  // Initialize IR TX/RX pins and protocols
  virtual void begin() = 0;

  // Send an IR code with the given protocol
  virtual bool send(const String& protocol, uint64_t code, uint16_t bits) = 0;

  // Start non-blocking receive mode (callback on decoded pulse)
  virtual void receive() = 0;

  // Capture raw IR pulse for learning (returns microseconds array)
  virtual uint16_t* learn_raw(uint16_t& out_len) = 0;

  // Check if a decoded IR code is available
  virtual bool hasDecoded() const = 0;

  // Get the last decoded protocol string
  virtual String lastProtocol() const = 0;

  // Get the last decoded code value
  virtual uint64_t lastCode() const = 0;

  // Get the bit length of the last decoded code
  virtual uint16_t lastBits() const = 0;
};

}  // namespace InsomniaTV

#endif  // SRC_HAL_IIRDRIVER_H_
