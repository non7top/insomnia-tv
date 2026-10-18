// Copyright 2026 insomniaTV Contributors. All rights reserved.

#ifndef SRC_SENSORS_SAMSUNGPOWERSENSOR_H_
#define SRC_SENSORS_SAMSUNGPOWERSENSOR_H_

#include <memory>
#include <string>

#include "Sensor.h"

namespace InsomniaTV {

// True while a Samsung Tizen TV reports PowerState "on" on its REST API
// (port 8001). Unlike ping/http, it goes false in standby, when the TV still
// answers on the network.
class SamsungPowerSensor : public Sensor {
public:
  SamsungPowerSensor(const std::string& id, const std::string& host);
  static std::shared_ptr<Sensor> create(const JsonDocument& cfg);

  std::string getId() const override { return id_; }
  std::string getType() const override { return "samsung_tv"; }
  bool read() override;
  JsonDocument getConfig() override;
  void setConfig(const JsonDocument& cfg) override;
  bool isAvailable() const override { return reachable_; }

private:
  std::string id_;
  std::string host_;
  bool reachable_;
  uint32_t timeoutMs_;
};

}  // namespace InsomniaTV

#endif  // SRC_SENSORS_SAMSUNGPOWERSENSOR_H_
