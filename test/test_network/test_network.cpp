// Copyright 2026 insomniaTV Contributors. All rights reserved.

#include <unity.h>
#include "../../src/network/NetworkChecker.h"

using InsomniaTV::NetworkChecker;

void test_network_checker_init() {
    NetworkChecker nc;
    nc.setTimeout(500);
    // Since WiFi is mocked or unavailable in native tests,
    // isConnected() should reflect the state accurately.
    TEST_ASSERT_EQUAL(false, nc.isConnected());
}

void test_network_checker_operations() {
    NetworkChecker nc;
    // Test base operations. Given no hardware access,
    // these should return failure.
    TEST_ASSERT_EQUAL(-1, nc.ping("192.168.1.1"));
    TEST_ASSERT_EQUAL(-1, nc.httpGet("http://example.com"));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_network_checker_init);
    RUN_TEST(test_network_checker_operations);
    return UNITY_END();
}
