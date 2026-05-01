// Copyright 2026 insomniaTV Contributors. All rights reserved.

#include <unity.h>
#include <thread>
#include <chrono>
#include <string>
#include "../../src/ir/ActivityTracker.h"
#include "../../src/ir/IrDriver.h"

using InsomniaTV::ActivityTracker;
using InsomniaTV::IrDriver;

void delay_ms(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void test_activity_tracker_initial_state() {
    ActivityTracker tracker(1000);
    TEST_ASSERT_LESS_OR_EQUAL(50, tracker.msSinceLastActivity());
}

void test_activity_tracker_record_updates_time() {
    ActivityTracker tracker(1000);
    delay_ms(100);
    tracker.record("NEC", 0x1234, 32);
    TEST_ASSERT_LESS_OR_EQUAL(50, tracker.msSinceLastActivity());
}

void test_activity_tracker_reset() {
    ActivityTracker tracker(1000);
    tracker.record("NEC", 0x1234, 32);
    tracker.reset();
    TEST_ASSERT_LESS_OR_EQUAL(50, tracker.msSinceLastActivity());
}

void test_ir_driver_basic() {
    IrDriver driver(1, 2);
    driver.begin();
    TEST_ASSERT_EQUAL(false, driver.hasDecoded());
    TEST_ASSERT_EQUAL(0, driver.lastCode());
    TEST_ASSERT_EQUAL(0, driver.lastBits());
    TEST_ASSERT_EQUAL_STRING("", driver.lastProtocol().c_str());
}

void test_ir_driver_send_receive() {
    IrDriver driver(1, 2);
    bool success = driver.send("NEC", 0x5678, 32);
    TEST_ASSERT_EQUAL(true, success);
    driver.receive();
    uint16_t len = 0;
    uint16_t* raw = driver.learn_raw(len);
    TEST_ASSERT_EQUAL(nullptr, raw);
    TEST_ASSERT_EQUAL(0, len);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_activity_tracker_initial_state);
    RUN_TEST(test_activity_tracker_record_updates_time);
    RUN_TEST(test_activity_tracker_reset);
    RUN_TEST(test_ir_driver_basic);
    RUN_TEST(test_ir_driver_send_receive);
    return UNITY_END();
}
