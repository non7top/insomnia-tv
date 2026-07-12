// Copyright 2026 insomniaTV Contributors. All rights reserved.

#include "UpdateCompletion.h"

namespace InsomniaTV {

namespace {
// Empirically enough for ESPAsyncWebServer to flush the response before the
// connection is torn down by restart() (#65).
constexpr uint32_t kResponseFlushDelayMs = 1000;
}  // namespace

void handleUpdateCompletion(bool updateOk, IRebootSequencer& sequencer) {
  sequencer.sendResponse(updateOk);
  if (updateOk) {
    sequencer.delayMs(kResponseFlushDelayMs);
    sequencer.restart();
  }
}

}  // namespace InsomniaTV
