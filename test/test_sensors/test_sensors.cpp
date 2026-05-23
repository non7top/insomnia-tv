// Copyright 2026 insomniaTV Contributors. All rights reserved.

#include <unity.h>
#include <memory>
#include "../../src/sensors/SensorManager.h"
#include "../../src/sensors/GpioInputSensor.h"
#include "../../src/sensors/GpioAnalogSensor.h"
#include "../../src/sensors/PingSensor.h"
#include "../../src/sensors/HttpSensor.h"

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

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_sensor_manager_registry);
    RUN_TEST(test_sensor_manager_lifecycle);
    RUN_TEST(test_sensor_state_machine);
    RUN_TEST(test_gpio_input_sensor);
    RUN_TEST(test_gpio_analog_sensor);
    RUN_TEST(test_ping_sensor);
    RUN_TEST(test_http_sensor);
    return UNITY_END();
}
