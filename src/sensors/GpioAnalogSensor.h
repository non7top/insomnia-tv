// Copyright 2026 insomniaTV Contributors. All rights reserved.

#ifndef SRC_SENSORS_GPIOANALOGSENSOR_H_
#define SRC_SENSORS_GPIOANALOGSENSOR_H_

#include <memory>
#include <string>

#include "Sensor.h"

namespace InsomniaTV {

class GpioAnalogSensor : public Sensor {
public:
  GpioAnalogSensor(const std::string& id, uint8_t pin);
  static std::shared_ptr<Sensor> create(const JsonDocument& cfg);

  std::string getId() const override { return id_; }
  std::string getType() const override { return "gpio_analog"; }
  bool read() override;
  JsonDocument getConfig() override;
  void setConfig(const JsonDocument& cfg) override;

  float getValue() const { return lastValue_; }

private:
  std::string id_;
  uint8_t pin_;
  float lastValue_;
  float scale_;
  float offset_;
};

}  // namespace InsomniaTV

#endif  // SRC_SENSORS_GPIOANALOGSENSOR_H_
