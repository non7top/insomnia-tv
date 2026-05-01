#include <unity.h>
#include <thread>
#include <chrono>
#include "../../src/ir/ActivityTracker.h"
#include "../../src/ir/IrDriver.h"

using namespace InsomniaTV;

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

void test_ir_driver_basic() {
    IrDriver driver(1, 2);
    driver.begin();
    TEST_ASSERT_EQUAL(false, driver.hasDecoded());
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_activity_tracker_initial_state);
    RUN_TEST(test_activity_tracker_record_updates_time);
    RUN_TEST(test_ir_driver_basic);
    return UNITY_END();
}
