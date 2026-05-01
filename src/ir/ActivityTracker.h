// Copyright 2026 insomniaTV Contributors. All rights reserved.

#ifndef SRC_IR_ACTIVITYTRACKER_H_
#define SRC_IR_ACTIVITYTRACKER_H_

#include <cstdint>
#include <vector>
#include <string>

namespace InsomniaTV {

struct IrPulse {
    std::string protocol;
    uint64_t code;
    uint16_t bits;
    uint32_t timestamp;
};

class ActivityTracker {
public:
    ActivityTracker(uint32_t inactivity_threshold_ms);

    // Records a new IR activity, returns true if similarity group threshold exceeded
    bool record(const std::string& protocol, uint64_t code, uint16_t bits);

    // Returns time since last recorded activity
    uint32_t msSinceLastActivity() const;

    // Resets tracker state
    void reset();

private:
    uint32_t _inactivity_threshold_ms;
    uint32_t _last_activity_ts;
    std::vector<IrPulse> _recent_pulses;
};

}  // namespace InsomniaTV

#endif  // SRC_IR_ACTIVITYTRACKER_H_
