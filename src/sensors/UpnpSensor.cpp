// Copyright 2026 insomniaTV Contributors. All rights reserved.

#include "UpnpSensor.h"

#include <algorithm>
#include <memory>
#include <string>

namespace InsomniaTV {

UpnpSensor::UpnpSensor(const std::string& id, SamsungTvDiscovery& discovery,
                       const std::string& targetName)
    : id_(id),
      discovery_(discovery),
      targetName_(targetName),
      lastResult_(false) {}

std::shared_ptr<Sensor> UpnpSensor::create(const JsonDocument& cfg,
                                           SamsungTvDiscovery& discovery) {
  std::string id = cfg["id"] | "";
  std::string targetName = cfg["target_name"] | "";
  if (id == "")
    return nullptr;
  auto sensor = std::make_shared<UpnpSensor>(id, discovery, targetName);
  sensor->setConfig(cfg);
  return sensor;
}

bool UpnpSensor::read() {
  const auto& tvs = discovery_.getDiscoveredTvs();
  auto it =
      std::find_if(tvs.begin(), tvs.end(), [this](const SamsungTvInfo& info) {
        return info.name == targetName_ || info.ip == targetName_;
      });
  lastResult_ = (it != tvs.end());
  return lastResult_;
}

JsonDocument UpnpSensor::getConfig() {
  JsonDocument doc;
  doc["id"] = id_;
  doc["type"] = "upnp";
  doc["target_name"] = targetName_;
  return doc;
}

void UpnpSensor::setConfig(const JsonDocument& cfg) {
  if (cfg["target_name"].is<std::string>()) {
    targetName_ = cfg["target_name"].as<std::string>();
  }
}

}  // namespace InsomniaTV
