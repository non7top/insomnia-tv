// Copyright 2026 insomniaTV Contributors. All rights reserved.

#ifndef SRC_NETWORK_WIFISETUP_H_
#define SRC_NETWORK_WIFISETUP_H_

#include <stdint.h>

#include <atomic>
#include <string>

namespace InsomniaTV {

/**
 * @brief Manages WiFi configuration and initial setup portal.
 */
class WifiSetup {
public:
  WifiSetup();

  /**
   * @brief Initializes WiFi connection or starts AP portal if not configured.
   */
  void begin();

  /**
   * @brief Checks if reset button is held for 10 seconds and resets WiFi if so.
   */
  void handleResetButton();

  /**
   * @brief True while the WiFi config (AP) portal is active, awaiting setup.
   *        Safe to poll from another task.
   */
  bool isPortalActive() const;

private:
  std::string getChipId();
  uint32_t _buttonPressStart;
  bool _isButtonPressed;
  std::atomic<bool> _portalActive{false};
};

}  // namespace InsomniaTV

#endif  // SRC_NETWORK_WIFISETUP_H_
