// Copyright 2026 insomniaTV Contributors. All rights reserved.

#ifndef SRC_NETWORK_NETWORKCHECKER_H_
#define SRC_NETWORK_NETWORKCHECKER_H_

#include <string>
#include "../hal/INetworkChecker.h"

namespace InsomniaTV {

/**
 * @brief Concrete implementation of INetworkChecker for ESP32.
 *
 * Provides network diagnostic capabilities such as ICMP ping to check the
 * reachability of the TV and other network-connected devices.
 */
class NetworkChecker : public INetworkChecker {
public:
    /**
     * @brief Constructs the NetworkChecker.
     */
    NetworkChecker() = default;
    ~NetworkChecker() override = default;

    /**
     * @brief Pings the target IP address.
     * @param ip The target IP address as a string.
     * @return RTT in ms if successful, -1 otherwise.
     */
    int32_t ping(const std::string& ip) override;

    /**
     * @brief Performs an HTTP GET request to the given URL.
     * @param url The target URL.
     * @return HTTP status code if successful, -1 otherwise.
     */
    int32_t httpGet(const std::string& url) override;

    /**
     * @brief Sets the timeout for network operations.
     * @param timeoutMs Timeout duration in milliseconds.
     */
    void setTimeout(uint32_t timeoutMs) override;

    /**
     * @brief Checks if the WiFi interface is currently connected.
     */
    bool isConnected() const override;

private:
    uint32_t _timeoutMs = 1000;
};

}  // namespace InsomniaTV

#endif  // SRC_NETWORK_NETWORKCHECKER_H_
