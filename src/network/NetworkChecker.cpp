// Copyright 2026 insomniaTV Contributors. All rights reserved.

#include "NetworkChecker.h"

#include <ESP32Ping.h>
#include <WiFi.h>

namespace InsomniaTV {

int32_t NetworkChecker::ping(const std::string& ip) {
  if (Ping.ping(ip.c_str(), 1)) {
    return Ping.averageTime();
  }
  return -1;
}

int32_t NetworkChecker::httpGet(const std::string& url) {
  // Implementation using HTTPClient would go here
  return -1;
}

void NetworkChecker::setTimeout(uint32_t timeoutMs) {
  _timeoutMs = timeoutMs;
}

bool NetworkChecker::isConnected() const {
  return WiFi.status() == WL_CONNECTED;
}

}  // namespace InsomniaTV
