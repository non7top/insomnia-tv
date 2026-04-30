#include "config/ConfigManager.h"
#include <ArduinoJson.h>

namespace InsomniaTV {

const char* ConfigManager::kConfigPath = "/config/insomnia_tv.json";
const char* ConfigManager::kSchemaPath = "/config/insomnia_tv_schema.json";

ConfigManager::ConfigManager() {
  resetToDefaults();
}

ConfigStatus ConfigManager::load() {
  // Phase 1: placeholder for FS -- filesystem integration in Phase 5
  // For now, just ensuring it returns Ok with defaults.
  applyDefaults_();
  return ConfigStatus::Ok;
}

ConfigStatus ConfigManager::save() {
  // Phase 1: placeholder for FS -- filesystem integration in Phase 5
  // We can still call toJson_ to ensure it's functional.
  std::string json = toJson_(current_);
  (void)json;
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
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, json);
  if (error) return false;

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
    out.irVolumeUpProtocol = doc["ir_codes"]["volume_up"]["protocol"] | "NEC";
    out.irVolumeUpCode = doc["ir_codes"]["volume_up"]["hex"] | 0;
    out.irVolumeUpBits = doc["ir_codes"]["volume_up"]["bits"] | 32;
    out.irVolumeDownProtocol =
        doc["ir_codes"]["volume_down"]["protocol"] | "NEC";
    out.irVolumeDownCode = doc["ir_codes"]["volume_down"]["hex"] | 0;
    out.irVolumeDownBits = doc["ir_codes"]["volume_down"]["bits"] | 32;
    out.irLearnedCodesPath =
        doc["ir_codes"]["learned_codes_path"] | "/ir_learned.json";
  }

  if (doc["web"].is<JsonObject>()) {
    out.webPort = doc["web"]["port"] | 80;
    out.webAuthEnabled = doc["web"]["auth_enabled"] | false;
  }

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
  ir["volume_up"]["protocol"] = cfg.irVolumeUpProtocol;
  ir["volume_up"]["hex"] = cfg.irVolumeUpCode;
  ir["volume_up"]["bits"] = cfg.irVolumeUpBits;
  ir["volume_down"]["protocol"] = cfg.irVolumeDownProtocol;
  ir["volume_down"]["hex"] = cfg.irVolumeDownCode;
  ir["volume_down"]["bits"] = cfg.irVolumeDownBits;
  ir["learned_codes_path"] = cfg.irLearnedCodesPath;

  JsonObject web = doc["web"].to<JsonObject>();
  web["port"] = cfg.webPort;
  web["auth_enabled"] = cfg.webAuthEnabled;

  std::string out;
  serializeJson(doc, out);
  return out;
}

bool ConfigManager::applyDefaults_() {
  resetToDefaults();
  return true;
}

}  // namespace InsomniaTV
