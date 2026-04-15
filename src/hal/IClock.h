#ifndef INSOMNIATV_IClock_H
#define INSOMNIATV_IClock_H

#include <stdint.h>

namespace InsomniaTV {

/**
 * Abstract interface for time source.
 * Wraps millis()/micros() on ESP32; allows time injection for native tests.
 */
class IClock {
public:
    virtual ~IClock() = default;

    /** Current time in milliseconds since boot */
    virtual uint32_t nowMs() const = 0;

    /** Advance simulated time (no-op in production, used by test mocks) */
    virtual void advanceMs(uint32_t ms) = 0;
};

} // namespace InsomniaTV

#endif // INSOMNIATV_IClock_H
