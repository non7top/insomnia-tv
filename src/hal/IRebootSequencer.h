// Copyright 2026 insomniaTV Contributors. All rights reserved.

#ifndef SRC_HAL_IREBOOTSEQUENCER_H_
#define SRC_HAL_IREBOOTSEQUENCER_H_

#include <cstdint>

namespace InsomniaTV {

/**
 * @brief Abstracts the response-then-reboot tail of an OTA update, so the
 * ordering (response flushed before the device restarts) can be unit
 * tested without a real ESPAsyncWebServer/ESP.restart().
 */
class IRebootSequencer {
public:
  virtual ~IRebootSequencer() = default;

  // Sends the update-complete response to the client.
  virtual void sendResponse(bool ok) = 0;

  // Blocks for ms milliseconds (gives the response time to flush).
  virtual void delayMs(uint32_t ms) = 0;

  // Restarts the device.
  virtual void restart() = 0;
};

}  // namespace InsomniaTV

#endif  // SRC_HAL_IREBOOTSEQUENCER_H_
