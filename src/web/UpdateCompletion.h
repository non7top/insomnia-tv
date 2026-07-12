// Copyright 2026 insomniaTV Contributors. All rights reserved.

#ifndef SRC_WEB_UPDATECOMPLETION_H_
#define SRC_WEB_UPDATECOMPLETION_H_

#include "../hal/IRebootSequencer.h"

namespace InsomniaTV {

/**
 * @brief The /update handler's completion logic, extracted so it's unit
 * testable without a real AsyncWebServerRequest/Update/ESP.restart().
 *
 * On success: send OK, wait for the response to actually flush, then
 * restart. On failure: send FAIL, don't restart.
 */
void handleUpdateCompletion(bool updateOk, IRebootSequencer& sequencer);

}  // namespace InsomniaTV

#endif  // SRC_WEB_UPDATECOMPLETION_H_
