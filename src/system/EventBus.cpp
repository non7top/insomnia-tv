// Copyright 2026 insomniaTV Contributors. All rights reserved.

#include "EventBus.h"

namespace InsomniaTV {

EventBus& EventBus::instance() {
  static EventBus instance;
  return instance;
}

void EventBus::subscribe(EventCallback callback) {
  _subscribers.push_back(callback);
}

void EventBus::publish(SystemEvent event) {
  for (auto& callback : _subscribers) {
    callback(event);
  }
}

void EventBus::clear() {
  _subscribers.clear();
}

}  // namespace InsomniaTV
