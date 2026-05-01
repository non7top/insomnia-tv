// Copyright 2026 insomniaTV Contributors. All rights reserved.

#include "SleepStateMachine.h"

namespace InsomniaTV {

SleepStateMachine::SleepStateMachine() : _current_state(State::MONITORING) {}

void SleepStateMachine::tick() {
    // State machine logic
}

void SleepStateMachine::onIrActivity() {
    _current_state = State::MONITORING;
}

State SleepStateMachine::getCurrentState() const {
    return _current_state;
}

}  // namespace InsomniaTV
