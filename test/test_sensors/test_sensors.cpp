// Copyright 2026 insomniaTV Contributors. All rights reserved.

#include <unity.h>

#include <memory>
#include <string>

#include "../../src/sensors/GpioAnalogSensor.h"
#include "../../src/sensors/GpioInputSensor.h"
#include "../../src/sensors/HttpSensor.h"
#include "../../src/sensors/PingSensor.h"
#include "../../src/sensors/SensorManager.h"

using InsomniaTV::GpioAnalogSensor;
using InsomniaTV::GpioInputSensor;
using InsomniaTV::HttpSensor;
using InsomniaTV::PingSensor;
using InsomniaTV::Sensor;
using InsomniaTV::SensorManager;

void test_sensor_manager_registry() {
  auto& mgr = SensorManager::instance();
  mgr.clear();
  auto sensor = std::make_shared<GpioInputSensor>("test_gpio", 5);

  mgr.registerSensor(sensor);

  auto retrieved = mgr.getSensor("test_gpio");
  TEST_ASSERT_NOT_NULL(retrieved.get());
  TEST_ASSERT_EQUAL_STRING("test_gpio", retrieved->getId().c_str());
  TEST_ASSERT_EQUAL_STRING("gpio_input", retrieved->getType().c_str());
}

void test_sensor_manager_lifecycle() {
  auto& mgr = SensorManager::instance();
  mgr.clear();

  auto s1 = std::make_shared<GpioInputSensor>("s1", 1);
  auto s2 = std::make_shared<GpioInputSensor>("s2", 2);

  mgr.registerSensor(s1);
  mgr.registerSensor(s2);
  TEST_ASSERT_EQUAL(2, mgr.listSensors().size());

  mgr.removeSensor("s1");
  TEST_ASSERT_EQUAL(1, mgr.listSensors().size());
  TEST_ASSERT_NULL(mgr.getSensor("s1").get());
  TEST_ASSERT_NOT_NULL(mgr.getSensor("s2").get());

  mgr.clear();
  TEST_ASSERT_EQUAL(0, mgr.listSensors().size());
}

void test_sensor_state_machine() {
  GpioInputSensor sensor("state_test", 10);
  TEST_ASSERT_TRUE(sensor.getState() == Sensor::State::UNINITIALIZED);

  sensor.setState(Sensor::State::READY);
  TEST_ASSERT_TRUE(sensor.isAvailable());

  sensor.setState(Sensor::State::ERROR);
  TEST_ASSERT_FALSE(sensor.isAvailable());
}

void test_gpio_input_sensor() {
  GpioInputSensor sensor("gpio1", 10, true);
  TEST_ASSERT_EQUAL_STRING("gpio1", sensor.getId().c_str());

  JsonDocument cfg = sensor.getConfig();
  TEST_ASSERT_EQUAL(10, cfg["pin"]);
  TEST_ASSERT_TRUE(cfg["pullup"]);
}

void test_gpio_analog_sensor() {
  GpioAnalogSensor sensor("analog1", 34);
  TEST_ASSERT_EQUAL_STRING("analog1", sensor.getId().c_str());
}

void test_ping_sensor() {
  PingSensor sensor("ping1", "192.168.1.1");
  TEST_ASSERT_EQUAL_STRING("ping1", sensor.getId().c_str());
}

void test_http_sensor() {
  HttpSensor sensor("http1", "http://example.com");
  TEST_ASSERT_EQUAL_STRING("http1", sensor.getId().c_str());
}

// ---------------------------------------------------------------------------
// Controllable sensor for testing SensorManager's value cache directly,
// without depending on any real sensor type's native-stub read() behavior.
// ---------------------------------------------------------------------------
class MockCacheSensor : public Sensor {
public:
  MockCacheSensor(const std::string& id, bool value) : id_(id), value_(value) {
    state_ = State::READY;
  }
  std::string getId() const override { return id_; }
  std::string getType() const override { return "mock"; }
  bool read() override { return value_; }
  JsonDocument getConfig() override { return JsonDocument(); }
  void setConfig(const JsonDocument&) override {}
  void setValue(bool v) { value_ = v; }

private:
  std::string id_;
  bool value_;
};

// ---------------------------------------------------------------------------
// Test: getCachedValue() defaults to false for a sensor that's never been
// tick()'d yet, and for an unknown id -- doesn't block, doesn't crash (#75).
// ---------------------------------------------------------------------------
void test_get_cached_value_defaults_false_when_never_read() {
  auto& mgr = SensorManager::instance();
  mgr.clear();
  auto sensor = std::make_shared<MockCacheSensor>("cache_test", true);
  mgr.registerSensor(sensor);

  TEST_ASSERT_FALSE(mgr.getCachedValue("cache_test"));
  TEST_ASSERT_FALSE(mgr.getCachedValue("nonexistent_id"));
}

// ---------------------------------------------------------------------------
// Test: getCachedValue() reflects the value from the last tick(), not a
// fresh live read -- this is exactly what lets /api/sensors and the SSE
// poll task avoid blocking on hardware I/O themselves (#75).
// ---------------------------------------------------------------------------
void test_get_cached_value_reflects_last_tick() {
  auto& mgr = SensorManager::instance();
  mgr.clear();
  auto sensor = std::make_shared<MockCacheSensor>("cache_test2", true);
  mgr.registerSensor(sensor);

  mgr.tick();
  TEST_ASSERT_TRUE(mgr.getCachedValue("cache_test2"));

  sensor->setValue(false);
  TEST_ASSERT_TRUE_MESSAGE(
      mgr.getCachedValue("cache_test2"),
      "cache should still show the old value before the next tick()");

  mgr.tick();
  TEST_ASSERT_FALSE(mgr.getCachedValue("cache_test2"));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_sensor_manager_registry);
  RUN_TEST(test_sensor_manager_lifecycle);
  RUN_TEST(test_sensor_state_machine);
  RUN_TEST(test_gpio_input_sensor);
  RUN_TEST(test_gpio_analog_sensor);
  RUN_TEST(test_ping_sensor);
  RUN_TEST(test_http_sensor);
  RUN_TEST(test_get_cached_value_defaults_false_when_never_read);
  RUN_TEST(test_get_cached_value_reflects_last_tick);
  return UNITY_END();
}
