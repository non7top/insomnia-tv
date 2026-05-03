// Copyright 2026 insomniaTV Contributors. All rights reserved.

#ifndef SRC_SENSORS_GPIOINPUTSENSOR_H_
#define SRC_SENSORS_GPIOINPUTSENSOR_H_

#include <memory>
#include <string>

#include "Sensor.h"

#if defined(ARDUINO)
#include <Arduino.h>
#endif

namespace InsomniaTV {

class GpioInputSensor : public Sensor {
public:
  GpioInputSensor(const std::string& id, uint8_t pin, bool pullup = true);
  static std::shared_ptr<Sensor> create(const JsonDocument& cfg);

  std::string getId() const override { return id_; }
  std::string getType() const override { return "gpio_input"; }
  bool read() override;
  JsonDocument getConfig() override;
  void setConfig(const JsonDocument& cfg) override;

private:
  std::string id_;
  uint8_t pin_;
  bool pullup_;
  bool lastState_;
  uint32_t lastDebounceTime_;
  uint32_t debounceDelay_;
};

}  // namespace InsomniaTV

#endif  // SRC_SENSORS_GPIOINPUTSENSOR_H_
