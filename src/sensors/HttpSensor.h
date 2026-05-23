// Copyright 2026 insomniaTV Contributors. All rights reserved.

#ifndef SRC_SENSORS_HTTPSENSOR_H_
#define SRC_SENSORS_HTTPSENSOR_H_

#include <memory>
#include <string>

#include "Sensor.h"

namespace InsomniaTV {

class HttpSensor : public Sensor {
public:
  HttpSensor(const std::string& id, const std::string& url);
  static std::shared_ptr<Sensor> create(const JsonDocument& cfg);

  std::string getId() const override { return id_; }
  std::string getType() const override { return "http"; }
  bool read() override;
  JsonDocument getConfig() override;
  void setConfig(const JsonDocument& cfg) override;
  bool isAvailable() const override { return lastCode_ > 0; }

  int getLastCode() const { return lastCode_; }

private:
  std::string id_;
  std::string url_;
  int lastCode_;
  uint32_t timeoutMs_;
};

}  // namespace InsomniaTV

#endif  // SRC_SENSORS_HTTPSENSOR_H_
