// Copyright 2026 insomniaTV Contributors. All rights reserved.

#ifndef SRC_SENSORS_PINGSENSOR_H_
#define SRC_SENSORS_PINGSENSOR_H_

#include <memory>
#include <string>

#include "Sensor.h"

namespace InsomniaTV {

class PingSensor : public Sensor {
public:
  PingSensor(const std::string& id, const std::string& targetIp);
  static std::shared_ptr<Sensor> create(const JsonDocument& cfg);

  std::string getId() const override { return id_; }
  std::string getType() const override { return "ping"; }
  bool read() override;
  JsonDocument getConfig() override;
  void setConfig(const JsonDocument& cfg) override;
  bool isAvailable() const override { return lastResult_; }

private:
  std::string id_;
  std::string targetIp_;
  bool lastResult_;
  uint32_t timeoutMs_;
};

}  // namespace InsomniaTV

#endif  // SRC_SENSORS_PINGSENSOR_H_
