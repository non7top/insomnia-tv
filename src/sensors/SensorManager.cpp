// Copyright 2026 insomniaTV Contributors. All rights reserved.

#include "SensorManager.h"

#include <ArduinoJson.h>

#include <memory>
#include <string>
#include <vector>

#include "GpioAnalogSensor.h"
#include "GpioInputSensor.h"
#include "HttpSensor.h"
#include "PingSensor.h"
#include "UpnpSensor.h"

namespace InsomniaTV {

SensorManager& SensorManager::instance() {
  static SensorManager instance;
  return instance;
}

void SensorManager::registerFactory(const std::string& type,
                                    SensorFactory factory) {
  std::lock_guard<std::mutex> lock(mutex_);
  factories_[type] = factory;
}

void SensorManager::registerSensor(std::shared_ptr<Sensor> sensor) {
  if (!sensor)
    return;
  std::lock_guard<std::mutex> lock(mutex_);
  sensors_[sensor->getId()] = sensor;

  for (auto& cb : subscribers_) {
    cb(sensor);
  }
}

std::shared_ptr<Sensor> SensorManager::getSensor(const std::string& id) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = sensors_.find(id);
  if (it != sensors_.end()) {
    return it->second;
  }
  return nullptr;
}

std::vector<std::shared_ptr<Sensor>> SensorManager::listSensors(
    const std::string& type) {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<std::shared_ptr<Sensor>> result;
  for (auto const& [id, sensor] : sensors_) {
    if (type == "" || sensor->getType() == type) {
      result.push_back(sensor);
    }
  }
  return result;
}

void SensorManager::subscribe(SensorCallback callback) {
  std::lock_guard<std::mutex> lock(mutex_);
  subscribers_.push_back(callback);
}

void SensorManager::subscribeValueChange(ValueChangeCallback callback) {
  std::lock_guard<std::mutex> lock(mutex_);
  valueChangeSubscribers_.push_back(callback);
}

void SensorManager::clear() {
  std::lock_guard<std::mutex> lock(mutex_);
  sensors_.clear();
}

void SensorManager::removeSensor(const std::string& id) {
  std::lock_guard<std::mutex> lock(mutex_);
  sensors_.erase(id);
}

void SensorManager::init(const std::string& configJson,
                         SamsungTvDiscovery& discovery) {
  // Register built-in factories if map is empty
  if (factories_.empty()) {
    registerFactory("gpio_input", [](const JsonDocument& cfg) {
      return GpioInputSensor::create(cfg);
    });
    registerFactory("gpio_analog", [](const JsonDocument& cfg) {
      return GpioAnalogSensor::create(cfg);
    });
    registerFactory("ping", [](const JsonDocument& cfg) {
      return PingSensor::create(cfg);
    });
    registerFactory("http", [](const JsonDocument& cfg) {
      return HttpSensor::create(cfg);
    });
    registerFactory("upnp", [&discovery](const JsonDocument& cfg) {
      return UpnpSensor::create(cfg, discovery);
    });
  }

  if (configJson.empty())
    return;

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, configJson);
  if (error)
    return;

  if (doc.is<JsonArray>()) {
    // Clear before re-loading to avoid orphaned sensors. Sensors registered
    // manually and absent from config will be lost (intended for config sync).
    clear();

    for (JsonObject sensorCfg : doc.as<JsonArray>()) {
      std::string type = sensorCfg["type"] | "";
      std::string id = sensorCfg["id"] | "";

      if (type != "" && id != "") {
        std::lock_guard<std::mutex> lock(mutex_);
        if (factories_.count(type)) {
          auto sensor = factories_[type](sensorCfg);
          if (sensor) {
            sensor->setState(Sensor::State::READY);
            sensors_[id] = sensor;
          }
        }
      }
    }
  }
}

void SensorManager::tick() {
  std::vector<std::pair<std::string, bool>> changes;
  std::vector<ValueChangeCallback> cbs;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto const& [id, sensor] : sensors_) {
      bool value = sensor->read();
      sensor->setState(value ? Sensor::State::READY : Sensor::State::ERROR);
      auto it = lastValues_.find(id);
      if (it == lastValues_.end() || it->second != value) {
        lastValues_[id] = value;
        changes.emplace_back(id, value);
      }
    }
    cbs = valueChangeSubscribers_;
  }
  // Notify outside the lock to avoid deadlock with re-entrant subscribers.
  for (auto const& [id, value] : changes) {
    for (auto& cb : cbs) {
      cb(id, value);
    }
  }
}

bool SensorManager::getCachedValue(const std::string& id) const {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = lastValues_.find(id);
  return it != lastValues_.end() ? it->second : false;
}

}  // namespace InsomniaTV
