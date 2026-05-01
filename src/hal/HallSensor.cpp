// Copyright 2026 insomniaTV Contributors. All rights reserved.

#include "HallSensor.h"
#if defined(ARDUINO)
#include <Arduino.h>
#endif

namespace InsomniaTV {

HallSensor::HallSensor(uint8_t pin) : _pin(pin), _zeroPoint(2.5f) {}

float HallSensor::getCurrentAmperes() {
#if defined(ARDUINO)
    int raw = analogRead(_pin);
    // Basic conversion logic (example)
    float voltage = (raw * 3.3f) / 4095.0f;
    return (voltage - _zeroPoint) / 0.185f;  // Example sensitivity 185mV/A
#else
    return 0.0f;
#endif
}

void HallSensor::calibrate() {
#if defined(ARDUINO)
    // Simple average over several samples
    float sum = 0;
    for (int i = 0; i < 10; ++i) {
        sum += analogRead(_pin);
        delay(10);
    }
    _zeroPoint = (sum / 10.0f * 3.3f) / 4095.0f;
#endif
}

}  // namespace InsomniaTV
