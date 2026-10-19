// Copyright 2026 insomniaTV Contributors. All rights reserved.

#include <unity.h>

#include <vector>

#include "../../src/network/WifiSetup.h"
#include "../../src/network/WifiWatchdog.h"

using InsomniaTV::ApCandidate;
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

static ApCandidate makeAp(const char* ssid, uint8_t lastByte, int rssi) {
  ApCandidate ap;
  ap.ssid = ssid;
  for (int i = 0; i < 5; i++) {
    ap.bssid[i] = 0x04;
  }
  ap.bssid[5] = lastByte;
  ap.rssi = rssi;
  ap.channel = 1;
  return ap;
}

void test_select_ap_skips_the_failed_bssid_even_if_strongest() {
  std::vector<ApCandidate> aps = {makeAp("home", 0xE6, -40),
                                  makeAp("home", 0x56, -60)};
  int pick = InsomniaTV::selectAccessPoint(aps, "home", aps[0].bssid);
  TEST_ASSERT_EQUAL_INT(1, pick);
}

void test_select_ap_picks_the_strongest_remaining() {
  std::vector<ApCandidate> aps = {makeAp("home", 0x01, -70),
                                  makeAp("home", 0x02, -50),
                                  makeAp("home", 0x03, -60)};
  uint8_t none[6] = {0};
  TEST_ASSERT_EQUAL_INT(1, InsomniaTV::selectAccessPoint(aps, "home", none));
}

void test_select_ap_ignores_other_ssids() {
  std::vector<ApCandidate> aps = {makeAp("neighbour", 0x01, -30),
                                  makeAp("home", 0x02, -80)};
  uint8_t none[6] = {0};
  TEST_ASSERT_EQUAL_INT(1, InsomniaTV::selectAccessPoint(aps, "home", none));
}

void test_select_ap_returns_minus_one_when_only_the_failed_one_exists() {
  std::vector<ApCandidate> aps = {makeAp("home", 0xE6, -40)};
  TEST_ASSERT_EQUAL_INT(
      -1, InsomniaTV::selectAccessPoint(aps, "home", aps[0].bssid));
  TEST_ASSERT_EQUAL_INT(
      -1, InsomniaTV::selectAccessPoint({}, "home", aps[0].bssid));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_wifi_setup_init);
  RUN_TEST(test_watchdog_ignores_failures_until_gateway_answered_once);
  RUN_TEST(test_watchdog_rejoins_after_consecutive_failures);
  RUN_TEST(test_watchdog_success_resets_the_failure_count);
  RUN_TEST(test_watchdog_restarts_when_rejoins_do_not_help);
  RUN_TEST(test_watchdog_recovery_resets_the_rejoin_budget);
  RUN_TEST(test_select_ap_skips_the_failed_bssid_even_if_strongest);
  RUN_TEST(test_select_ap_picks_the_strongest_remaining);
  RUN_TEST(test_select_ap_ignores_other_ssids);
  RUN_TEST(test_select_ap_returns_minus_one_when_only_the_failed_one_exists);
  return UNITY_END();
}
