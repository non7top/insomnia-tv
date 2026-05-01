// Copyright 2026 insomniaTV Contributors. All rights reserved.

#include "RampScheduler.h"

namespace InsomniaTV {

RampScheduler::RampScheduler(uint32_t interval_ms,
                             std::function<void()> on_ramp_step)
    : _interval_ms(interval_ms), _on_ramp_step(on_ramp_step) {}

void RampScheduler::start() {
  // Start FreeRTOS timer
}

void RampScheduler::stop() {
  // Stop FreeRTOS timer
}

}  // namespace InsomniaTV
