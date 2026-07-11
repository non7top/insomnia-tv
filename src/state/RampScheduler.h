// Copyright 2026 insomniaTV Contributors. All rights reserved.

#ifndef SRC_STATE_RAMPSCHEDULER_H_
#define SRC_STATE_RAMPSCHEDULER_H_

#include <cstdint>
#include <functional>

#include "../hal/IClock.h"

namespace InsomniaTV {

/**
 * @brief Drives periodic ramp steps (e.g. IR volume-down commands) leading
 *        up to a power-off attempt.
 *
 * Externally clocked via tick(), matching the pattern used by
 * SleepStateMachine/TvStateMachine — avoids needing a platform-specific
 * timer to be natively testable.
 */
class RampScheduler {
public:
  RampScheduler(IClock& clock, uint32_t interval_ms, uint8_t max_steps,
               std::function<void()> on_ramp_step,
               std::function<void()> on_complete);

  // Begins the ramp sequence: resets step count, marks active.
  void start();

  // Cancels the ramp sequence (e.g. activity detected mid-ramp).
  void stop();

  // Call every loop iteration. Fires on_ramp_step() every interval_ms while
  // active; fires on_complete() and stops once max_steps is reached.
  void tick();

  bool isActive() const;
  uint8_t stepCount() const;

private:
  IClock& clock_;
  uint32_t interval_ms_;
  uint8_t max_steps_;
  std::function<void()> on_ramp_step_;
  std::function<void()> on_complete_;

  bool active_;
  uint8_t step_count_;
  uint32_t last_step_ms_;
};

}  // namespace InsomniaTV

#endif  // SRC_STATE_RAMPSCHEDULER_H_
