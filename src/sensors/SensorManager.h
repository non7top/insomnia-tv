// Copyright 2026 insomniaTV Contributors. All rights reserved.

#ifndef SRC_SENSORS_SENSORMANAGER_H_
#define SRC_SENSORS_SENSORMANAGER_H_

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "../discovery/SamsungTvDiscovery.h"
#include "Sensor.h"

namespace InsomniaTV {

using SensorCallback = std::function<void(std::shared_ptr<Sensor>)>;
using SensorFactory =
    std::function<std::shared_ptr<Sensor>(const JsonDocument&)>;
using ValueChangeCallback =
    std::function<void(const std::string& id, bool value)>;

/**
 * @brief Singleton manager for all sensors in the system.
 */
class SensorManager {
public:
  static SensorManager& instance();

  // Register a sensor factory for a specific type
  void registerFactory(const std::string& type, SensorFactory factory);

  // Register a sensor instance
  void registerSensor(std::shared_ptr<Sensor> sensor);

  // Get a sensor by its unique ID
  std::shared_ptr<Sensor> getSensor(const std::string& id);

  // List all sensors, optionally filtered by type
  std::vector<std::shared_ptr<Sensor>> listSensors(
      const std::string& type = "");

  // Subscribe to sensor registration events
  void subscribe(SensorCallback callback);

  // Subscribe to sensor value changes (fires only when value differs from
  // last read)
  void subscribeValueChange(ValueChangeCallback callback);

  // Thread-safe access to all sensors
  void init(const std::string& configJson, SamsungTvDiscovery& discovery);
  void tick();
  void clear();
  void removeSensor(const std::string& id);

private:
  SensorManager() = default;
  ~SensorManager() = default;
  SensorManager(const SensorManager&) = delete;
  SensorManager& operator=(const SensorManager&) = delete;

  std::map<std::string, std::shared_ptr<Sensor>> sensors_;
  std::map<std::string, SensorFactory> factories_;
  std::vector<SensorCallback> subscribers_;
  std::vector<ValueChangeCallback> valueChangeSubscribers_;
  std::map<std::string, bool> lastValues_;
  mutable std::mutex mutex_;
};

}  // namespace InsomniaTV

#endif  // SRC_SENSORS_SENSORMANAGER_H_
