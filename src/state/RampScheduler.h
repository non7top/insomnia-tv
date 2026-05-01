// Copyright 2026 insomniaTV Contributors. All rights reserved.

#ifndef SRC_STATE_RAMPSCHEDULER_H_
#define SRC_STATE_RAMPSCHEDULER_H_

#include <cstdint>
#include <functional>

namespace InsomniaTV {

/**
 * @brief Schedules periodic IR commands for volume ramping.
 */
class RampScheduler {
public:
    RampScheduler(uint32_t interval_ms, std::function<void()> on_ramp_step);

    void start();
    void stop();

private:
    uint32_t _interval_ms;
    std::function<void()> _on_ramp_step;
};

}  // namespace InsomniaTV

#endif  // SRC_STATE_RAMPSCHEDULER_H_
