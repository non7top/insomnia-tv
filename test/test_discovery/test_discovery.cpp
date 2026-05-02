// Copyright 2026 insomniaTV Contributors. All rights reserved.

#include <unity.h>

#include <string>

#include "../../src/discovery/SamsungTvDiscovery.h"


using InsomniaTV::SamsungTvDiscovery;
using InsomniaTV::SamsungTvInfo;

class SamsungTvDiscoveryTester : public SamsungTvDiscovery {
public:
  void testParseSsdp(const std::string& response) {
    parseSsdpResponse(response);
  }

  // Override to avoid network call in unit test
  void fetchDeviceMetadata(SamsungTvInfo& tv) override {
    tv.name = "Mock Samsung TV";
    tv.model = "Mock Model";
  }
};

void test_samsung_discovery_parser() {
  SamsungTvDiscoveryTester discovery;

  std::string mockResponse =
      "HTTP/1.1 200 OK\r\n"
      "LOCATION: http://192.168.1.50:8001/ms/v1/thumbnail/\r\n"
      "ST: urn:samsung.com:device:RemoteControlReceiver:1\r\n"
      "\r\n";

  discovery.testParseSsdp(mockResponse);

  const auto& tvs = discovery.getDiscoveredTvs();
  TEST_ASSERT_EQUAL(1, tvs.size());
  TEST_ASSERT_EQUAL_STRING("Mock Samsung TV", tvs[0].name.c_str());
  TEST_ASSERT_EQUAL_STRING("192.168.1.50", tvs[0].ip.c_str());
}

void test_samsung_discovery_integration() {
  // This test expects a real network environment (like Docker host mode)
  // and a mock TV running.
  SamsungTvDiscovery discovery;

  discovery.scan();

  const auto& tvs = discovery.getDiscoveredTvs();
  // In a real scan, we might find something
  if (tvs.size() > 0) {
    TEST_ASSERT_NOT_EQUAL(0, tvs[0].name.length());
    TEST_ASSERT_NOT_EQUAL(0, tvs[0].ip.length());
  }
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_samsung_discovery_parser);
  RUN_TEST(test_samsung_discovery_integration);
  return UNITY_END();
}
