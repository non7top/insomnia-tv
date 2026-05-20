// Copyright 2026 insomniaTV Contributors. All rights reserved.

#include "SleepStateMachine.h"

#include "../tv/TvStateMachine.h"

namespace InsomniaTV {

SleepStateMachine::SleepStateMachine(IClock& clock,
                                     uint32_t inactivityTimeoutMs)
    : clock_(clock),
      inactivityTimeoutMs_(inactivityTimeoutMs),
      lastActivityMs_(clock.nowMs()),
      current_(State::MONITORING),
      tvStateMachine_(nullptr) {}

void SleepStateMachine::tick() {
  switch (current_) {
    case State::MONITORING:
      if (clock_.nowMs() - lastActivityMs_ >= inactivityTimeoutMs_) {
        current_ = State::RAMPING;
      }
      break;

    case State::RAMPING:
      // Transition driven by onRampComplete(); nothing to do here.
      break;

    case State::VERIFYING: {
      State next = State::POWERING_OFF;
      if (tvStateMachine_) {
        auto ps = tvStateMachine_->getPowerState();
        if (ps == TvStateMachine::PowerState::OFF) {
          // TV already off — nothing to do.
          resetToMonitoring();
          return;
        } else if (ps == TvStateMachine::PowerState::UNKNOWN) {
          next = State::FALLBACK_OFF;
        }
        // ON or TRANSITIONING → proceed to POWERING_OFF
      }
      current_ = next;
      // Fall through to execute the power-off action immediately.
      if (powerOffCallback_) powerOffCallback_();
      resetToMonitoring();
      break;
    }

    case State::POWERING_OFF:
      if (powerOffCallback_) powerOffCallback_();
      resetToMonitoring();
      break;

    case State::FALLBACK_OFF:
      if (powerOffCallback_) powerOffCallback_();
      resetToMonitoring();
      break;
  }
}

void SleepStateMachine::onIrActivity() {
  resetToMonitoring();
}

void SleepStateMachine::onRampComplete() {
  if (current_ == State::RAMPING) {
    current_ = State::VERIFYING;
  }
}

State SleepStateMachine::getCurrentState() const {
  return current_;
}

void SleepStateMachine::setTvStateMachine(TvStateMachine* tv) {
  tvStateMachine_ = tv;
}

void SleepStateMachine::setPowerOffCallback(std::function<void()> callback) {
  powerOffCallback_ = callback;
}

void SleepStateMachine::resetToMonitoring() {
  current_ = State::MONITORING;
  lastActivityMs_ = clock_.nowMs();
}

}  // namespace InsomniaTV
