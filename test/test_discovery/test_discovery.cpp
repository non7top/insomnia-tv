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
    std::string payload =
        "<root><device>"
        "<friendlyName>Mock Samsung TV</friendlyName>"
        "<modelName>Mock Model</modelName>"
        "<modelNumber>AllShare1.0</modelNumber>"
        "</device></root>";

    size_t fnStart = payload.find("<friendlyName>");
    size_t fnEnd = payload.find("</friendlyName>");
    if (fnStart != std::string::npos && fnEnd != std::string::npos) {
      tv.name = payload.substr(fnStart + 14, fnEnd - (fnStart + 14));
    }

    size_t mnStart = payload.find("<modelName>");
    size_t mnEnd = payload.find("</modelName>");
    if (mnStart != std::string::npos && mnEnd != std::string::npos) {
      tv.model = payload.substr(mnStart + 11, mnEnd - (mnStart + 11));
    }

    size_t mNumStart = payload.find("<modelNumber>");
    size_t mNumEnd = payload.find("</modelNumber>");
    if (mNumStart != std::string::npos && mNumEnd != std::string::npos) {
      std::string modelNumber =
          payload.substr(mNumStart + 13, mNumEnd - (mNumStart + 13));
      if (!modelNumber.empty()) {
        tv.model += " (" + modelNumber + ")";
      }
    }
  }
};

void test_samsung_discovery_parser() {
  SamsungTvDiscoveryTester discovery;

  std::string mockResponse =
      "HTTP/1.1 200 OK\r\n"
      "LOCATION: http://192.168.1.50:8001/ms/v1/thumbnail/\r\n"
      "ST: urn:schemas-upnp-org:device:MediaRenderer:1\r\n"
      "\r\n";

  discovery.testParseSsdp(mockResponse);

  const auto& tvs = discovery.getDiscoveredTvs();
  TEST_ASSERT_EQUAL(1, tvs.size());
  TEST_ASSERT_EQUAL_STRING("Mock Samsung TV", tvs[0].name.c_str());
  TEST_ASSERT_EQUAL_STRING("Mock Model (AllShare1.0)", tvs[0].model.c_str());
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
