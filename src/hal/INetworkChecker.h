// Copyright 2026 insomniaTV Contributors. All rights reserved.

#ifndef SRC_HAL_INETWORKCHECKER_H_
#define SRC_HAL_INETWORKCHECKER_H_

#include <Arduino.h>
#include <stdint.h>

namespace InsomniaTV {

// Abstract interface for network connectivity checks.
// Used by TvVerifier to determine if the TV is still powered on.
class INetworkChecker {
public:
  virtual ~INetworkChecker() = default;

  // ICMP ping to target IP, returns RTT in ms or -1 on failure
  virtual int32_t ping(const String& ip) = 0;

  // HTTP GET request, returns status code or -1 on failure
  virtual int32_t httpGet(const String& url) = 0;

  // Set timeout for all network operations in milliseconds
  virtual void setTimeout(uint32_t timeoutMs) = 0;

  // Check if WiFi is connected
  virtual bool isConnected() const = 0;
};

}  // namespace InsomniaTV

#endif  // SRC_HAL_INETWORKCHECKER_H_
