// Copyright 2026 insomniaTV Contributors. All rights reserved.

#ifndef SRC_HAL_IHALLSENSOR_H_
#define SRC_HAL_IHALLSENSOR_H_

#include <stdint.h>

namespace InsomniaTV {

/**
 * @brief Interface for AC current Hall sensors.
 */
class IHallSensor {
public:
    virtual ~IHallSensor() = default;

    /**
     * @brief Gets the current reading in Amperes.
     * @return Current in Amperes, or -1.0 on failure.
     */
    virtual float getCurrentAmperes() = 0;

    /**
     * @brief Calibrates the sensor zero-point.
     */
    virtual void calibrate() = 0;
};

}  // namespace InsomniaTV

#endif  // SRC_HAL_IHALLSENSOR_H_
