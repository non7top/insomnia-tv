// Copyright 2026 insomniaTV Contributors. All rights reserved.

#ifndef SRC_NETWORK_NETWORKCHECKER_H_
#define SRC_NETWORK_NETWORKCHECKER_H_

#include "../hal/INetworkChecker.h"

namespace InsomniaTV {

/**
 * @brief Concrete implementation of INetworkChecker for ESP32.
 */
class NetworkChecker : public INetworkChecker {
public:
  NetworkChecker() = default;
  ~NetworkChecker() override = default;

  int32_t ping(const std::string& ip) override;
  int32_t httpGet(const std::string& url) override;
  void setTimeout(uint32_t timeoutMs) override;
  bool isConnected() const override;

private:
  uint32_t _timeoutMs = 1000;
};

}  // namespace InsomniaTV

#endif  // SRC_NETWORK_NETWORKCHECKER_H_
