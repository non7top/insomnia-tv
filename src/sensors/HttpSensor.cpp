// Copyright 2026 insomniaTV Contributors. All rights reserved.

#include "HttpSensor.h"

#include <memory>
#include <string>

#if defined(ARDUINO)
#include <HTTPClient.h>
#endif

namespace InsomniaTV {

HttpSensor::HttpSensor(const std::string& id, const std::string& url)
    : id_(id), url_(url), lastCode_(-1), timeoutMs_(5000) {}

std::shared_ptr<Sensor> HttpSensor::create(const JsonDocument& cfg) {
  std::string id = cfg["id"] | "";
  std::string url = cfg["url"] | "";
  if (id == "")
    return nullptr;
  auto sensor = std::make_shared<HttpSensor>(id, url);
  sensor->setConfig(cfg);
  return sensor;
}

bool HttpSensor::read() {
#if defined(ARDUINO)
  HTTPClient http;
  http.begin(url_.c_str());
  http.setTimeout(timeoutMs_);
  lastCode_ = http.GET();
  http.end();
  return lastCode_ > 0;
#else
  return false;
#endif
}

JsonDocument HttpSensor::getConfig() {
  JsonDocument doc;
  doc["id"] = id_;
  doc["type"] = "http";
  doc["url"] = url_;
  doc["timeout_ms"] = timeoutMs_;
  return doc;
}

void HttpSensor::setConfig(const JsonDocument& cfg) {
  if (cfg["url"].is<std::string>()) {
    url_ = cfg["url"].as<std::string>();
  }
  if (cfg["timeout_ms"].is<uint32_t>()) {
    timeoutMs_ = cfg["timeout_ms"];
  }
}

}  // namespace InsomniaTV
