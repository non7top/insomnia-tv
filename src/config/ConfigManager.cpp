// Copyright 2026 insomniaTV Contributors. All rights reserved.

#include "config/ConfigManager.h"

#include <ArduinoJson.h>

#include <string>

#if defined(ARDUINO)
#include <LittleFS.h>
#endif

namespace InsomniaTV {

const char* ConfigManager::kConfigPath = "/config/insomnia_tv.json";
const char* ConfigManager::kSchemaPath = "/config/insomnia_tv_schema.json";

ConfigManager::ConfigManager() {
  resetToDefaults();
}

ConfigStatus ConfigManager::load() {
#if defined(ARDUINO)
  if (!LittleFS.exists(kConfigPath)) {
    applyDefaults_();
    return ConfigStatus::FileNotFound;
  }

  File file = LittleFS.open(kConfigPath, "r");
  if (!file)
    return ConfigStatus::FileNotFound;

  std::string json = file.readString().c_str();
  file.close();

  Config next;
  if (!parseJson_(json, next)) {
    return ConfigStatus::InvalidJson;
  }

  current_ = next;
  return ConfigStatus::Ok;
#else
  applyDefaults_();
  return ConfigStatus::Ok;
#endif
}

ConfigStatus ConfigManager::save() {
  std::string json = toJson_(current_);
#if defined(ARDUINO)
  if (!LittleFS.exists("/config")) {
    LittleFS.mkdir("/config");
  }
  // Write to a temp file then rename for atomicity — a power cut during
  // the write leaves the old file intact rather than a partial corrupt one.
  static const char* kTmpPath = "/config/insomnia_tv.json.tmp";
  File file = LittleFS.open(kTmpPath, "w");
  if (!file) {
    return ConfigStatus::WriteFailed;
  }
  file.print(json.c_str());
  file.close();
  LittleFS.remove(kConfigPath);
  if (!LittleFS.rename(kTmpPath, kConfigPath)) {
    return ConfigStatus::WriteFailed;
  }
  return ConfigStatus::Ok;
#else
  (void)json;
  return ConfigStatus::Ok;
#endif
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
  d.sensorsJson = "[]";
  d.tvSmConfigJson = "{\"hysteresis_count\":2,\"detection_sensors\":[]}";
  d.configVersion = 1;
  current_ = d;
}

bool ConfigManager::parseJson_(const std::string& json, Config& out) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, json);
  if (error)
    return false;

  if (doc["wifi"].is<JsonObject>()) {
    out.wifiSsid = doc["wifi"]["ssid"] | "";
    out.wifiPassword = doc["wifi"]["password"] | "";
  }

  if (doc["mqtt"].is<JsonObject>()) {
    out.mqttEnabled = doc["mqtt"]["enabled"] | true;
    out.mqttBroker = doc["mqtt"]["broker"] | "";
    out.mqttPort = doc["mqtt"]["port"] | 1883;
    out.mqttClientId = doc["mqtt"]["client_id"] | "insomniatv-esp32";
    out.mqttTopicRoot = doc["mqtt"]["topic_root"] | "home/insomnia_tv";
    out.mqttUser = doc["mqtt"]["user"] | "";
    out.mqttPassword = doc["mqtt"]["password"] | "";
  }

  if (doc["behavior"].is<JsonObject>()) {
    out.inactivityTimeoutMin = doc["behavior"]["inactivity_timeout_min"] | 15;
    out.volumeStepPerRamp = doc["behavior"]["volume_step_per_ramp"] | 1;
    out.rampIntervalMin = doc["behavior"]["ramp_interval_min"] | 2;
    out.maxRampStepsBeforePoweroff =
        doc["behavior"]["max_ramp_steps_before_poweroff"] | 10;
    out.stayAwake = doc["behavior"]["stay_awake"] | true;
  }

  if (doc["tv_verification"].is<JsonObject>()) {
    out.tvVerifyMethod = doc["tv_verification"]["method"] | "ping";
    out.tvVerifyTarget = doc["tv_verification"]["target"] | "";
    out.tvVerifyTimeoutMs = doc["tv_verification"]["timeout_ms"] | 1000;
    out.tvVerifyRetries = doc["tv_verification"]["retries"] | 3;
  }

  if (doc["ir_codes"].is<JsonObject>()) {
    JsonObject ir = doc["ir_codes"];
    if (ir["volume_up"].is<JsonObject>()) {
      JsonObject volUp = ir["volume_up"];
      out.irVolumeUpProtocol = volUp["protocol"] | "NEC";
      out.irVolumeUpCode = volUp["hex"] | 0ULL;
      out.irVolumeUpBits = volUp["bits"] | 32;
    }
    if (ir["volume_down"].is<JsonObject>()) {
      JsonObject volDown = ir["volume_down"];
      out.irVolumeDownProtocol = volDown["protocol"] | "NEC";
      out.irVolumeDownCode = volDown["hex"] | 0ULL;
      out.irVolumeDownBits = volDown["bits"] | 32;
    }
    out.irLearnedCodesPath = ir["learned_codes_path"] | "/ir_learned.json";
  }

  if (doc["web"].is<JsonObject>()) {
    out.webPort = doc["web"]["port"] | 80;
    out.webAuthEnabled = doc["web"]["auth_enabled"] | false;
  }

  if (doc["sensors"].is<JsonArray>()) {
    serializeJson(doc["sensors"], out.sensorsJson);
  }

  if (doc["tv_sm"].is<JsonObject>()) {
    serializeJson(doc["tv_sm"], out.tvSmConfigJson);
  }

  out.configVersion = doc["config_version"] | 1;

  return true;
}

std::string ConfigManager::toJson_(const Config& cfg) {
  JsonDocument doc;

  JsonObject wifi = doc["wifi"].to<JsonObject>();
  wifi["ssid"] = cfg.wifiSsid;
  wifi["password"] = cfg.wifiPassword;

  JsonObject mqtt = doc["mqtt"].to<JsonObject>();
  mqtt["enabled"] = cfg.mqttEnabled;
  mqtt["broker"] = cfg.mqttBroker;
  mqtt["port"] = cfg.mqttPort;
  mqtt["client_id"] = cfg.mqttClientId;
  mqtt["topic_root"] = cfg.mqttTopicRoot;
  mqtt["user"] = cfg.mqttUser;
  mqtt["password"] = cfg.mqttPassword;

  JsonObject behavior = doc["behavior"].to<JsonObject>();
  behavior["inactivity_timeout_min"] = cfg.inactivityTimeoutMin;
  behavior["volume_step_per_ramp"] = cfg.volumeStepPerRamp;
  behavior["ramp_interval_min"] = cfg.rampIntervalMin;
  behavior["max_ramp_steps_before_poweroff"] = cfg.maxRampStepsBeforePoweroff;
  behavior["stay_awake"] = cfg.stayAwake;

  JsonObject tv = doc["tv_verification"].to<JsonObject>();
  tv["method"] = cfg.tvVerifyMethod;
  tv["target"] = cfg.tvVerifyTarget;
  tv["timeout_ms"] = cfg.tvVerifyTimeoutMs;
  tv["retries"] = cfg.tvVerifyRetries;

  JsonObject ir = doc["ir_codes"].to<JsonObject>();
  JsonObject volUp = ir["volume_up"].to<JsonObject>();
  volUp["protocol"] = cfg.irVolumeUpProtocol;
  volUp["hex"] = cfg.irVolumeUpCode;
  volUp["bits"] = cfg.irVolumeUpBits;

  JsonObject volDown = ir["volume_down"].to<JsonObject>();
  volDown["protocol"] = cfg.irVolumeDownProtocol;
  volDown["hex"] = cfg.irVolumeDownCode;
  volDown["bits"] = cfg.irVolumeDownBits;

  ir["learned_codes_path"] = cfg.irLearnedCodesPath;

  JsonObject web = doc["web"].to<JsonObject>();
  web["port"] = cfg.webPort;
  web["auth_enabled"] = cfg.webAuthEnabled;

  if (!cfg.sensorsJson.empty()) {
    JsonDocument sensorDoc;
    deserializeJson(sensorDoc, cfg.sensorsJson);
    doc["sensors"] = sensorDoc.as<JsonArray>();
  }

  if (!cfg.tvSmConfigJson.empty()) {
    JsonDocument tvDoc;
    deserializeJson(tvDoc, cfg.tvSmConfigJson);
    doc["tv_sm"] = tvDoc.as<JsonObject>();
  }

  doc["config_version"] = cfg.configVersion;

  std::string out;
  serializeJson(doc, out);
  return out;
}

bool ConfigManager::applyDefaults_() {
  resetToDefaults();
  return true;
}

}  // namespace InsomniaTV
