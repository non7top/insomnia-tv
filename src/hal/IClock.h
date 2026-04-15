// Copyright 2026 insomniaTV Contributors. All rights reserved.

#ifndef SRC_HAL_ICLOCK_H_
#define SRC_HAL_ICLOCK_H_

#include <stdint.h>

namespace InsomniaTV {

// Abstract interface for time source.
// Wraps millis()/micros() on ESP32; allows time injection for tests.
class IClock {
public:
  virtual ~IClock() = default;

  // Current time in milliseconds since boot
  virtual uint32_t nowMs() const = 0;

  // Advance simulated time (no-op in production, used by mocks)
  virtual void advanceMs(uint32_t ms) = 0;
};

}  // namespace InsomniaTV

#endif  // SRC_HAL_ICLOCK_H_
