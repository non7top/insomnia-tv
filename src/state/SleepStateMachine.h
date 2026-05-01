// Copyright 2026 insomniaTV Contributors. All rights reserved.

#ifndef SRC_STATE_SLEEPSTATEMACHINE_H_
#define SRC_STATE_SLEEPSTATEMACHINE_H_

#include <cstdint>
#include <string>

namespace InsomniaTV {

enum class State { MONITORING, RAMPING, VERIFYING, POWERING_OFF, FALLBACK_OFF };

/**
 * @brief Manages the TV sleep state machine.
 */
class SleepStateMachine {
public:
  SleepStateMachine();

  void tick();
  void onIrActivity();
  State getCurrentState() const;

private:
  State _current_state;
};

}  // namespace InsomniaTV

#endif  // SRC_STATE_SLEEPSTATEMACHINE_H_
