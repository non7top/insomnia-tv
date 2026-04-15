#include "config/ConfigManager.h"

namespace InsomniaTV {

const char* ConfigManager::kConfigPath = "/config/insomnia_tv.json";
const char* ConfigManager::kSchemaPath = "/config/insomnia_tv_schema.json";

ConfigManager::ConfigManager() {
    resetToDefaults();
}

ConfigStatus ConfigManager::load() {
    // Phase 1: placeholder — filesystem integration in Phase 5
    // For now, use defaults
    (void)kConfigPath;
    applyDefaults_();
    return ConfigStatus::Ok;
}

ConfigStatus ConfigManager::save() {
    // Phase 1: placeholder — filesystem integration in Phase 5
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

bool ConfigManager::validate(const Config& cfg, String& outError) {
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
    if (cfg.maxRampStepsBeforePoweroff < 1 || cfg.maxRampStepsBeforePoweroff > 50) {
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
    Config defaults;
    defaults.wifiSsid = "";
    defaults.wifiPassword = "";

    defaults.mqttEnabled = true;
    defaults.mqttBroker = "";
    defaults.mqttPort = 1883;
    defaults.mqttClientId = "insomniatv-esp32";
    defaults.mqttTopicRoot = "home/insomnia_tv";
    defaults.mqttUser = "";
    defaults.mqttPassword = "";

    defaults.inactivityTimeoutMin = 15;
    defaults.volumeStepPerRamp = 1;
    defaults.rampIntervalMin = 2;
    defaults.maxRampStepsBeforePoweroff = 10;
    defaults.stayAwake = true;

    defaults.tvVerifyMethod = "ping";
    defaults.tvVerifyTarget = "";
    defaults.tvVerifyTimeoutMs = 1000;
    defaults.tvVerifyRetries = 3;

    defaults.irVolumeUpProtocol = "NEC";
    defaults.irVolumeUpCode = 0;
    defaults.irVolumeUpBits = 32;
    defaults.irVolumeDownProtocol = "NEC";
    defaults.irVolumeDownCode = 0;
    defaults.irVolumeDownBits = 32;
    defaults.irLearnedCodesPath = "/ir_learned.json";

    defaults.webPort = 80;
    defaults.webAuthEnabled = false;

    current_ = defaults;
}

bool ConfigManager::parseJson_(const String& json, Config& out) {
    // Phase 5: full JSON parsing with ArduinoJson
    (void)json;
    (void)out;
    return false;
}

String ConfigManager::toJson_(const Config& cfg) {
    // Phase 5: full JSON serialization
    (void)cfg;
    return String();
}

bool ConfigManager::applyDefaults_() {
    resetToDefaults();
    return true;
}

} // namespace InsomniaTV
