// Copyright 2026 insomniaTV Contributors. All rights reserved.

#ifndef SRC_SYSTEM_EVENTBUS_H_
#define SRC_SYSTEM_EVENTBUS_H_

#include <functional>
#include <vector>

#include "SystemEvents.h"

namespace InsomniaTV {

/**
 * @brief Callback type for system events.
 */
using EventCallback = std::function<void(SystemEvent)>;

/**
 * @brief A simple, synchronous observer-based event bus for system
 * communication.
 *
 * Decouples system components by allowing them to publish and subscribe
 * to system-wide events without direct dependencies.
 */
class EventBus {
public:
  /**
   * @brief Gets the singleton instance of the EventBus.
   */
  static EventBus& instance();

  /**
   * @brief Subscribes a callback to all system events.
   * @param callback The function to execute on event.
   */
  void subscribe(EventCallback callback);

  /**
   * @brief Publishes an event to all subscribers.
   * @param event The event to broadcast.
   */
  void publish(SystemEvent event);

  /**
   * @brief Clears all subscribers (for testing purposes).
   */
  void clear();

private:
  EventBus() = default;
  std::vector<EventCallback> _subscribers;
};

}  // namespace InsomniaTV

#endif  // SRC_SYSTEM_EVENTBUS_H_
