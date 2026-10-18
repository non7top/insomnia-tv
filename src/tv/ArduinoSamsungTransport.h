// Copyright 2026 insomniaTV Contributors. All rights reserved.

#ifndef SRC_TV_ARDUINOSAMSUNGTRANSPORT_H_
#define SRC_TV_ARDUINOSAMSUNGTRANSPORT_H_

#include <string>

#include "SamsungTvController.h"

namespace InsomniaTV {

// ESP32 implementation of ISamsungTvTransport (HTTPClient + WebSockets lib).
// Host-only builds get no definition; native tests use a mock instead.
class ArduinoSamsungTransport : public ISamsungTvTransport {
public:
  std::string fetchDeviceInfo(const std::string& host) override;
  SamsungWsResult sendKey(const std::string& host, const std::string& token,
                          const std::string& key, uint32_t timeoutMs) override;
};

}  // namespace InsomniaTV

#endif  // SRC_TV_ARDUINOSAMSUNGTRANSPORT_H_
