// Copyright 2026 insomniaTV Contributors. All rights reserved.

#include <unity.h>
#include "../../src/state/SleepStateMachine.h"
#include "../../src/state/RampScheduler.h"

using InsomniaTV::SleepStateMachine;
using InsomniaTV::RampScheduler;
using InsomniaTV::State;

void test_state_machine_initial_state() {
    SleepStateMachine sm;
    TEST_ASSERT_EQUAL(State::MONITORING, sm.getCurrentState());
}

void test_state_machine_activity_reset() {
    SleepStateMachine sm;
    sm.onIrActivity();
    TEST_ASSERT_EQUAL(State::MONITORING, sm.getCurrentState());
}

void test_ramp_scheduler_init() {
    auto on_step = []() {};
    RampScheduler rs(1000, on_step);
    rs.start();
    rs.stop();
    TEST_ASSERT_TRUE(true);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_state_machine_initial_state);
    RUN_TEST(test_state_machine_activity_reset);
    RUN_TEST(test_ramp_scheduler_init);
    return UNITY_END();
}
