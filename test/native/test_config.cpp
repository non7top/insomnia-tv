#include <unity.h>
#include "config/ConfigManager.h"

using namespace InsomniaTV;

// ---------------------------------------------------------------------------
// Test: Default config values are set correctly
// ---------------------------------------------------------------------------
void test_config_defaults(void) {
    ConfigManager mgr;
    mgr.resetToDefaults();
    const Config& cfg = mgr.get();

    TEST_ASSERT_FALSE(cfg.mqttEnabled == false);  // default is true
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
    ConfigManager mgr;
    mgr.resetToDefaults();
    String err;
    TEST_ASSERT_TRUE(ConfigManager::validate(mgr.get(), err));
    TEST_ASSERT_EQUAL_STRING("", err.c_str());
}

// ---------------------------------------------------------------------------
// Test: Validation rejects inactivity_timeout_min out of range
// ---------------------------------------------------------------------------
void test_config_validate_inactivity_zero(void) {
    ConfigManager mgr;
    mgr.resetToDefaults();
    Config cfg = mgr.get();
    cfg.inactivityTimeoutMin = 0;
    String err;
    TEST_ASSERT_FALSE(ConfigManager::validate(cfg, err));
    TEST_ASSERT_TRUE(err.length() > 0);
}

void test_config_validate_inactivity_too_high(void) {
    ConfigManager mgr;
    mgr.resetToDefaults();
    Config cfg = mgr.get();
    cfg.inactivityTimeoutMin = 121;
    String err;
    TEST_ASSERT_FALSE(ConfigManager::validate(cfg, err));
    TEST_ASSERT_TRUE(err.length() > 0);
}

// ---------------------------------------------------------------------------
// Test: Validation rejects invalid volume step
// ---------------------------------------------------------------------------
void test_config_validate_volume_step_zero(void) {
    ConfigManager mgr;
    mgr.resetToDefaults();
    Config cfg = mgr.get();
    cfg.volumeStepPerRamp = 0;
    String err;
    TEST_ASSERT_FALSE(ConfigManager::validate(cfg, err));
}

void test_config_validate_volume_step_eleven(void) {
    ConfigManager mgr;
    mgr.resetToDefaults();
    Config cfg = mgr.get();
    cfg.volumeStepPerRamp = 11;
    String err;
    TEST_ASSERT_FALSE(ConfigManager::validate(cfg, err));
}

// ---------------------------------------------------------------------------
// Test: Validation rejects invalid TV verify method
// ---------------------------------------------------------------------------
void test_config_validate_bad_verify_method(void) {
    ConfigManager mgr;
    mgr.resetToDefaults();
    Config cfg = mgr.get();
    cfg.tvVerifyMethod = "snmp";
    String err;
    TEST_ASSERT_FALSE(ConfigManager::validate(cfg, err));
    TEST_ASSERT_TRUE(err.indexOf("method") >= 0);
}

// ---------------------------------------------------------------------------
// Test: Validation rejects invalid timeout range
// ---------------------------------------------------------------------------
void test_config_validate_timeout_too_low(void) {
    ConfigManager mgr;
    mgr.resetToDefaults();
    Config cfg = mgr.get();
    cfg.tvVerifyTimeoutMs = 100;
    String err;
    TEST_ASSERT_FALSE(ConfigManager::validate(cfg, err));
}

void test_config_validate_timeout_too_high(void) {
    ConfigManager mgr;
    mgr.resetToDefaults();
    Config cfg = mgr.get();
    cfg.tvVerifyTimeoutMs = 20000;
    String err;
    TEST_ASSERT_FALSE(ConfigManager::validate(cfg, err));
}

// ---------------------------------------------------------------------------
// Test: Validation rejects invalid retries
// ---------------------------------------------------------------------------
void test_config_validate_retries_zero(void) {
    ConfigManager mgr;
    mgr.resetToDefaults();
    Config cfg = mgr.get();
    cfg.tvVerifyRetries = 0;
    String err;
    TEST_ASSERT_FALSE(ConfigManager::validate(cfg, err));
}

void test_config_validate_retries_six(void) {
    ConfigManager mgr;
    mgr.resetToDefaults();
    Config cfg = mgr.get();
    cfg.tvVerifyRetries = 6;
    String err;
    TEST_ASSERT_FALSE(ConfigManager::validate(cfg, err));
}

// ---------------------------------------------------------------------------
// Test: Set/get and change callback
// ---------------------------------------------------------------------------
static bool callbackInvoked = false;
static Config capturedOld;
static Config capturedNew;

void onChangeTest(const Config& oldCfg, const Config& newCfg) {
    callbackInvoked = true;
    capturedOld = oldCfg;
    capturedNew = newCfg;
}

void test_config_change_callback(void) {
    ConfigManager mgr;
    mgr.resetToDefaults();
    callbackInvoked = false;

    mgr.onChange(onChangeTest);

    Config modified = mgr.get();
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
    ConfigManager mgr;
    ConfigStatus status = mgr.load();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ConfigStatus::Ok), static_cast<int>(status));
}

// ---------------------------------------------------------------------------
// Test: Save returns Ok (placeholder in Phase 1)
// ---------------------------------------------------------------------------
void test_config_save_returns_ok(void) {
    ConfigManager mgr;
    mgr.resetToDefaults();
    ConfigStatus status = mgr.save();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ConfigStatus::Ok), static_cast<int>(status));
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
    return UNITY_END();
}

#ifdef INSOMNIATV_NATIVE
int main(void) {
    return runUnityTests();
}
#endif
