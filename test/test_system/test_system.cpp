// Copyright 2026 insomniaTV Contributors. All rights reserved.

#include <unity.h>
#include <vector>
#include "../../src/system/EventBus.h"
#include "../../src/system/SystemStateManager.h"

using InsomniaTV::Activity;
using InsomniaTV::EventBus;
using InsomniaTV::SystemEvent;
using InsomniaTV::SystemStateManager;

void setUp() {
    EventBus::instance().clear();
}

void test_event_bus_subscription() {
    bool event_received = false;
    EventBus::instance().subscribe([&event_received](SystemEvent e) {
        if (e == SystemEvent::SYSTEM_IDLE) {
            event_received = true;
        }
    });
    EventBus::instance().publish(SystemEvent::SYSTEM_IDLE);
    TEST_ASSERT_TRUE(event_received);
}

void test_system_state_manager_update() {
    SystemStateManager ssm;

    EventBus::instance().publish(SystemEvent::IR_ACTIVITY);
    TEST_ASSERT_EQUAL(Activity::HIGH, ssm.getState().activity);

    EventBus::instance().publish(SystemEvent::SYSTEM_IDLE);
    TEST_ASSERT_EQUAL(Activity::IDLE, ssm.getState().activity);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_event_bus_subscription);
    RUN_TEST(test_system_state_manager_update);
    return UNITY_END();
}
