// Copyright 2026 insomniaTV Contributors. All rights reserved.

#include <unity.h>

#include "config/ConfigManager.h"

// ---------------------------------------------------------------------------
// Test: Default config values are set correctly
// ---------------------------------------------------------------------------
void test_config_defaults(void) {
  InsomniaTV::ConfigManager mgr;
  mgr.resetToDefaults();
  const InsomniaTV::Config& cfg = mgr.get();

  TEST_ASSERT_FALSE(cfg.mqttEnabled == false);
  TEST_ASSERT_EQUAL_UINT16(1883, cfg.mqttPort);
  TEST_ASSERT_EQUAL_UINT32(15, cfg.inactivityTimeoutMin);
  TEST_ASSERT_EQUAL_UINT8(1, cfg.volumeStepPerRamp);
  TEST_ASSERT_EQUAL_UINT32(2, cfg.rampIntervalMin);
  TEST_ASSERT_EQUAL_UINT8(10, cfg.maxRampStepsBeforePoweroff);
  TEST_ASSERT_TRUE(cfg.stayAwake);
  TEST_ASSERT_EQUAL_STRING("ping", cfg.tvVerifyMethod.c_str());
  TEST_ASSERT_EQUAL_UINT32(1000, cfg.tvVerifyTimeoutMs);
  TEST_ASSERT_EQUAL_UINT8(3, cfg.tvVerifyRetries);
  TEST_ASSERT_EQUAL_UINT16(80, cfg.webPort);
  TEST_ASSERT_FALSE(cfg.webAuthEnabled);
  TEST_ASSERT_EQUAL_STRING("home/insomnia_tv", cfg.mqttTopicRoot.c_str());
  TEST_ASSERT_EQUAL_STRING("insomniatv-esp32", cfg.mqttClientId.c_str());
}

// ---------------------------------------------------------------------------
// Test: Schema validation accepts valid config
// ---------------------------------------------------------------------------
void test_config_validate_ok(void) {
  InsomniaTV::ConfigManager mgr;
  mgr.resetToDefaults();
  std::string err;
  TEST_ASSERT_TRUE(InsomniaTV::ConfigManager::validate(mgr.get(), err));
  TEST_ASSERT_EQUAL_STRING("", err.c_str());
}

// ---------------------------------------------------------------------------
// Test: Validation rejects inactivity_timeout_min = 0
// ---------------------------------------------------------------------------
void test_config_validate_inactivity_zero(void) {
  InsomniaTV::ConfigManager mgr;
  mgr.resetToDefaults();
  InsomniaTV::Config cfg = mgr.get();
  cfg.inactivityTimeoutMin = 0;
  std::string err;
  TEST_ASSERT_FALSE(InsomniaTV::ConfigManager::validate(cfg, err));
  TEST_ASSERT_TRUE(err.length() > 0);
}

// ---------------------------------------------------------------------------
// Test: Validation rejects inactivity_timeout_min > 120
// ---------------------------------------------------------------------------
void test_config_validate_inactivity_too_high(void) {
  InsomniaTV::ConfigManager mgr;
  mgr.resetToDefaults();
  InsomniaTV::Config cfg = mgr.get();
  cfg.inactivityTimeoutMin = 121;
  std::string err;
  TEST_ASSERT_FALSE(InsomniaTV::ConfigManager::validate(cfg, err));
  TEST_ASSERT_TRUE(err.length() > 0);
}

// ---------------------------------------------------------------------------
// Test: Validation rejects volume step = 0
// ---------------------------------------------------------------------------
void test_config_validate_volume_step_zero(void) {
  InsomniaTV::ConfigManager mgr;
  mgr.resetToDefaults();
  InsomniaTV::Config cfg = mgr.get();
  cfg.volumeStepPerRamp = 0;
  std::string err;
  TEST_ASSERT_FALSE(InsomniaTV::ConfigManager::validate(cfg, err));
}

// ---------------------------------------------------------------------------
// Test: Validation rejects volume step = 11
// ---------------------------------------------------------------------------
void test_config_validate_volume_step_eleven(void) {
  InsomniaTV::ConfigManager mgr;
  mgr.resetToDefaults();
  InsomniaTV::Config cfg = mgr.get();
  cfg.volumeStepPerRamp = 11;
  std::string err;
  TEST_ASSERT_FALSE(InsomniaTV::ConfigManager::validate(cfg, err));
}

// ---------------------------------------------------------------------------
// Test: Validation rejects invalid TV verify method
// ---------------------------------------------------------------------------
void test_config_validate_bad_verify_method(void) {
  InsomniaTV::ConfigManager mgr;
  mgr.resetToDefaults();
  InsomniaTV::Config cfg = mgr.get();
  cfg.tvVerifyMethod = "snmp";
  std::string err;
  TEST_ASSERT_FALSE(InsomniaTV::ConfigManager::validate(cfg, err));
  TEST_ASSERT_TRUE(err.find("method") != std::string::npos);
}

// ---------------------------------------------------------------------------
// Test: Validation rejects timeout too low
// ---------------------------------------------------------------------------
void test_config_validate_timeout_too_low(void) {
  InsomniaTV::ConfigManager mgr;
  mgr.resetToDefaults();
  InsomniaTV::Config cfg = mgr.get();
  cfg.tvVerifyTimeoutMs = 100;
  std::string err;
  TEST_ASSERT_FALSE(InsomniaTV::ConfigManager::validate(cfg, err));
}

// ---------------------------------------------------------------------------
// Test: Validation rejects timeout too high
// ---------------------------------------------------------------------------
void test_config_validate_timeout_too_high(void) {
  InsomniaTV::ConfigManager mgr;
  mgr.resetToDefaults();
  InsomniaTV::Config cfg = mgr.get();
  cfg.tvVerifyTimeoutMs = 20000;
  std::string err;
  TEST_ASSERT_FALSE(InsomniaTV::ConfigManager::validate(cfg, err));
}

// ---------------------------------------------------------------------------
// Test: Validation rejects retries = 0
// ---------------------------------------------------------------------------
void test_config_validate_retries_zero(void) {
  InsomniaTV::ConfigManager mgr;
  mgr.resetToDefaults();
  InsomniaTV::Config cfg = mgr.get();
  cfg.tvVerifyRetries = 0;
  std::string err;
  TEST_ASSERT_FALSE(InsomniaTV::ConfigManager::validate(cfg, err));
}

// ---------------------------------------------------------------------------
// Test: Validation rejects retries = 6
// ---------------------------------------------------------------------------
void test_config_validate_retries_six(void) {
  InsomniaTV::ConfigManager mgr;
  mgr.resetToDefaults();
  InsomniaTV::Config cfg = mgr.get();
  cfg.tvVerifyRetries = 6;
  std::string err;
  TEST_ASSERT_FALSE(InsomniaTV::ConfigManager::validate(cfg, err));
}

// ---------------------------------------------------------------------------
// Test: Set/get and change callback
// ---------------------------------------------------------------------------
static bool callbackInvoked = false;
static InsomniaTV::Config capturedOld;
static InsomniaTV::Config capturedNew;

static void onChangeTest(const InsomniaTV::Config& oldCfg,
                         const InsomniaTV::Config& newCfg) {
  callbackInvoked = true;
  capturedOld = oldCfg;
  capturedNew = newCfg;
}

void test_config_change_callback(void) {
  InsomniaTV::ConfigManager mgr;
  mgr.resetToDefaults();
  callbackInvoked = false;

  mgr.onChange(onChangeTest);

  InsomniaTV::Config modified = mgr.get();
  modified.inactivityTimeoutMin = 30;
  mgr.set(modified);

  TEST_ASSERT_TRUE(callbackInvoked);
  TEST_ASSERT_EQUAL_UINT32(15, capturedOld.inactivityTimeoutMin);
  TEST_ASSERT_EQUAL_UINT32(30, capturedNew.inactivityTimeoutMin);
}

// ---------------------------------------------------------------------------
// Test: Load returns Ok (defaults in Phase 1)
// ---------------------------------------------------------------------------
void test_config_load_returns_ok(void) {
  InsomniaTV::ConfigManager mgr;
  InsomniaTV::ConfigStatus status = mgr.load();
  TEST_ASSERT_EQUAL_INT(static_cast<int>(InsomniaTV::ConfigStatus::Ok),
                        static_cast<int>(status));
}

// ---------------------------------------------------------------------------
// Test: Save returns Ok (placeholder in Phase 1)
// ---------------------------------------------------------------------------
void test_config_save_returns_ok(void) {
  InsomniaTV::ConfigManager mgr;
  mgr.resetToDefaults();
  InsomniaTV::ConfigStatus status = mgr.save();
  TEST_ASSERT_EQUAL_INT(static_cast<int>(InsomniaTV::ConfigStatus::Ok),
                        static_cast<int>(status));
}

// ---------------------------------------------------------------------------
// Test: JSON round-trip serialization/de-serialization
// ---------------------------------------------------------------------------
void test_config_json_roundtrip(void) {
  InsomniaTV::ConfigManager mgr;
  InsomniaTV::Config cfg;
  cfg.wifiSsid = "test-ssid";
  cfg.wifiPassword = "test-password";
  cfg.mqttEnabled = false;
  cfg.mqttBroker = "10.0.0.1";
  cfg.mqttPort = 1234;
  cfg.inactivityTimeoutMin = 45;
  cfg.volumeStepPerRamp = 3;
  cfg.tvVerifyMethod = "http";
  cfg.irVolumeUpCode = 0xDEADBEEF;

  // Use the internal parseJson_/toJson_ via a friend or by implementing
  // helper methods if they were public. Since they are private, we'll
  // test them through the ConfigManager's set/get logic if we can,
  // or we just test the public methods.

  // For Phase 1, we'll expose parseJson_/toJson_ as public for testing
  // or use a test-specific subclass.
}

class TestConfigManager : public InsomniaTV::ConfigManager {
public:
  bool testParseJson(const std::string& json, InsomniaTV::Config& out) {
    return parseJson_(json, out);
  }
  std::string testToJson(const InsomniaTV::Config& cfg) {
    return toJson_(cfg);
  }
};

void test_config_json_logic(void) {
  TestConfigManager mgr;
  InsomniaTV::Config cfg;
  cfg.wifiSsid = "test-ssid";
  cfg.wifiPassword = "test-password";
  cfg.mqttEnabled = false;
  cfg.mqttBroker = "10.0.0.1";
  cfg.mqttPort = 1234;
  cfg.mqttClientId = "test-client";
  cfg.mqttTopicRoot = "test/root";
  cfg.mqttUser = "user";
  cfg.mqttPassword = "pass";
  cfg.inactivityTimeoutMin = 45;
  cfg.volumeStepPerRamp = 3;
  cfg.rampIntervalMin = 5;
  cfg.maxRampStepsBeforePoweroff = 20;
  cfg.stayAwake = false;
  cfg.tvVerifyMethod = "http";
  cfg.tvVerifyTarget = "10.0.0.2";
  cfg.tvVerifyTimeoutMs = 2000;
  cfg.tvVerifyRetries = 2;
  cfg.irVolumeUpProtocol = "SONY";
  cfg.irVolumeUpCode = 0x1234;
  cfg.irVolumeUpBits = 12;
  cfg.irVolumeDownProtocol = "SONY";
  cfg.irVolumeDownCode = 0x5678;
  cfg.irVolumeDownBits = 12;
  cfg.irLearnedCodesPath = "/test_learned.json";
  cfg.webPort = 8080;
  cfg.webAuthEnabled = true;

  std::string json = mgr.testToJson(cfg);
  InsomniaTV::Config parsed;
  TEST_ASSERT_TRUE(mgr.testParseJson(json, parsed));

  TEST_ASSERT_EQUAL_STRING(cfg.wifiSsid.c_str(), parsed.wifiSsid.c_str());
  TEST_ASSERT_EQUAL_STRING(cfg.wifiPassword.c_str(), parsed.wifiPassword.c_str());
  TEST_ASSERT_EQUAL_INT(cfg.mqttEnabled, parsed.mqttEnabled);
  TEST_ASSERT_EQUAL_STRING(cfg.mqttBroker.c_str(), parsed.mqttBroker.c_str());
  TEST_ASSERT_EQUAL_UINT16(cfg.mqttPort, parsed.mqttPort);
  TEST_ASSERT_EQUAL_UINT32(cfg.inactivityTimeoutMin, parsed.inactivityTimeoutMin);
  TEST_ASSERT_EQUAL_UINT8(cfg.volumeStepPerRamp, parsed.volumeStepPerRamp);
  TEST_ASSERT_EQUAL_STRING(cfg.tvVerifyMethod.c_str(), parsed.tvVerifyMethod.c_str());
  TEST_ASSERT_EQUAL_UINT64(cfg.irVolumeUpCode, parsed.irVolumeUpCode);
  TEST_ASSERT_EQUAL_UINT16(cfg.webPort, parsed.webPort);
  TEST_ASSERT_EQUAL_INT(cfg.webAuthEnabled, parsed.webAuthEnabled);
}

// ---------------------------------------------------------------------------
// Unity entry point
// ---------------------------------------------------------------------------
int runUnityTests(void) {
  UNITY_BEGIN();
  RUN_TEST(test_config_defaults);
  RUN_TEST(test_config_validate_ok);
  RUN_TEST(test_config_validate_inactivity_zero);
  RUN_TEST(test_config_validate_inactivity_too_high);
  RUN_TEST(test_config_validate_volume_step_zero);
  RUN_TEST(test_config_validate_volume_step_eleven);
  RUN_TEST(test_config_validate_bad_verify_method);
  RUN_TEST(test_config_validate_timeout_too_low);
  RUN_TEST(test_config_validate_timeout_too_high);
  RUN_TEST(test_config_validate_retries_zero);
  RUN_TEST(test_config_validate_retries_six);
  RUN_TEST(test_config_change_callback);
  RUN_TEST(test_config_load_returns_ok);
  RUN_TEST(test_config_save_returns_ok);
  RUN_TEST(test_config_json_logic);
  return UNITY_END();
}

#ifdef INSOMNIATV_NATIVE
int main(void) {
  return runUnityTests();
}
#endif
