// Copyright 2026 insomniaTV Contributors. All rights reserved.

#ifndef SRC_HAL_HALLSENSOR_H_
#define SRC_HAL_HALLSENSOR_H_

#include "IHallSensor.h"

namespace InsomniaTV {

/**
 * @brief Concrete implementation for ACS712 or similar Hall sensors.
 */
class HallSensor : public IHallSensor {
public:
    explicit HallSensor(uint8_t pin);
    ~HallSensor() override = default;

    float getCurrentAmperes() override;
    void calibrate() override;

private:
    uint8_t _pin;
    float _zeroPoint;
};

}  // namespace InsomniaTV

#endif  // SRC_HAL_HALLSENSOR_H_
