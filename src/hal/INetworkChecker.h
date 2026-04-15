#ifndef INSOMNIATV_INetworkChecker_H
#define INSOMNIATV_INetworkChecker_H

#include <stdint.h>
#include <Arduino.h>

namespace InsomniaTV {

/**
 * Abstract interface for network connectivity checks (ping/HTTP).
 * Used by TvVerifier to determine if the TV is still powered on.
 */
class INetworkChecker {
public:
    virtual ~INetworkChecker() = default;

    /** ICMP ping to target IP, returns round-trip time in ms or -1 on failure */
    virtual int32_t ping(const String& ip) = 0;

    /** HTTP GET request, returns status code or -1 on failure */
    virtual int32_t httpGet(const String& url) = 0;

    /** Set timeout for all network operations in milliseconds */
    virtual void setTimeout(uint32_t timeoutMs) = 0;

    /** Check if WiFi is connected */
    virtual bool isConnected() const = 0;
};

} // namespace InsomniaTV

#endif // INSOMNIATV_INetworkChecker_H
