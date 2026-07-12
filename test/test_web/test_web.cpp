// Copyright 2026 insomniaTV Contributors. All rights reserved.

#include <unity.h>

#include <cstdint>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "config/SystemCredentials.h"
#include "web/UpdateCompletion.h"

using InsomniaTV::handleUpdateCompletion;
using InsomniaTV::IRebootSequencer;

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

// Reads an entire file into a string for substring scanning.
static std::string readFile(const std::string& filePath) {
  std::ifstream f(filePath, std::ios::binary);
  std::ostringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

// ---------------------------------------------------------------------------
// Test: the shared system credentials constants are actually non-empty.
// ---------------------------------------------------------------------------
void test_system_credentials_non_empty() {
  TEST_ASSERT_TRUE(InsomniaTV::kSystemUsername[0] != '\0');
  TEST_ASSERT_TRUE(InsomniaTV::kSystemPassword[0] != '\0');
}

// ---------------------------------------------------------------------------
// Regression guard: WebServer.cpp and WifiSetup.cpp must authenticate via
// the shared SystemCredentials constants, not a re-duplicated hardcoded
// literal that can drift out of sync with it (#64).
// ---------------------------------------------------------------------------
void test_no_duplicate_hardcoded_credentials() {
  std::string webServer = readFile(projectRoot() + "/src/web/WebServer.cpp");
  std::string wifiSetup =
      readFile(projectRoot() + "/src/network/WifiSetup.cpp");
  TEST_ASSERT_TRUE_MESSAGE(!webServer.empty(),
                           "Cannot open src/web/WebServer.cpp for inspection");
  TEST_ASSERT_TRUE_MESSAGE(
      !wifiSetup.empty(),
      "Cannot open src/network/WifiSetup.cpp for inspection");

  TEST_ASSERT_EQUAL_INT_MESSAGE(
      static_cast<int>(std::string::npos),
      static_cast<int>(webServer.find("authenticate(\"admin\", \"insomnia\")")),
      "WebServer.cpp has a re-duplicated hardcoded credential literal -- "
      "use kSystemUsername/kSystemPassword instead");
  TEST_ASSERT_EQUAL_INT_MESSAGE(
      static_cast<int>(std::string::npos),
      static_cast<int>(wifiSetup.find("setPassword(\"insomnia\")")),
      "WifiSetup.cpp has a re-duplicated hardcoded credential literal -- "
      "use kSystemPassword instead");
}

// ---------------------------------------------------------------------------
// Records call order and arguments so tests can assert on sequencing, not
// just final state -- this is what makes the #65 regression actually
// catchable (a response-then-restart-with-no-delay reintroduction would
// fail test_update_completion_delays_before_restart_on_success below).
// ---------------------------------------------------------------------------
class MockRebootSequencer : public IRebootSequencer {
public:
  std::vector<std::string> calls;
  uint32_t lastDelayMs = 0;

  void sendResponse(bool ok) override {
    calls.push_back(ok ? "response:ok" : "response:fail");
  }
  void delayMs(uint32_t ms) override {
    lastDelayMs = ms;
    calls.push_back("delay");
  }
  void restart() override { calls.push_back("restart"); }
};

// ---------------------------------------------------------------------------
// Test: on a successful update, the response is sent, then a real delay
// happens, then the device restarts -- in that order (#65).
// ---------------------------------------------------------------------------
void test_update_completion_delays_before_restart_on_success() {
  MockRebootSequencer seq;
  handleUpdateCompletion(true, seq);

  TEST_ASSERT_EQUAL(3, static_cast<int>(seq.calls.size()));
  TEST_ASSERT_EQUAL_STRING("response:ok", seq.calls[0].c_str());
  TEST_ASSERT_EQUAL_STRING("delay", seq.calls[1].c_str());
  TEST_ASSERT_EQUAL_STRING("restart", seq.calls[2].c_str());
  // Catches a regression back to a too-short/zero delay, not just a missing
  // call -- the original bug was the response never reaching the client
  // before the connection was torn down.
  TEST_ASSERT_TRUE(seq.lastDelayMs >= 500);
}

// ---------------------------------------------------------------------------
// Test: on a failed update, only the failure response is sent -- no delay,
// no restart.
// ---------------------------------------------------------------------------
void test_update_completion_no_restart_on_failure() {
  MockRebootSequencer seq;
  handleUpdateCompletion(false, seq);

  TEST_ASSERT_EQUAL(1, static_cast<int>(seq.calls.size()));
  TEST_ASSERT_EQUAL_STRING("response:fail", seq.calls[0].c_str());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_webserver_no_curly_quotes);
  RUN_TEST(test_system_credentials_non_empty);
  RUN_TEST(test_no_duplicate_hardcoded_credentials);
  RUN_TEST(test_update_completion_delays_before_restart_on_success);
  RUN_TEST(test_update_completion_no_restart_on_failure);
  return UNITY_END();
}
