// Copyright 2026 insomniaTV Contributors. All rights reserved.

#include "GpioInputSensor.h"

#include <memory>
#include <string>

namespace InsomniaTV {

GpioInputSensor::GpioInputSensor(const std::string& id, uint8_t pin,
                                 bool pullup)
    : id_(id),
      pin_(pin),
      pullup_(pullup),
      lastState_(false),
      lastDebounceTime_(0),
      debounceDelay_(50) {
#if defined(ARDUINO)
  pinMode(pin_, pullup_ ? INPUT_PULLUP : INPUT);
#endif
}

std::shared_ptr<Sensor> GpioInputSensor::create(const JsonDocument& cfg) {
  std::string id = cfg["id"] | "";
  uint8_t pin = cfg["pin"] | 0;
  bool pullup = cfg["pullup"] | true;
  if (id == "")
    return nullptr;
  auto sensor = std::make_shared<GpioInputSensor>(id, pin, pullup);
  sensor->setConfig(cfg);
  return sensor;
}

bool GpioInputSensor::read() {
#if defined(ARDUINO)
  bool reading = (digitalRead(pin_) == (pullup_ ? LOW : HIGH));

  if (reading != lastState_) {
    lastDebounceTime_ = millis();
  }

  if ((millis() - lastDebounceTime_) > debounceDelay_) {
    lastState_ = reading;
  }
#endif
  return lastState_;
}

JsonDocument GpioInputSensor::getConfig() {
  JsonDocument doc;
  doc["id"] = id_;
  doc["type"] = "gpio_input";
  doc["pin"] = pin_;
  doc["pullup"] = pullup_;
  doc["debounce_ms"] = debounceDelay_;
  return doc;
}

void GpioInputSensor::setConfig(const JsonDocument& cfg) {
  if (cfg["pin"].is<uint8_t>()) {
    pin_ = cfg["pin"];
  }
  if (cfg["pullup"].is<bool>()) {
    pullup_ = cfg["pullup"];
  }
  if (cfg["debounce_ms"].is<uint32_t>()) {
    debounceDelay_ = cfg["debounce_ms"];
  }
#if defined(ARDUINO)
  pinMode(pin_, pullup_ ? INPUT_PULLUP : INPUT);
#endif
}

}  // namespace InsomniaTV
