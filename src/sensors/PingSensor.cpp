// Copyright 2026 insomniaTV Contributors. All rights reserved.

#include "PingSensor.h"

#include <memory>
#include <string>

#if defined(ARDUINO)
#include <ESP32Ping.h>
#endif

namespace InsomniaTV {

PingSensor::PingSensor(const std::string& id, const std::string& targetIp)
    : id_(id), targetIp_(targetIp), lastResult_(false), timeoutMs_(1000) {}

std::shared_ptr<Sensor> PingSensor::create(const JsonDocument& cfg) {
  std::string id = cfg["id"] | "";
  std::string targetIp = cfg["target_ip"] | "";
  if (id == "")
    return nullptr;
  auto sensor = std::make_shared<PingSensor>(id, targetIp);
  sensor->setConfig(cfg);
  return sensor;
}

bool PingSensor::read() {
#if defined(ARDUINO)
  lastResult_ = Ping.ping(targetIp_.c_str(), 1);
#else
  lastResult_ = false;
#endif
  return lastResult_;
}

JsonDocument PingSensor::getConfig() {
  JsonDocument doc;
  doc["id"] = id_;
  doc["type"] = "ping";
  doc["target_ip"] = targetIp_;
  doc["timeout_ms"] = timeoutMs_;
  return doc;
}

void PingSensor::setConfig(const JsonDocument& cfg) {
  if (cfg["target_ip"].is<std::string>()) {
    targetIp_ = cfg["target_ip"].as<std::string>();
  }
  if (cfg["timeout_ms"].is<uint32_t>()) {
    timeoutMs_ = cfg["timeout_ms"];
  }
}

}  // namespace InsomniaTV
