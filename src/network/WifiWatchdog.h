// Copyright 2026 insomniaTV Contributors. All rights reserved.

#ifndef SRC_NETWORK_WIFIWATCHDOG_H_
#define SRC_NETWORK_WIFIWATCHDOG_H_

#include <cstdint>

namespace InsomniaTV {

// Decides when a device that thinks it is on WiFi should rejoin or restart.
// The caller checks reachability (gateway ping) on a fixed period and feeds
// each result to onCheck(); this class only counts.
//
// A router that never answers ICMP must not trigger restarts, so failures are
// ignored until the gateway has answered at least once since boot.
class WifiWatchdog {
public:
  enum class Action { None, Rejoin, Restart };

  // Rejoin after `rejoinAfter` consecutive failed checks; restart once
  // `restartAfterRejoins` rejoins in a row have not restored connectivity.
  explicit WifiWatchdog(uint8_t rejoinAfter = 4,
                        uint8_t restartAfterRejoins = 3);

  Action onCheck(bool reachable);

  uint8_t consecutiveFailures() const { return failures_; }
  bool everReachable() const { return everReachable_; }

private:
  uint8_t rejoinAfter_;
  uint8_t restartAfterRejoins_;
  uint8_t failures_;
  uint8_t rejoins_;
  bool everReachable_;
};

}  // namespace InsomniaTV

#endif  // SRC_NETWORK_WIFIWATCHDOG_H_
