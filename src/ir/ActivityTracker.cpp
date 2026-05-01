// Copyright 2026 insomniaTV Contributors. All rights reserved.

#include "ActivityTracker.h"

#include <chrono>
#include <string>

namespace InsomniaTV {

static uint32_t get_millis() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

ActivityTracker::ActivityTracker(uint32_t inactivity_threshold_ms)
    : _inactivity_threshold_ms(inactivity_threshold_ms),
      _last_activity_ts(get_millis()) {}

bool ActivityTracker::record(const std::string& protocol, uint64_t code,
                             uint16_t bits) {
  _last_activity_ts = get_millis();
  return true;
}

uint32_t ActivityTracker::msSinceLastActivity() const {
  return get_millis() - _last_activity_ts;
}

void ActivityTracker::reset() {
  _last_activity_ts = get_millis();
  _recent_pulses.clear();
}

}  // namespace InsomniaTV
