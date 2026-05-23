// Copyright 2026 insomniaTV Contributors. All rights reserved.

#include <unity.h>

#include <cstdint>
#include <fstream>
#include <string>

// Returns the project root by stripping everything from "/test/" in __FILE__.
static std::string projectRoot() {
  std::string path(__FILE__);
  auto pos = path.rfind("/test/");
  return (pos != std::string::npos) ? path.substr(0, pos) : ".";
}

// Scans a binary file for UTF-8 curly/smart quote sequences:
//   U+201C (left ")  = 0xE2 0x80 0x9C
//   U+201D (right ") = 0xE2 0x80 0x9D
// Returns true if any are found.
static bool hasCurlyQuotes(const std::string& filePath) {
  std::ifstream f(filePath, std::ios::binary);
  if (!f.is_open())
    return false;
  uint8_t b0 = 0, b1 = 0;
  char ch;
  while (f.get(ch)) {
    uint8_t b2 = static_cast<uint8_t>(ch);
    if (b0 == 0xE2 && b1 == 0x80 && (b2 == 0x9C || b2 == 0x9D)) {
      return true;
    }
    b0 = b1;
    b1 = b2;
  }
  return false;
}

// ---------------------------------------------------------------------------
// Curly/smart quotes (U+201C/U+201D) inside JS string literals embedded in
// C++ raw strings silently break HTML attribute parsing in the browser.
// This test catches the regression before it reaches firmware.
// ---------------------------------------------------------------------------
void test_webserver_no_curly_quotes() {
  std::string path = projectRoot() + "/src/web/WebServer.cpp";
  std::ifstream probe(path, std::ios::binary);
  TEST_ASSERT_TRUE_MESSAGE(probe.is_open(),
                           "Cannot open src/web/WebServer.cpp for inspection");
  probe.close();

  TEST_ASSERT_FALSE_MESSAGE(
      hasCurlyQuotes(path),
      "Curly/smart quotes (U+201C/U+201D) found in WebServer.cpp "
      "-- they corrupt HTML attribute values in embedded JS strings");
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_webserver_no_curly_quotes);
  return UNITY_END();
}
