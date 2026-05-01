// Copyright 2026 insomniaTV Contributors. All rights reserved.

#ifndef SRC_SYSTEM_SYSTEMEVENTS_H_
#define SRC_SYSTEM_SYSTEMEVENTS_H_

namespace InsomniaTV {

enum class SystemEvent {
    PIR_ACTIVE,
    IR_ACTIVITY,
    TV_PING_SUCCESS,
    TV_PING_FAIL,
    SYSTEM_IDLE,
    SYSTEM_ACTIVE,
    CURRENT_UPDATE
};
}  // namespace InsomniaTV

#endif  // SRC_SYSTEM_SYSTEMEVENTS_H_
