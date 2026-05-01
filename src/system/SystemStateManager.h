// Copyright 2026 insomniaTV Contributors. All rights reserved.

#ifndef SRC_SYSTEM_SYSTEMSTATEMANAGER_H_
#define SRC_SYSTEM_SYSTEMSTATEMANAGER_H_

#include "SystemEvents.h"
#include <cstdint>

namespace InsomniaTV {

enum class TvPower { ON, OFF, VERIFYING };
enum class Presence { PRESENT, ABSENT };
enum class Activity { IDLE, A_LOW, HIGH };

/**
 * @brief Represents the centralized state of the insomniaTV system.
 */
struct SystemState {
    TvPower tvPower;
    Presence presence;
    Activity activity;
    float currentAmperes;
};

/**
 * @brief Manages the centralized system state by subscribing to events.
 */
class SystemStateManager {
public:
    /**
     * @brief Initializes the state manager and subscribes to the EventBus.
     */
    SystemStateManager();

    /**
     * @brief Gets the current system state.
     */
    const SystemState& getState() const;

private:
    /**
     * @brief Processes system events to update the state.
     */
    void handleEvent(SystemEvent event);

    SystemState _state;
};

}  // namespace InsomniaTV

#endif  // SRC_SYSTEM_SYSTEMSTATEMANAGER_H_
