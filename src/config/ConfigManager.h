// Copyright 2026 insomniaTV Contributors. All rights reserved.

#ifndef SRC_CONFIG_CONFIGMANAGER_H_
#define SRC_CONFIG_CONFIGMANAGER_H_

#if defined(ARDUINO) || defined(ESP32)
#include <Arduino.h>
#endif

#include <functional>
#include <string>
#include <map>
#include <cstdint>

namespace InsomniaTV {

// Configuration loading result codes
enum class ConfigStatus {
  Ok,
  FileNotFound,
  InvalidJson,
  SchemaViolation,
  WriteFailed
};

// Full device configuration
struct Config {
  // WiFi
  std::string wifiSsid;
  std::string wifiPassword;

  // MQTT
  bool mqttEnabled;
  std::string mqttBroker;
  uint16_t mqttPort;
  std::string mqttClientId;
  std::string mqttTopicRoot;
  std::string mqttUser;
  std::string mqttPassword;

  // Behavior
  uint32_t inactivityTimeoutMin;
  uint8_t volumeStepPerRamp;
  uint32_t rampIntervalMin;
  uint8_t maxRampStepsBeforePoweroff;
  bool stayAwake;

  // TV Verification
  std::string tvVerifyMethod;  // "ping" or "http"
  std::string tvVerifyTarget;
  uint32_t tvVerifyTimeoutMs;
  uint8_t tvVerifyRetries;

  // IR Codes
  std::string irVolumeUpProtocol;
  uint64_t irVolumeUpCode;
  uint16_t irVolumeUpBits;
  std::string irVolumeDownProtocol;
  uint64_t irVolumeDownCode;
  uint16_t irVolumeDownBits;
  std::string irLearnedCodesPath;

  // Web
  uint16_t webPort;
  bool webAuthEnabled;
};

// Config change notification callback
using ConfigChangeCallback = std::function<void(const Config&, const Config&)>;

// Manages device configuration: load/save/validate from JSON.
// Notifies listeners on config reload via registered callbacks.
class ConfigManager {
public:
  static const char* kConfigPath;
  static const char* kSchemaPath;

  ConfigManager();
  ~ConfigManager() = default;

  // Load configuration from filesystem
  ConfigStatus load();

  // Save current configuration to filesystem
  ConfigStatus save();

  // Get active configuration (read-only reference)
  const Config& get() const;

  // Modify configuration atomically
  void set(const Config& cfg);

  // Register callback for config change notifications
  void onChange(ConfigChangeCallback callback);

  // Validate configuration against schema rules
  static bool validate(const Config& cfg, std::string& outError);

  // Reset all fields to factory defaults
  void resetToDefaults();

protected:
  Config current_;
  ConfigChangeCallback onChangeCb_;

  bool parseJson_(const std::string& json, Config& out);
  std::string toJson_(const Config& cfg);
  bool applyDefaults_();
};

}  // namespace InsomniaTV

#endif  // SRC_CONFIG_CONFIGMANAGER_H_
