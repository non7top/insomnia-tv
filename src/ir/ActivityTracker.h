// Copyright 2026 insomniaTV Contributors. All rights reserved.

#ifndef SRC_IR_ACTIVITYTRACKER_H_
#define SRC_IR_ACTIVITYTRACKER_H_

#include <cstdint>
#include <vector>
#include <string>

namespace InsomniaTV {

/**
 * @brief Represents an individual decoded IR pulse.
 */
struct IrPulse {
    std::string protocol;
    uint64_t code;
    uint16_t bits;
    uint32_t timestamp;
};

/**
 * @brief Monitors IR activity and tracks inactivity periods.
 *
 * Uses a sliding window to keep track of recent IR pulses and maintains
 * the timestamp of the last detected activity to trigger state machine
 * transitions.
 */
class ActivityTracker {
public:
    /**
     * @brief Constructs the tracker with a custom inactivity threshold.
     * @param inactivity_threshold_ms Time in milliseconds before considering
     *                                the system idle.
     */
    explicit ActivityTracker(uint32_t inactivity_threshold_ms);

    /**
     * @brief Records a new IR activity.
     * @param protocol The IR protocol name (e.g., "NEC").
     * @param code The decoded IR code.
     * @param bits The number of bits in the code.
     * @return true if the activity indicates a repeat pattern or high frequency
     *         that exceeds similarity thresholds (reserved for future use).
     */
    bool record(const std::string& protocol, uint64_t code, uint16_t bits);

    /**
     * @brief Calculates time elapsed since the last IR pulse.
     * @return Milliseconds since the last recorded IR activity.
     */
    uint32_t msSinceLastActivity() const;

    /**
     * @brief Clears pulse history and resets the last activity timestamp.
     */
    void reset();

private:
    uint32_t _inactivity_threshold_ms;
    uint32_t _last_activity_ts;
    std::vector<IrPulse> _recent_pulses;
};

}  // namespace InsomniaTV

#endif  // SRC_IR_ACTIVITYTRACKER_H_
