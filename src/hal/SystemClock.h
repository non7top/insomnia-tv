// Copyright 2026 insomniaTV Contributors. All rights reserved.

#ifndef SRC_HAL_SYSTEMCLOCK_H_
#define SRC_HAL_SYSTEMCLOCK_H_

#if defined(ARDUINO)
#include <Arduino.h>
#endif

#include "IClock.h"

namespace InsomniaTV {

// Production IClock backed by the Arduino millis() counter.
class SystemClock : public IClock {
public:
  uint32_t nowMs() const override {
#if defined(ARDUINO)
    return millis();
#else
    return 0;
#endif
  }

  // No-op: wall time cannot be manually advanced in production.
  void advanceMs(uint32_t) override {}
};

}  // namespace InsomniaTV

#endif  // SRC_HAL_SYSTEMCLOCK_H_
