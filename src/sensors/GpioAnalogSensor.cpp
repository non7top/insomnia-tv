// Copyright 2026 insomniaTV Contributors. All rights reserved.

#include "GpioAnalogSensor.h"

#include <memory>
#include <string>

#if defined(ARDUINO)
#include <Arduino.h>
#endif

namespace InsomniaTV {

GpioAnalogSensor::GpioAnalogSensor(const std::string& id, uint8_t pin)
    : id_(id), pin_(pin), lastValue_(0.0f), scale_(1.0f), offset_(0.0f) {}

std::shared_ptr<Sensor> GpioAnalogSensor::create(const JsonDocument& cfg) {
  std::string id = cfg["id"] | "";
  uint8_t pin = cfg["pin"] | 0;
  if (id == "")
    return nullptr;
  auto sensor = std::make_shared<GpioAnalogSensor>(id, pin);
  sensor->setConfig(cfg);
  return sensor;
}

bool GpioAnalogSensor::read() {
#if defined(ARDUINO)
  int raw = analogRead(pin_);
  lastValue_ = (static_cast<float>(raw) * scale_) + offset_;
  return true;
#else
  return false;
#endif
}

JsonDocument GpioAnalogSensor::getConfig() {
  JsonDocument doc;
  doc["id"] = id_;
  doc["type"] = "gpio_analog";
  doc["pin"] = pin_;
  doc["scale"] = scale_;
  doc["offset"] = offset_;
  return doc;
}

void GpioAnalogSensor::setConfig(const JsonDocument& cfg) {
  if (cfg["pin"].is<uint8_t>()) {
    pin_ = cfg["pin"];
  }
  if (cfg["scale"].is<float>()) {
    scale_ = cfg["scale"];
  }
  if (cfg["offset"].is<float>()) {
    offset_ = cfg["offset"];
  }
}

}  // namespace InsomniaTV
