// Copyright 2026 insomniaTV Contributors. All rights reserved.

#ifndef SRC_STATE_SLEEPSTATEMACHINE_H_
#define SRC_STATE_SLEEPSTATEMACHINE_H_

#include <cstdint>
#include <functional>

#include "../hal/IClock.h"

namespace InsomniaTV {

class TvStateMachine;  // forward declaration

enum class State { MONITORING, RAMPING, VERIFYING, POWERING_OFF, FALLBACK_OFF };

/**
 * @brief Drives the sleep sequence: monitor IR → ramp volume → verify TV
 *        state → send power-off.
 *
 * State transitions:
 *   MONITORING  ──(inactivity timeout)──► RAMPING
 *   RAMPING     ──(onRampComplete)──────► VERIFYING
 *   VERIFYING   ──(TV OFF)─────────────► MONITORING  (already off, done)
 *               ──(TV ON or no SM)──────► POWERING_OFF
 *               ──(TV UNKNOWN)──────────► FALLBACK_OFF
 *   POWERING_OFF / FALLBACK_OFF  ───────► MONITORING  (fires power callback)
 *
 *   Any state  ──(onIrActivity)─────────► MONITORING
 */
class SleepStateMachine {
public:
  explicit SleepStateMachine(IClock& clock,
                             uint32_t inactivityTimeoutMs = 30000);

  void tick();
  void onIrActivity();
  void onRampComplete();

  State getCurrentState() const;

  // Optional: supply TvStateMachine for VERIFYING state decisions.
  void setTvStateMachine(TvStateMachine* tv);

  // Callback invoked when a power-off command should be sent.
  void setPowerOffCallback(std::function<void()> callback);

  // Callback invoked on entering RAMPING (starts the ramp sequence).
  void setRampStartCallback(std::function<void()> callback);

private:
  void resetToMonitoring();

  IClock& clock_;
  uint32_t inactivityTimeoutMs_;
  uint32_t lastActivityMs_;

  State current_;
  TvStateMachine* tvStateMachine_;
  std::function<void()> powerOffCallback_;
  std::function<void()> rampStartCallback_;
};

}  // namespace InsomniaTV

#endif  // SRC_STATE_SLEEPSTATEMACHINE_H_
