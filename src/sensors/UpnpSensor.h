// Copyright 2026 insomniaTV Contributors. All rights reserved.

#ifndef SRC_SENSORS_UPNPSENSOR_H_
#define SRC_SENSORS_UPNPSENSOR_H_

#include <memory>
#include <string>

#include "../discovery/SamsungTvDiscovery.h"
#include "Sensor.h"

namespace InsomniaTV {

class UpnpSensor : public Sensor {
public:
  UpnpSensor(const std::string& id, SamsungTvDiscovery& discovery,
             const std::string& targetName);
  static std::shared_ptr<Sensor> create(const JsonDocument& cfg,
                                        SamsungTvDiscovery& discovery);

  std::string getId() const override { return id_; }
  std::string getType() const override { return "upnp"; }
  bool read() override;
  JsonDocument getConfig() override;
  void setConfig(const JsonDocument& cfg) override;
  bool isAvailable() const override { return lastResult_; }

private:
  std::string id_;
  SamsungTvDiscovery& discovery_;
  std::string targetName_;
  bool lastResult_;
};

}  // namespace InsomniaTV

#endif  // SRC_SENSORS_UPNPSENSOR_H_
