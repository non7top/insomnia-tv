// Copyright 2026 insomniaTV Contributors. All rights reserved.

#include "RampScheduler.h"

namespace InsomniaTV {

RampScheduler::RampScheduler(IClock& clock, uint32_t interval_ms,
                             uint8_t max_steps,
                             std::function<void()> on_ramp_step,
                             std::function<void()> on_complete)
    : clock_(clock),
      interval_ms_(interval_ms),
      max_steps_(max_steps),
      on_ramp_step_(on_ramp_step),
      on_complete_(on_complete),
      active_(false),
      step_count_(0),
      last_step_ms_(0) {}

void RampScheduler::start() {
  active_ = true;
  step_count_ = 0;
  last_step_ms_ = clock_.nowMs();
}

void RampScheduler::stop() {
  active_ = false;
}

void RampScheduler::tick() {
  if (!active_)
    return;

  if (clock_.nowMs() - last_step_ms_ < interval_ms_)
    return;

  last_step_ms_ = clock_.nowMs();
  if (on_ramp_step_)
    on_ramp_step_();
  step_count_++;

  if (step_count_ >= max_steps_) {
    active_ = false;
    if (on_complete_)
      on_complete_();
  }
}

bool RampScheduler::isActive() const {
  return active_;
}

uint8_t RampScheduler::stepCount() const {
  return step_count_;
}

}  // namespace InsomniaTV
