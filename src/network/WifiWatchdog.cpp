// Copyright 2026 insomniaTV Contributors. All rights reserved.

#include "WifiWatchdog.h"

namespace InsomniaTV {

WifiWatchdog::WifiWatchdog(uint8_t rejoinAfter, uint8_t restartAfterRejoins)
    : rejoinAfter_(rejoinAfter),
      restartAfterRejoins_(restartAfterRejoins),
      failures_(0),
      rejoins_(0),
      everReachable_(false) {}

WifiWatchdog::Action WifiWatchdog::onCheck(bool reachable) {
  if (reachable) {
    everReachable_ = true;
    failures_ = 0;
    rejoins_ = 0;
    return Action::None;
  }
  if (!everReachable_) {
    return Action::None;
  }
  if (++failures_ < rejoinAfter_) {
    return Action::None;
  }
  failures_ = 0;
  if (++rejoins_ > restartAfterRejoins_) {
    return Action::Restart;
  }
  return Action::Rejoin;
}

}  // namespace InsomniaTV
