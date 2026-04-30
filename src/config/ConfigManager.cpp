// Copyright 2026 insomniaTV Contributors. All rights reserved.

#if defined(ARDUINO) || defined(ESP32)
#include <Arduino.h>
#endif

#include "config/ConfigManager.h"
#include <sstream>
#include <algorithm>

namespace InsomniaTV {

const char* ConfigManager::kConfigPath = "/config/insomnia_tv.json";
const char* ConfigManager::kSchemaPath = "/config/insomnia_tv_schema.json";

ConfigManager::ConfigManager() {
  resetToDefaults();
}

ConfigStatus ConfigManager::load() {
  // Phase 1: placeholder -- filesystem integration in Phase 5
  // For now, use defaults
  applyDefaults_();
  return ConfigStatus::Ok;
}

ConfigStatus ConfigManager::save() {
  // Phase 1: placeholder -- filesystem integration in Phase 5
  return ConfigStatus::Ok;
}

const Config& ConfigManager::get() const {
  return current_;
}

void ConfigManager::set(const Config& cfg) {
  Config old = current_;
  current_ = cfg;
  if (onChangeCb_) {
    onChangeCb_(old, current_);
  }
}

void ConfigManager::onChange(ConfigChangeCallback callback) {
  onChangeCb_ = callback;
}

bool ConfigManager::validate(const Config& cfg, std::string& outError) {
  if (cfg.inactivityTimeoutMin < 1 || cfg.inactivityTimeoutMin > 120) {
    outError = "inactivity_timeout_min must be between 1 and 120";
    return false;
  }
  if (cfg.volumeStepPerRamp < 1 || cfg.volumeStepPerRamp > 10) {
    outError = "volume_step_per_ramp must be between 1 and 10";
    return false;
  }
  if (cfg.rampIntervalMin < 1 || cfg.rampIntervalMin > 30) {
    outError = "ramp_interval_min must be between 1 and 30";
    return false;
  }
  if (cfg.maxRampStepsBeforePoweroff < 1 ||
      cfg.maxRampStepsBeforePoweroff > 50) {
    outError = "max_ramp_steps_before_poweroff must be between 1 and 50";
    return false;
  }
  if (cfg.tvVerifyTimeoutMs < 500 || cfg.tvVerifyTimeoutMs > 10000) {
    outError = "tv_verify timeout_ms must be between 500 and 10000";
    return false;
  }
  if (cfg.tvVerifyRetries < 1 || cfg.tvVerifyRetries > 5) {
    outError = "tv_verify retries must be between 1 and 5";
    return false;
  }
  if (cfg.tvVerifyMethod != "ping" && cfg.tvVerifyMethod != "http") {
    outError = "tv_verify method must be 'ping' or 'http'";
    return false;
  }
  return true;
}

std::string trim_(const std::string& str) {
  const std::string whitespace = " \t\n\r";
  auto start = str.find_first_not_of(whitespace);
  if (start == std::string::npos) return "";
  auto end = str.find_last_not_of(whitespace);
  return str.substr(start, end - start + 1);
}

std::string toLower_(std::string str) {
  std::transform(str.begin(), str.end(), str.begin(), ::tolower);
  return str;
}

bool startsWith_(const std::string& str, const std::string& prefix) {
  return str.size() >= prefix.size() && str.substr(0, prefix.size()) == prefix;
}

void ConfigManager::resetToDefaults() {
  Config d;
  d.wifiSsid = "";
  d.wifiPassword = "";
  d.mqttEnabled = true;
  d.mqttBroker = "";
  d.mqttPort = 1883;
  d.mqttClientId = "insomniatv-esp32";
  d.mqttTopicRoot = "home/insomnia_tv";
  d.mqttUser = "";
  d.mqttPassword = "";
  d.inactivityTimeoutMin = 15;
  d.volumeStepPerRamp = 1;
  d.rampIntervalMin = 2;
  d.maxRampStepsBeforePoweroff = 10;
  d.stayAwake = true;
  d.tvVerifyMethod = "ping";
  d.tvVerifyTarget = "";
  d.tvVerifyTimeoutMs = 1000;
  d.tvVerifyRetries = 3;
  d.irVolumeUpProtocol = "NEC";
  d.irVolumeUpCode = 0;
  d.irVolumeUpBits = 32;
  d.irVolumeDownProtocol = "NEC";
  d.irVolumeDownCode = 0;
  d.irVolumeDownBits = 32;
  d.irLearnedCodesPath = "/ir_learned.json";
  d.webPort = 80;
  d.webAuthEnabled = false;
  current_ = d;
}

bool ConfigManager::parseJson_(const std::string& json, Config& out) {
  // Phase 5: full JSON parsing with ArduinoJson
  // For now, return false to indicate parsing not implemented
  (void)json;
  (void)out;
  return false;
}

std::string ConfigManager::toJson_(const Config& cfg) {
  // Phase 5: full JSON serialization
  // Simple JSON string builder without external dependency
  std::ostringstream oss;
  oss << "{\n";
  oss << "  \"wifi_ssid\": \"" << cfg.wifiSsid << "\",\n";
  oss << "  \"wifi_password\": \"" << cfg.wifiPassword << "\",\n";
  oss << "  \"mqtt_enabled\": " << (cfg.mqttEnabled ? "true" : "false") << ",\n";
  oss << "  \"mqtt_broker\": \"" << cfg.mqttBroker << "\",\n";
  oss << "  \"mqtt_port\": " << cfg.mqttPort << ",\n";
  oss << "  \"mqtt_client_id\": \"" << cfg.mqttClientId << "\",\n";
  oss << "  \"mqtt_topic_root\": \"" << cfg.mqttTopicRoot << "\",\n";
  oss << "  \"mqtt_user\": \"" << cfg.mqttUser << "\",\n";
  oss << "  \"mqtt_password\": \"" << cfg.mqttPassword << "\",\n";
  oss << "  \"inactivity_timeout_min\": " << cfg.inactivityTimeoutMin << ",\n";
  oss << "  \"volume_step_per_ramp\": " << (int)cfg.volumeStepPerRamp << ",\n";
  oss << "  \"ramp_interval_min\": " << cfg.rampIntervalMin << ",\n";
  oss << "  \"max_ramp_steps_before_poweroff\": " << (int)cfg.maxRampStepsBeforePoweroff << ",\n";
  oss << "  \"stay_awake\": " << (cfg.stayAwake ? "true" : "false") << ",\n";
  oss << "  \"tv_verify_method\": \"" << cfg.tvVerifyMethod << "\",\n";
  oss << "  \"tv_verify_target\": \"" << cfg.tvVerifyTarget << "\",\n";
  oss << "  \"tv_verify_timeout_ms\": " << cfg.tvVerifyTimeoutMs << ",\n";
  oss << "  \"tv_verify_retries\": " << (int)cfg.tvVerifyRetries << ",\n";
  oss << "  \"ir_volume_up_protocol\": \"" << cfg.irVolumeUpProtocol << "\",\n";
  oss << "  \"ir_volume_up_code\": " << cfg.irVolumeUpCode << ",\n";
  oss << "  \"ir_volume_up_bits\": " << (int)cfg.irVolumeUpBits << ",\n";
  oss << "  \"ir_volume_down_protocol\": \"" << cfg.irVolumeDownProtocol << "\",\n";
  oss << "  \"ir_volume_down_code\": " << cfg.irVolumeDownCode << ",\n";
  oss << "  \"ir_volume_down_bits\": " << (int)cfg.irVolumeDownBits << ",\n";
  oss << "  \"ir_learned_codes_path\": \"" << cfg.irLearnedCodesPath << "\",\n";
  oss << "  \"web_port\": " << cfg.webPort << ",\n";
  oss << "  \"web_auth_enabled\": " << (cfg.webAuthEnabled ? "true" : "false") << "\n";
  oss << "}";
  return oss.str();
}

bool ConfigManager::applyDefaults_() {
  resetToDefaults();
  return true;
}

}  // namespace InsomniaTV
