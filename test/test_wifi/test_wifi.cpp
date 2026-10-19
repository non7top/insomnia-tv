// Copyright 2026 insomniaTV Contributors. All rights reserved.

#include <unity.h>

#include "../../src/network/WifiSetup.h"
#include "../../src/network/WifiWatchdog.h"

using InsomniaTV::WifiSetup;
using InsomniaTV::WifiWatchdog;

void test_wifi_setup_init() {
  WifiSetup ws;
  // Basic instantiation test
  TEST_ASSERT_TRUE(true);
}

static bool isAction(WifiWatchdog::Action got, WifiWatchdog::Action want) {
  return got == want;
}

void test_watchdog_ignores_failures_until_gateway_answered_once() {
  WifiWatchdog wd(4, 3);
  for (int i = 0; i < 50; i++) {
    TEST_ASSERT_TRUE(isAction(wd.onCheck(false), WifiWatchdog::Action::None));
  }
  TEST_ASSERT_FALSE(wd.everReachable());
}

void test_watchdog_rejoins_after_consecutive_failures() {
  WifiWatchdog wd(4, 3);
  wd.onCheck(true);
  for (int i = 0; i < 3; i++) {
    TEST_ASSERT_TRUE(isAction(wd.onCheck(false), WifiWatchdog::Action::None));
  }
  TEST_ASSERT_TRUE(isAction(wd.onCheck(false), WifiWatchdog::Action::Rejoin));
}

void test_watchdog_success_resets_the_failure_count() {
  WifiWatchdog wd(4, 3);
  wd.onCheck(true);
  wd.onCheck(false);
  wd.onCheck(false);
  wd.onCheck(false);
  wd.onCheck(true);
  TEST_ASSERT_EQUAL_UINT8(0, wd.consecutiveFailures());
  for (int i = 0; i < 3; i++) {
    TEST_ASSERT_TRUE(isAction(wd.onCheck(false), WifiWatchdog::Action::None));
  }
}

void test_watchdog_restarts_when_rejoins_do_not_help() {
  WifiWatchdog wd(2, 3);
  wd.onCheck(true);
  for (int rejoin = 0; rejoin < 3; rejoin++) {
    wd.onCheck(false);
    TEST_ASSERT_TRUE(isAction(wd.onCheck(false), WifiWatchdog::Action::Rejoin));
  }
  wd.onCheck(false);
  TEST_ASSERT_TRUE(isAction(wd.onCheck(false), WifiWatchdog::Action::Restart));
}

void test_watchdog_recovery_resets_the_rejoin_budget() {
  WifiWatchdog wd(2, 1);
  wd.onCheck(true);
  wd.onCheck(false);
  TEST_ASSERT_TRUE(isAction(wd.onCheck(false), WifiWatchdog::Action::Rejoin));
  wd.onCheck(true);
  wd.onCheck(false);
  TEST_ASSERT_TRUE(isAction(wd.onCheck(false), WifiWatchdog::Action::Rejoin));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_wifi_setup_init);
  RUN_TEST(test_watchdog_ignores_failures_until_gateway_answered_once);
  RUN_TEST(test_watchdog_rejoins_after_consecutive_failures);
  RUN_TEST(test_watchdog_success_resets_the_failure_count);
  RUN_TEST(test_watchdog_restarts_when_rejoins_do_not_help);
  RUN_TEST(test_watchdog_recovery_resets_the_rejoin_budget);
  return UNITY_END();
}
