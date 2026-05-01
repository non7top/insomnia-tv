// Copyright 2026 insomniaTV Contributors. All rights reserved.

#ifndef SRC_SYSTEM_SYSTEMSTATEMANAGER_H_
#define SRC_SYSTEM_SYSTEMSTATEMANAGER_H_

#include <cstdint>

#include "SystemEvents.h"

namespace InsomniaTV {

enum class TvPower { ON, OFF, VERIFYING };
enum class Presence { PRESENT, ABSENT };
enum class Activity { IDLE, LOW, HIGH };

struct SystemState {
  TvPower tvPower;
  Presence presence;
  Activity activity;
};

class SystemStateManager {
public:
  SystemStateManager();

  const SystemState& getState() const;

private:
  void handleEvent(SystemEvent event);

  SystemState _state;
};

}  // namespace InsomniaTV

#endif  // SRC_SYSTEM_SYSTEMSTATEMANAGER_H_
