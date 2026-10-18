// Copyright 2026 insomniaTV Contributors. All rights reserved.

#include "SamsungPowerSensor.h"

#include <memory>
#include <string>

#include "../tv/SamsungTvProtocol.h"

#if defined(ARDUINO)
#include <HTTPClient.h>
#endif

namespace InsomniaTV {

SamsungPowerSensor::SamsungPowerSensor(const std::string& id,
                                       const std::string& host)
    : id_(id), host_(host), reachable_(false), timeoutMs_(3000) {}

std::shared_ptr<Sensor> SamsungPowerSensor::create(const JsonDocument& cfg) {
  std::string id = cfg["id"] | "";
  if (id == "")
    return nullptr;
  auto sensor = std::make_shared<SamsungPowerSensor>(id, "");
  sensor->setConfig(cfg);
  return sensor;
}

bool SamsungPowerSensor::read() {
#if defined(ARDUINO)
  if (host_.empty()) {
    reachable_ = false;
    return false;
  }
  HTTPClient http;
  http.setTimeout(timeoutMs_);
  std::string url = "http://" + host_ + ":8001/api/v2/";
  std::string body;
  if (http.begin(url.c_str()) && http.GET() == 200) {
    body = http.getString().c_str();
  }
  http.end();
  reachable_ = !body.empty();
  return parseSamsungPower(body) == SamsungPower::On;
#else
  return false;
#endif
}

JsonDocument SamsungPowerSensor::getConfig() {
  JsonDocument doc;
  doc["id"] = id_;
  doc["type"] = "samsung_tv";
  doc["host"] = host_;
  doc["timeout_ms"] = timeoutMs_;
  return doc;
}

void SamsungPowerSensor::setConfig(const JsonDocument& cfg) {
  if (cfg["host"].is<std::string>()) {
    host_ = cfg["host"].as<std::string>();
  }
  if (cfg["timeout_ms"].is<uint32_t>()) {
    timeoutMs_ = cfg["timeout_ms"];
  }
}

}  // namespace InsomniaTV
