// Copyright 2026 insomniaTV Contributors. All rights reserved.
//
// Integration tests: config → SensorManager → TvStateMachine pipeline.
// Covers the weight-derivation logic (main.cpp boot sequence) and the
// multi-component wiring between the three subsystems.

#include <ArduinoJson.h>
#include <unity.h>

#include <cstring>
#include <memory>
#include <string>

#include "../../src/discovery/SamsungTvDiscovery.h"
#include "../../src/sensors/SensorManager.h"
#include "../../src/tv/TvStateMachine.h"

using InsomniaTV::SamsungTvDiscovery;
using InsomniaTV::SensorManager;
using InsomniaTV::TvStateMachine;

// ---------------------------------------------------------------------------
// Controllable sensor — reused from test_tv, redefined here to keep suites
// independent.
// ---------------------------------------------------------------------------
class MockSensor : public InsomniaTV::Sensor {
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

// ---------------------------------------------------------------------------
// Replicates the main.cpp boot-time weight-derivation logic:
//   upnp=4, ping=3, everything else=2.
// Having the same logic here lets us assert what the boot sequence produces
// without calling main.cpp (guarded by #ifndef UNIT_TEST).
// ---------------------------------------------------------------------------
static JsonDocument buildTvDoc(const std::string& sensorsJson) {
  JsonDocument sensorsDoc;
  deserializeJson(sensorsDoc, sensorsJson);
  JsonDocument tvDoc;
  tvDoc["hysteresis_count"] = 2;
  JsonArray detArr = tvDoc["detection_sensors"].to<JsonArray>();
  for (JsonObject s : sensorsDoc.as<JsonArray>()) {
    const char* t = s["type"] | "";
    int w = (strcmp(t, "upnp") == 0) ? 4 : (strcmp(t, "ping") == 0) ? 3 : 2;
    JsonObject ds = detArr.add<JsonObject>();
    ds["sensor_id"] = s["id"] | "";
    ds["weight"] = w;
    ds["enabled"] = true;
  }
  return tvDoc;
}

// ── Weight-derivation unit tests ────────────────────────────────────────────

void test_upnp_weight_is_4(void) {
  JsonDocument doc = buildTvDoc(R"([{"id":"s","type":"upnp"}])");
  TEST_ASSERT_EQUAL_INT(4, doc["detection_sensors"][0]["weight"].as<int>());
}

void test_ping_weight_is_3(void) {
  JsonDocument doc = buildTvDoc(R"([{"id":"s","type":"ping"}])");
  TEST_ASSERT_EQUAL_INT(3, doc["detection_sensors"][0]["weight"].as<int>());
}

void test_http_weight_is_2(void) {
  JsonDocument doc = buildTvDoc(R"([{"id":"s","type":"http"}])");
  TEST_ASSERT_EQUAL_INT(2, doc["detection_sensors"][0]["weight"].as<int>());
}

void test_unknown_type_weight_is_2(void) {
  JsonDocument doc = buildTvDoc(R"([{"id":"s","type":"gpio_input","pin":4}])");
  TEST_ASSERT_EQUAL_INT(2, doc["detection_sensors"][0]["weight"].as<int>());
}

void test_all_sensors_enabled_by_default(void) {
  JsonDocument doc =
      buildTvDoc(R"([{"id":"a","type":"ping"},{"id":"b","type":"upnp"}])");
  JsonArray arr = doc["detection_sensors"];
  TEST_ASSERT_EQUAL(2, arr.size());
  TEST_ASSERT_TRUE(arr[0]["enabled"].as<bool>());
  TEST_ASSERT_TRUE(arr[1]["enabled"].as<bool>());
}

void test_empty_sensors_json_yields_no_detection_sensors(void) {
  JsonDocument doc = buildTvDoc("[]");
  TEST_ASSERT_EQUAL(0, doc["detection_sensors"].as<JsonArray>().size());
}

void test_hysteresis_count_is_2(void) {
  JsonDocument doc = buildTvDoc("[]");
  TEST_ASSERT_EQUAL_INT(2, doc["hysteresis_count"].as<int>());
}

// ── SensorManager::init() integration tests ─────────────────────────────────

void test_sensor_manager_init_registers_sensors(void) {
  SamsungTvDiscovery discovery;
  SensorManager::instance().clear();

  const char* json =
      R"([{"id":"tv_ping","type":"ping","target_ip":"192.168.1.1"},)"
      R"({"id":"tv_http","type":"http","url":"http://tv.local/api/v2/"}])";

  SensorManager::instance().init(json, discovery);

  TEST_ASSERT_NOT_NULL(SensorManager::instance().getSensor("tv_ping").get());
  TEST_ASSERT_NOT_NULL(SensorManager::instance().getSensor("tv_http").get());
  TEST_ASSERT_NULL(SensorManager::instance().getSensor("nonexistent").get());
  TEST_ASSERT_EQUAL(2, SensorManager::instance().listSensors().size());
}

void test_sensor_manager_reinit_replaces_sensors(void) {
  SamsungTvDiscovery discovery;
  SensorManager::instance().clear();

  SensorManager::instance().init(
      R"([{"id":"old","type":"ping","target_ip":"1.1.1.1"}])", discovery);
  TEST_ASSERT_NOT_NULL(SensorManager::instance().getSensor("old").get());

  SensorManager::instance().init(
      R"([{"id":"new","type":"http","url":"http://tv.local"}])", discovery);
  TEST_ASSERT_NULL(SensorManager::instance().getSensor("old").get());
  TEST_ASSERT_NOT_NULL(SensorManager::instance().getSensor("new").get());
}

// ── Full pipeline integration test ──────────────────────────────────────────
// Wires SensorManager + TvStateMachine together using the derived config and
// verifies that sensor weights flow through to TvSM contributions correctly.

void test_full_pipeline_weights_flow_to_tv_sm(void) {
  SensorManager::instance().clear();

  // Register mock sensors with known, controllable readings (all ON).
  SensorManager::instance().registerSensor(
      std::make_shared<MockSensor>("tv_ping", "ping", true));
  SensorManager::instance().registerSensor(
      std::make_shared<MockSensor>("tv_upnp", "upnp", true));
  SensorManager::instance().registerSensor(
      std::make_shared<MockSensor>("tv_http", "http", true));

  // Build the derived tvDoc (same logic as main.cpp boot sequence).
  const char* sensorsJson = R"([{"id":"tv_ping","type":"ping"},)"
                            R"({"id":"tv_upnp","type":"upnp"},)"
                            R"({"id":"tv_http","type":"http"}])";
  JsonDocument tvDoc = buildTvDoc(sensorsJson);

  TvStateMachine tvSm(SensorManager::instance());
  tvSm.begin(tvDoc);
  // TvStateMachine is push-based: it subscribed to SensorManager's
  // value-change events in its constructor, so ticking the manager (not
  // the state machine) is what drives sensor reads into it.
  SensorManager::instance().tick();

  auto contribs = tvSm.getContributions();
  TEST_ASSERT_EQUAL(3, contribs.size());

  // All sensors read true → weightedVote == +weight.
  bool checkedPing = false, checkedUpnp = false, checkedHttp = false;
  for (const auto& c : contribs) {
    if (c.sensorId == "tv_ping") {
      TEST_ASSERT_EQUAL_INT(3, c.weightedVote);
      checkedPing = true;
    } else if (c.sensorId == "tv_upnp") {
      TEST_ASSERT_EQUAL_INT(4, c.weightedVote);
      checkedUpnp = true;
    } else if (c.sensorId == "tv_http") {
      TEST_ASSERT_EQUAL_INT(2, c.weightedVote);
      checkedHttp = true;
    }
  }
  TEST_ASSERT_TRUE(checkedPing && checkedUpnp && checkedHttp);
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------
int main(void) {
  UNITY_BEGIN();

  RUN_TEST(test_upnp_weight_is_4);
  RUN_TEST(test_ping_weight_is_3);
  RUN_TEST(test_http_weight_is_2);
  RUN_TEST(test_unknown_type_weight_is_2);
  RUN_TEST(test_all_sensors_enabled_by_default);
  RUN_TEST(test_empty_sensors_json_yields_no_detection_sensors);
  RUN_TEST(test_hysteresis_count_is_2);

  RUN_TEST(test_sensor_manager_init_registers_sensors);
  RUN_TEST(test_sensor_manager_reinit_replaces_sensors);

  RUN_TEST(test_full_pipeline_weights_flow_to_tv_sm);

  return UNITY_END();
}
