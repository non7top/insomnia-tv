// Copyright 2026 insomniaTV Contributors. All rights reserved.

#include <unity.h>

#include "../../src/network/WifiSetup.h"

using InsomniaTV::WifiSetup;

void test_wifi_setup_init() {
  WifiSetup ws;
  // Basic instantiation test
  TEST_ASSERT_TRUE(true);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_wifi_setup_init);
  return UNITY_END();
}
