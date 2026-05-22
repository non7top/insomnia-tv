// Copyright 2026 insomniaTV Contributors. All rights reserved.

#include <unity.h>

#include <memory>
#include <string>

#include "../../src/hal/IClock.h"
#include "../../src/sensors/SensorManager.h"
#include "../../src/state/SleepStateMachine.h"
#include "../../src/tv/TvStateMachine.h"

using InsomniaTV::IClock;
using InsomniaTV::Sensor;
using InsomniaTV::SensorManager;
using InsomniaTV::SleepStateMachine;
using InsomniaTV::TvStateMachine;

// ---------------------------------------------------------------------------
// Controllable sensor mock
// ---------------------------------------------------------------------------
class MockSensor : public Sensor {
 public:
  MockSensor(const std::string& id, const std::string& type, bool value = false)
      : id_(id), type_(type), value_(value) {
    state_ = State::READY;
  }

  std::string getId() const override { return id_; }
  std::string getType() const override { return type_; }
  bool read() override { return value_; }
  JsonDocument getConfig() override { return JsonDocument(); }
  void setConfig(const JsonDocument&) override {}

  void setValue(bool v) { value_ = v; }

 private:
  std::string id_;
  std::string type_;
  bool value_;
};

// Helper: build TvStateMachine config from a list of {id, weight, enabled}.
static JsonDocument makeConfig(
    int hysteresis,
    std::initializer_list<std::tuple<const char*, int, bool>> sensors) {
  JsonDocument doc;
  doc["hysteresis_count"] = hysteresis;
  JsonArray arr = doc["detection_sensors"].to<JsonArray>();
  for (auto& [id, w, en] : sensors) {
    JsonObject o = arr.add<JsonObject>();
    o["sensor_id"] = id;
    o["weight"] = w;
    o["enabled"] = en;
  }
  return doc;
}

// ---------------------------------------------------------------------------
// Initial state is UNKNOWN
// ---------------------------------------------------------------------------
void test_tv_sm_initial_state() {
  SensorManager& mgr = SensorManager::instance();
  mgr.clear();
  TvStateMachine sm(mgr);
  TEST_ASSERT_EQUAL(TvStateMachine::PowerState::UNKNOWN, sm.getPowerState());
}

// ---------------------------------------------------------------------------
// Single sensor ON → confidence = +10, after 2 updates → ON
// ---------------------------------------------------------------------------
void test_tv_sm_single_sensor_on() {
  SensorManager& mgr = SensorManager::instance();
  mgr.clear();
  TvStateMachine sm(mgr, /*hysteresis=*/2);

  JsonDocument cfg = makeConfig(2, {{"s1", 3, true}});
  sm.begin(cfg);

  // Simulate two consecutive ON readings (hysteresis = 2)
  sm.onSensorUpdate("s1", true);
  TEST_ASSERT_EQUAL(TvStateMachine::PowerState::UNKNOWN, sm.getPowerState());
  sm.onSensorUpdate("s1", true);
  TEST_ASSERT_EQUAL(TvStateMachine::PowerState::ON, sm.getPowerState());
}

// ---------------------------------------------------------------------------
// Single sensor OFF → confidence = -10, after 2 updates → OFF
// ---------------------------------------------------------------------------
void test_tv_sm_single_sensor_off() {
  SensorManager& mgr = SensorManager::instance();
  mgr.clear();
  TvStateMachine sm(mgr, 2);

  JsonDocument cfg = makeConfig(2, {{"s1", 3, true}});
  sm.begin(cfg);

  sm.onSensorUpdate("s1", false);
  sm.onSensorUpdate("s1", false);
  TEST_ASSERT_EQUAL(TvStateMachine::PowerState::OFF, sm.getPowerState());
}

// ---------------------------------------------------------------------------
// Weighted fusion: two ON sensors outweigh one OFF sensor
// ---------------------------------------------------------------------------
void test_tv_sm_weighted_fusion_on_wins() {
  SensorManager& mgr = SensorManager::instance();
  mgr.clear();
  TvStateMachine sm(mgr, 1);  // hysteresis=1 for simplicity

  // s1 weight=3 ON (+3), s2 weight=1 OFF (-1) → net=+2, confidence=+5 → ON
  JsonDocument cfg = makeConfig(1, {{"s1", 3, true}, {"s2", 1, true}});
  sm.begin(cfg);

  sm.onSensorUpdate("s1", true);
  sm.onSensorUpdate("s2", false);
  TEST_ASSERT_EQUAL(TvStateMachine::PowerState::ON, sm.getPowerState());
}

// ---------------------------------------------------------------------------
// Balanced sensors → UNKNOWN (confidence in -3..+3)
// ---------------------------------------------------------------------------
void test_tv_sm_balanced_sensors_unknown() {
  SensorManager& mgr = SensorManager::instance();
  mgr.clear();
  TvStateMachine sm(mgr, 1);

  // equal weight ON + OFF → confidence = 0 → UNKNOWN
  JsonDocument cfg = makeConfig(1, {{"s1", 2, true}, {"s2", 2, true}});
  sm.begin(cfg);

  sm.onSensorUpdate("s1", true);
  sm.onSensorUpdate("s2", false);
  TEST_ASSERT_EQUAL(TvStateMachine::PowerState::UNKNOWN, sm.getPowerState());
}

// ---------------------------------------------------------------------------
// Hysteresis: inconsistent readings prevent state change
// ---------------------------------------------------------------------------
void test_tv_sm_hysteresis_prevents_flapping() {
  SensorManager& mgr = SensorManager::instance();
  mgr.clear();
  TvStateMachine sm(mgr, /*hysteresis=*/3);

  JsonDocument cfg = makeConfig(3, {{"s1", 5, true}});
  sm.begin(cfg);

  // Two ON readings — not enough (need 3)
  sm.onSensorUpdate("s1", true);
  sm.onSensorUpdate("s1", true);
  TEST_ASSERT_EQUAL(TvStateMachine::PowerState::UNKNOWN, sm.getPowerState());

  // Break the streak with an OFF, then two more ON — reset counter
  sm.onSensorUpdate("s1", false);
  sm.onSensorUpdate("s1", true);
  sm.onSensorUpdate("s1", true);
  TEST_ASSERT_EQUAL(TvStateMachine::PowerState::UNKNOWN, sm.getPowerState());

  // Third consecutive ON → state changes
  sm.onSensorUpdate("s1", true);
  TEST_ASSERT_EQUAL(TvStateMachine::PowerState::ON, sm.getPowerState());
}

// ---------------------------------------------------------------------------
// Subscriber callback fires exactly once per state transition
// ---------------------------------------------------------------------------
void test_tv_sm_subscriber_callback() {
  SensorManager& mgr = SensorManager::instance();
  mgr.clear();
  TvStateMachine sm(mgr, 1);

  int callbackCount = 0;
  TvStateMachine::PowerState lastState = TvStateMachine::PowerState::UNKNOWN;
  sm.subscribe([&](TvStateMachine::PowerState ps) {
    callbackCount++;
    lastState = ps;
  });

  JsonDocument cfg = makeConfig(1, {{"s1", 4, true}});
  sm.begin(cfg);

  sm.onSensorUpdate("s1", true);
  TEST_ASSERT_EQUAL(1, callbackCount);
  TEST_ASSERT_EQUAL(TvStateMachine::PowerState::ON, lastState);

  // Same vote again — no additional callback
  sm.onSensorUpdate("s1", true);
  TEST_ASSERT_EQUAL(1, callbackCount);

  // Transition to OFF
  sm.onSensorUpdate("s1", false);
  TEST_ASSERT_EQUAL(2, callbackCount);
  TEST_ASSERT_EQUAL(TvStateMachine::PowerState::OFF, lastState);
}

// ---------------------------------------------------------------------------
// getContributions() reflects each sensor's current vote
// ---------------------------------------------------------------------------
void test_tv_sm_get_contributions() {
  SensorManager& mgr = SensorManager::instance();
  mgr.clear();
  TvStateMachine sm(mgr, 1);

  JsonDocument cfg =
      makeConfig(1, {{"ping1", 3, true}, {"upnp1", 5, false}});
  sm.begin(cfg);

  sm.onSensorUpdate("ping1", true);

  auto contribs = sm.getContributions();
  TEST_ASSERT_EQUAL(2, contribs.size());

  // ping1: enabled, available, ON → vote = +3
  TEST_ASSERT_EQUAL_STRING("ping1", contribs[0].sensorId.c_str());
  TEST_ASSERT_TRUE(contribs[0].enabled);
  TEST_ASSERT_TRUE(contribs[0].available);
  TEST_ASSERT_TRUE(contribs[0].rawValue);
  TEST_ASSERT_EQUAL_INT(3, contribs[0].weightedVote);

  // upnp1: disabled → vote = 0
  TEST_ASSERT_EQUAL_STRING("upnp1", contribs[1].sensorId.c_str());
  TEST_ASSERT_FALSE(contribs[1].enabled);
  TEST_ASSERT_EQUAL_INT(0, contribs[1].weightedVote);
}

// ---------------------------------------------------------------------------
// sendPowerCommand() fires callback and sets TRANSITIONING
// ---------------------------------------------------------------------------
void test_tv_sm_send_power_command() {
  SensorManager& mgr = SensorManager::instance();
  mgr.clear();
  TvStateMachine sm(mgr, 1);

  bool commandSent = false;
  bool commandValue = false;
  sm.setPowerCommandCallback([&](bool on) {
    commandSent = true;
    commandValue = on;
  });

  TEST_ASSERT_TRUE(sm.sendPowerCommand(false));
  TEST_ASSERT_TRUE(commandSent);
  TEST_ASSERT_FALSE(commandValue);
  TEST_ASSERT_EQUAL(TvStateMachine::PowerState::TRANSITIONING,
                    sm.getPowerState());
}

// ---------------------------------------------------------------------------
// sendPowerCommand() returns false when no callback is set
// ---------------------------------------------------------------------------
void test_tv_sm_send_power_command_no_callback() {
  SensorManager& mgr = SensorManager::instance();
  mgr.clear();
  TvStateMachine sm(mgr, 1);
  TEST_ASSERT_FALSE(sm.sendPowerCommand(true));
}

// ---------------------------------------------------------------------------
// Disabled sensor does not contribute to confidence
// ---------------------------------------------------------------------------
void test_tv_sm_disabled_sensor_excluded() {
  SensorManager& mgr = SensorManager::instance();
  mgr.clear();
  TvStateMachine sm(mgr, 1);

  // Only disabled sensor → maxVotes=0 → confidence=0 → UNKNOWN
  JsonDocument cfg = makeConfig(1, {{"s1", 5, false}});
  sm.begin(cfg);

  sm.onSensorUpdate("s1", true);
  TEST_ASSERT_EQUAL(TvStateMachine::PowerState::UNKNOWN, sm.getPowerState());
}

// ---------------------------------------------------------------------------
// SleepStateMachine: VERIFYING with TV ON → power-off fires
// ---------------------------------------------------------------------------
void test_sleep_sm_verifying_tv_on_fires_poweroff() {
  // Use SleepStateMachine + TvStateMachine together
  // TvStateMachine is in ON state; SleepSM should send power-off.
  SensorManager& mgr = SensorManager::instance();
  mgr.clear();
  TvStateMachine tvSm(mgr, 1);

  JsonDocument cfg = makeConfig(1, {{"s1", 5, true}});
  tvSm.begin(cfg);
  tvSm.onSensorUpdate("s1", true);  // TV is ON

  // Now wire into SleepStateMachine
  class SimpleClock : public IClock {
   public:
    uint32_t nowMs() const override { return ms_; }
    void advanceMs(uint32_t ms) override { ms_ += ms; }
    uint32_t ms_ = 0;
  } clk;

  SleepStateMachine sleepSm(clk, 1000);
  sleepSm.setTvStateMachine(&tvSm);

  int powerOffCalls = 0;
  sleepSm.setPowerOffCallback([&] { powerOffCalls++; });

  clk.advanceMs(2000);
  sleepSm.tick();          // MONITORING → RAMPING
  sleepSm.onRampComplete(); // RAMPING → VERIFYING
  sleepSm.tick();          // VERIFYING → POWERING_OFF → MONITORING

  TEST_ASSERT_EQUAL(1, powerOffCalls);
  TEST_ASSERT_EQUAL(InsomniaTV::State::MONITORING, sleepSm.getCurrentState());
}

// ---------------------------------------------------------------------------
// SleepStateMachine: VERIFYING with TV already OFF → no power-off sent
// ---------------------------------------------------------------------------
void test_sleep_sm_verifying_tv_already_off() {
  SensorManager& mgr = SensorManager::instance();
  mgr.clear();
  TvStateMachine tvSm(mgr, 1);

  JsonDocument cfg = makeConfig(1, {{"s1", 5, true}});
  tvSm.begin(cfg);
  tvSm.onSensorUpdate("s1", false);  // TV is OFF

  class SimpleClock : public IClock {
   public:
    uint32_t nowMs() const override { return ms_; }
    void advanceMs(uint32_t ms) override { ms_ += ms; }
    uint32_t ms_ = 0;
  } clk;

  SleepStateMachine sleepSm(clk, 1000);
  sleepSm.setTvStateMachine(&tvSm);

  int powerOffCalls = 0;
  sleepSm.setPowerOffCallback([&] { powerOffCalls++; });

  clk.advanceMs(2000);
  sleepSm.tick();
  sleepSm.onRampComplete();
  sleepSm.tick();  // VERIFYING → TV already OFF → MONITORING, no power-off

  TEST_ASSERT_EQUAL(0, powerOffCalls);
  TEST_ASSERT_EQUAL(InsomniaTV::State::MONITORING, sleepSm.getCurrentState());
}

// ---------------------------------------------------------------------------
// SleepStateMachine: VERIFYING with TV UNKNOWN → FALLBACK_OFF fires
// ---------------------------------------------------------------------------
void test_sleep_sm_verifying_tv_unknown_fallback() {
  SensorManager& mgr = SensorManager::instance();
  mgr.clear();
  TvStateMachine tvSm(mgr, 99);  // hysteresis so high it stays UNKNOWN

  JsonDocument cfg = makeConfig(99, {{"s1", 5, true}});
  tvSm.begin(cfg);

  class SimpleClock : public IClock {
   public:
    uint32_t nowMs() const override { return ms_; }
    void advanceMs(uint32_t ms) override { ms_ += ms; }
    uint32_t ms_ = 0;
  } clk;

  SleepStateMachine sleepSm(clk, 1000);
  sleepSm.setTvStateMachine(&tvSm);

  int powerOffCalls = 0;
  sleepSm.setPowerOffCallback([&] { powerOffCalls++; });

  clk.advanceMs(2000);
  sleepSm.tick();
  sleepSm.onRampComplete();
  sleepSm.tick();  // VERIFYING → UNKNOWN → FALLBACK_OFF → fires callback

  TEST_ASSERT_EQUAL(1, powerOffCalls);
  TEST_ASSERT_EQUAL(InsomniaTV::State::MONITORING, sleepSm.getCurrentState());
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------
int main() {
  UNITY_BEGIN();
  RUN_TEST(test_tv_sm_initial_state);
  RUN_TEST(test_tv_sm_single_sensor_on);
  RUN_TEST(test_tv_sm_single_sensor_off);
  RUN_TEST(test_tv_sm_weighted_fusion_on_wins);
  RUN_TEST(test_tv_sm_balanced_sensors_unknown);
  RUN_TEST(test_tv_sm_hysteresis_prevents_flapping);
  RUN_TEST(test_tv_sm_subscriber_callback);
  RUN_TEST(test_tv_sm_get_contributions);
  RUN_TEST(test_tv_sm_send_power_command);
  RUN_TEST(test_tv_sm_send_power_command_no_callback);
  RUN_TEST(test_tv_sm_disabled_sensor_excluded);
  RUN_TEST(test_sleep_sm_verifying_tv_on_fires_poweroff);
  RUN_TEST(test_sleep_sm_verifying_tv_already_off);
  RUN_TEST(test_sleep_sm_verifying_tv_unknown_fallback);
  return UNITY_END();
}
