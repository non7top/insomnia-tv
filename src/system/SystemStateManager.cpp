// Copyright 2026 insomniaTV Contributors. All rights reserved.

#include "SystemStateManager.h"

#include "EventBus.h"

namespace InsomniaTV {

SystemStateManager::SystemStateManager()
    : _state({TvPower::ON, Presence::PRESENT, Activity::HIGH}) {
    EventBus::instance().subscribe([this](SystemEvent event) {
        this->handleEvent(event);
    });
}


const SystemState& SystemStateManager::getState() const {
  return _state;
}

void SystemStateManager::handleEvent(SystemEvent event) {
    switch (event) {
        case SystemEvent::IR_ACTIVITY:
            _state.activity = Activity::HIGH;
            break;
        case SystemEvent::SYSTEM_IDLE:
            _state.activity = Activity::IDLE;
            break;
        case SystemEvent::TV_PING_SUCCESS:
            _state.tvPower = TvPower::ON;
            break;
        case SystemEvent::TV_PING_FAIL:
            _state.tvPower = TvPower::OFF;
            break;
        case SystemEvent::CURRENT_UPDATE:
            // Logic for future use
            break;
        default:
            break;
    }
}

}  // namespace InsomniaTV
