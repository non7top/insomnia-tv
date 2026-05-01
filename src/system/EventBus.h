// Copyright 2026 insomniaTV Contributors. All rights reserved.

#ifndef SRC_SYSTEM_EVENTBUS_H_
#define SRC_SYSTEM_EVENTBUS_H_

#include <functional>
#include <vector>

#include "SystemEvents.h"

namespace InsomniaTV {

using EventCallback = std::function<void(SystemEvent)>;

class EventBus {
public:
  static EventBus& instance();

  void subscribe(EventCallback callback);
  void publish(SystemEvent event);

private:
  EventBus() = default;
  std::vector<EventCallback> _subscribers;
};

}  // namespace InsomniaTV

#endif  // SRC_SYSTEM_EVENTBUS_H_
