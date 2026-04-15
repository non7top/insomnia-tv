#ifndef INSOMNIATV_ConfigManager_H
#define INSOMNIATV_ConfigManager_H

#include <Arduino.h>
#include <functional>

namespace InsomniaTV {

/** Configuration loaded result codes */
enum class ConfigStatus {
    Ok,
    FileNotFound,
    InvalidJson,
    SchemaViolation,
    WriteFailed
};

/** Full device configuration */
struct Config {
    // WiFi
    String wifiSsid;
    String wifiPassword;

    // MQTT
    bool mqttEnabled;
    String mqttBroker;
    uint16_t mqttPort;
    String mqttClientId;
    String mqttTopicRoot;
    String mqttUser;
    String mqttPassword;

    // Behavior
    uint32_t inactivityTimeoutMin;
    uint8_t volumeStepPerRamp;
    uint32_t rampIntervalMin;
    uint8_t maxRampStepsBeforePoweroff;
    bool stayAwake;

    // TV Verification
    String tvVerifyMethod;  // "ping" | "http"
    String tvVerifyTarget;
    uint32_t tvVerifyTimeoutMs;
    uint8_t tvVerifyRetries;

    // IR Codes
    String irVolumeUpProtocol;
    uint64_t irVolumeUpCode;
    uint16_t irVolumeUpBits;
    String irVolumeDownProtocol;
    uint64_t irVolumeDownCode;
    uint16_t irVolumeDownBits;
    String irLearnedCodesPath;

    // Web
    uint16_t webPort;
    bool webAuthEnabled;
};

/** Config change notification callback */
using ConfigChangeCallback = std::function<void(const Config& oldCfg, const Config& newCfg)>;

/**
 * Manages device configuration: load/save/validate from LittleFS JSON.
 * Notifies listeners on config reload via registered callbacks.
 */
class ConfigManager {
public:
    static const char* kConfigPath;
    static const char* kSchemaPath;

    ConfigManager();
    ~ConfigManager() = default;

    /** Load configuration from filesystem */
    ConfigStatus load();

    /** Save current configuration to filesystem */
    ConfigStatus save();

    /** Get active configuration (read-only reference) */
    const Config& get() const;

    /** Modify configuration atomically */
    void set(const Config& cfg);

    /** Register callback for config change notifications */
    void onChange(ConfigChangeCallback callback);

    /** Validate configuration against schema rules */
    static bool validate(const Config& cfg, String& outError);

    /** Reset all fields to factory defaults */
    void resetToDefaults();

private:
    Config current_;
    ConfigChangeCallback onChangeCb_;

    bool parseJson_(const String& json, Config& out);
    String toJson_(const Config& cfg);
    bool applyDefaults_();
};

} // namespace InsomniaTV

#endif // INSOMNIATV_ConfigManager_H
