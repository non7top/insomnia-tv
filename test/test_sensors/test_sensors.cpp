// Copyright 2026 insomniaTV Contributors. All rights reserved.

#include <unity.h>
#include <memory>
#include "../../src/sensors/SensorManager.h"
#include "../../src/sensors/GpioInputSensor.h"
#include "../../src/sensors/GpioAnalogSensor.h"
#include "../../src/sensors/PingSensor.h"
#include "../../src/sensors/HttpSensor.h"

using namespace InsomniaTV;

void test_sensor_manager_registry() {
    auto& mgr = SensorManager::instance();
    auto sensor = std::make_shared<GpioInputSensor>("test_gpio", 5);

    mgr.registerSensor(sensor);

    auto retrieved = mgr.getSensor("test_gpio");
    TEST_ASSERT_NOT_NULL(retrieved.get());
    TEST_ASSERT_EQUAL_STRING("test_gpio", retrieved->getId().c_str());
    TEST_ASSERT_EQUAL_STRING("gpio_input", retrieved->getType().c_str());
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
    RUN_TEST(test_gpio_input_sensor);
    RUN_TEST(test_gpio_analog_sensor);
    RUN_TEST(test_ping_sensor);
    RUN_TEST(test_http_sensor);
    return UNITY_END();
}
