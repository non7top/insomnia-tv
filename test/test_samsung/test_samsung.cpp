// Copyright 2026 insomniaTV Contributors. All rights reserved.

#include <unity.h>

#include <string>
#include <vector>

#include "../../src/tv/SamsungTvController.h"
#include "../../src/tv/SamsungTvProtocol.h"

using InsomniaTV::ISamsungTvTransport;
using InsomniaTV::PairState;
using InsomniaTV::PowerOffResult;
using InsomniaTV::SamsungPower;
using InsomniaTV::SamsungTvController;
using InsomniaTV::SamsungWsEvent;
using InsomniaTV::SamsungWsResult;

static const char kInfoOn[] =
    "{\"device\":{\"PowerState\":\"on\",\"wifiMac\":\"AA:BB:CC:DD:EE:FF\"}}";
static const char kInfoStandby[] = "{\"device\":{\"PowerState\":\"standby\"}}";

class MockTransport : public ISamsungTvTransport {
public:
  std::string info;
  SamsungWsResult ws;
  std::vector<std::string> keys;
  std::vector<std::string> tokens;
  std::vector<uint32_t> timeouts;
  int infoCalls = 0;

  std::string fetchDeviceInfo(const std::string&) override {
    infoCalls++;
    return info;
  }
  SamsungWsResult sendKey(const std::string&, const std::string& token,
                          const std::string& key, uint32_t timeoutMs) override {
    keys.push_back(key);
    tokens.push_back(token);
    timeouts.push_back(timeoutMs);
    return ws;
  }
};

// ---------------------------------------------------------------------------
// Protocol helpers
// ---------------------------------------------------------------------------
void test_base64_known_vectors(void) {
  TEST_ASSERT_EQUAL_STRING("", InsomniaTV::base64Encode("").c_str());
  TEST_ASSERT_EQUAL_STRING("Zg==", InsomniaTV::base64Encode("f").c_str());
  TEST_ASSERT_EQUAL_STRING("Zm8=", InsomniaTV::base64Encode("fo").c_str());
  TEST_ASSERT_EQUAL_STRING("Zm9v", InsomniaTV::base64Encode("foo").c_str());
  TEST_ASSERT_EQUAL_STRING("aW5zb21uaWFUVg==",
                           InsomniaTV::base64Encode("insomniaTV").c_str());
}

void test_remote_path_without_token(void) {
  TEST_ASSERT_EQUAL_STRING(
      "/api/v2/channels/samsung.remote.control?name=aW5zb21uaWFUVg==",
      InsomniaTV::samsungRemotePath("insomniaTV", "").c_str());
}

void test_remote_path_with_token(void) {
  TEST_ASSERT_EQUAL_STRING(
      "/api/v2/channels/samsung.remote.control?name=aW5zb21uaWFUVg=="
      "&token=16008142",
      InsomniaTV::samsungRemotePath("insomniaTV", "16008142").c_str());
}

void test_remote_path_escapes_base64_symbols(void) {
  // "???" encodes to "Pz8/", which contains '/'.
  std::string path = InsomniaTV::samsungRemotePath("???", "");
  TEST_ASSERT_TRUE(path.find("name=Pz8%2F") != std::string::npos);
}

void test_key_payload(void) {
  std::string p = InsomniaTV::samsungKeyPayload("KEY_POWER");
  TEST_ASSERT_TRUE(p.find("\"method\":\"ms.remote.control\"") !=
                   std::string::npos);
  TEST_ASSERT_TRUE(p.find("\"DataOfCmd\":\"KEY_POWER\"") != std::string::npos);
  TEST_ASSERT_TRUE(p.find("\"Cmd\":\"Click\"") != std::string::npos);
  TEST_ASSERT_TRUE(p.find("\"TypeOfRemote\":\"SendRemoteKey\"") !=
                   std::string::npos);
}

void test_ws_event_token_in_data(void) {
  SamsungWsEvent ev;
  TEST_ASSERT_TRUE(InsomniaTV::parseSamsungWsEvent(
      "{\"event\":\"ms.channel.connect\",\"data\":{\"token\":\"123\"}}", ev));
  TEST_ASSERT_EQUAL_STRING("ms.channel.connect", ev.event.c_str());
  TEST_ASSERT_EQUAL_STRING("123", ev.token.c_str());
}

void test_ws_event_numeric_token(void) {
  SamsungWsEvent ev;
  TEST_ASSERT_TRUE(InsomniaTV::parseSamsungWsEvent(
      "{\"event\":\"ms.channel.connect\",\"data\":{\"token\":16008142}}", ev));
  TEST_ASSERT_EQUAL_STRING("16008142", ev.token.c_str());
}

void test_ws_event_token_in_client_attributes(void) {
  SamsungWsEvent ev;
  TEST_ASSERT_TRUE(InsomniaTV::parseSamsungWsEvent(
      "{\"event\":\"ms.channel.connect\",\"data\":{\"clients\":["
      "{\"attributes\":{\"name\":\"x\",\"token\":\"777\"}}]}}",
      ev));
  TEST_ASSERT_EQUAL_STRING("777", ev.token.c_str());
}

void test_ws_event_without_token(void) {
  SamsungWsEvent ev;
  TEST_ASSERT_TRUE(InsomniaTV::parseSamsungWsEvent(
      "{\"event\":\"ms.channel.timeOut\"}", ev));
  TEST_ASSERT_EQUAL_STRING("ms.channel.timeOut", ev.event.c_str());
  TEST_ASSERT_EQUAL_STRING("", ev.token.c_str());
}

void test_ws_event_rejects_garbage(void) {
  SamsungWsEvent ev;
  TEST_ASSERT_FALSE(InsomniaTV::parseSamsungWsEvent("not json", ev));
  TEST_ASSERT_FALSE(InsomniaTV::parseSamsungWsEvent("{\"a\":1}", ev));
}

void test_parse_power_state(void) {
  TEST_ASSERT_EQUAL(static_cast<int>(SamsungPower::On),
                    static_cast<int>(InsomniaTV::parseSamsungPower(kInfoOn)));
  TEST_ASSERT_EQUAL(
      static_cast<int>(SamsungPower::Standby),
      static_cast<int>(InsomniaTV::parseSamsungPower(kInfoStandby)));
  TEST_ASSERT_EQUAL(static_cast<int>(SamsungPower::Unknown),
                    static_cast<int>(InsomniaTV::parseSamsungPower("")));
  TEST_ASSERT_EQUAL(static_cast<int>(SamsungPower::Unknown),
                    static_cast<int>(InsomniaTV::parseSamsungPower("{}")));
}

void test_parse_wifi_mac(void) {
  TEST_ASSERT_EQUAL_STRING("AA:BB:CC:DD:EE:FF",
                           InsomniaTV::parseSamsungWifiMac(kInfoOn).c_str());
  TEST_ASSERT_EQUAL_STRING(
      "", InsomniaTV::parseSamsungWifiMac(kInfoStandby).c_str());
}

// ---------------------------------------------------------------------------
// Controller: pairing
// ---------------------------------------------------------------------------
void test_pair_success_stores_and_reports_credentials(void) {
  MockTransport t;
  t.info = kInfoOn;
  t.ws.status = SamsungWsResult::Status::Ok;
  t.ws.token = "16008142";
  SamsungTvController tv(t);

  std::string cbHost, cbToken, cbMac;
  tv.setCredentialsCallback(
      [&](const std::string& h, const std::string& tk, const std::string& m) {
        cbHost = h;
        cbToken = tk;
        cbMac = m;
      });

  TEST_ASSERT_TRUE(tv.pair("192.168.2.190"));
  TEST_ASSERT_TRUE(tv.isPaired());
  TEST_ASSERT_EQUAL(static_cast<int>(PairState::Paired),
                    static_cast<int>(tv.pairState()));
  TEST_ASSERT_EQUAL_STRING("192.168.2.190", cbHost.c_str());
  TEST_ASSERT_EQUAL_STRING("16008142", cbToken.c_str());
  TEST_ASSERT_EQUAL_STRING("AA:BB:CC:DD:EE:FF", cbMac.c_str());
  // Pairing connects without a token and presses no key.
  TEST_ASSERT_EQUAL_STRING("", t.tokens[0].c_str());
  TEST_ASSERT_EQUAL_STRING("", t.keys[0].c_str());
  TEST_ASSERT_EQUAL_UINT32(SamsungTvController::kPairTimeoutMs, t.timeouts[0]);
}

void test_pair_prompt_not_accepted(void) {
  MockTransport t;
  t.ws.status = SamsungWsResult::Status::TimedOut;
  SamsungTvController tv(t);

  TEST_ASSERT_FALSE(tv.pair("192.168.2.190"));
  TEST_ASSERT_FALSE(tv.isPaired());
  TEST_ASSERT_EQUAL(static_cast<int>(PairState::Failed),
                    static_cast<int>(tv.pairState()));
  TEST_ASSERT_TRUE(tv.lastError().find("prompt") != std::string::npos);
}

void test_pair_connect_without_token_fails(void) {
  MockTransport t;
  t.ws.status = SamsungWsResult::Status::Ok;
  SamsungTvController tv(t);

  TEST_ASSERT_FALSE(tv.pair("192.168.2.190"));
  TEST_ASSERT_FALSE(tv.isPaired());
}

void test_pair_unreachable(void) {
  MockTransport t;
  t.ws.status = SamsungWsResult::Status::Unreachable;
  SamsungTvController tv(t);

  TEST_ASSERT_FALSE(tv.pair("192.168.2.190"));
  TEST_ASSERT_TRUE(tv.lastError().find("unreachable") != std::string::npos);
}

void test_pair_empty_host_rejected_without_io(void) {
  MockTransport t;
  SamsungTvController tv(t);
  TEST_ASSERT_FALSE(tv.pair(""));
  TEST_ASSERT_EQUAL_INT(0, static_cast<int>(t.keys.size()));
}

void test_configure_with_token_is_paired(void) {
  MockTransport t;
  SamsungTvController tv(t);
  tv.configure("192.168.2.190", "tok", "");
  TEST_ASSERT_TRUE(tv.isPaired());
  tv.configure("192.168.2.190", "", "");
  TEST_ASSERT_FALSE(tv.isPaired());
}

// ---------------------------------------------------------------------------
// Controller: power-off
// ---------------------------------------------------------------------------
void test_power_off_sends_key_power_when_on(void) {
  MockTransport t;
  t.info = kInfoOn;
  t.ws.status = SamsungWsResult::Status::Ok;
  SamsungTvController tv(t);
  tv.configure("192.168.2.190", "tok", "");

  TEST_ASSERT_EQUAL(static_cast<int>(PowerOffResult::Sent),
                    static_cast<int>(tv.powerOff()));
  TEST_ASSERT_EQUAL_INT(1, static_cast<int>(t.keys.size()));
  TEST_ASSERT_EQUAL_STRING("KEY_POWER", t.keys[0].c_str());
  TEST_ASSERT_EQUAL_STRING("tok", t.tokens[0].c_str());
}

void test_power_off_skips_when_standby(void) {
  MockTransport t;
  t.info = kInfoStandby;
  SamsungTvController tv(t);
  tv.configure("192.168.2.190", "tok", "");

  TEST_ASSERT_EQUAL(static_cast<int>(PowerOffResult::AlreadyOff),
                    static_cast<int>(tv.powerOff()));
  // KEY_POWER toggles: sending it in standby would switch the TV back on.
  TEST_ASSERT_EQUAL_INT(0, static_cast<int>(t.keys.size()));
}

void test_power_off_skips_when_state_unknown(void) {
  MockTransport t;
  t.info = "{\"device\":{}}";
  SamsungTvController tv(t);
  tv.configure("192.168.2.190", "tok", "");

  TEST_ASSERT_EQUAL(static_cast<int>(PowerOffResult::StateUnknown),
                    static_cast<int>(tv.powerOff()));
  TEST_ASSERT_EQUAL_INT(0, static_cast<int>(t.keys.size()));
}

void test_power_off_unreachable_tv(void) {
  MockTransport t;
  t.info = "";
  SamsungTvController tv(t);
  tv.configure("192.168.2.190", "tok", "");

  TEST_ASSERT_EQUAL(static_cast<int>(PowerOffResult::Unreachable),
                    static_cast<int>(tv.powerOff()));
  TEST_ASSERT_EQUAL_INT(0, static_cast<int>(t.keys.size()));
}

void test_power_off_not_paired(void) {
  MockTransport t;
  t.info = kInfoOn;
  SamsungTvController tv(t);
  TEST_ASSERT_EQUAL(static_cast<int>(PowerOffResult::NotPaired),
                    static_cast<int>(tv.powerOff()));
  TEST_ASSERT_EQUAL_INT(0, t.infoCalls);
}

void test_power_off_rejected_token(void) {
  MockTransport t;
  t.info = kInfoOn;
  t.ws.status = SamsungWsResult::Status::Unauthorized;
  SamsungTvController tv(t);
  tv.configure("192.168.2.190", "stale", "");

  TEST_ASSERT_EQUAL(static_cast<int>(PowerOffResult::Rejected),
                    static_cast<int>(tv.powerOff()));
}

void test_power_off_websocket_failure(void) {
  MockTransport t;
  t.info = kInfoOn;
  t.ws.status = SamsungWsResult::Status::Unreachable;
  SamsungTvController tv(t);
  tv.configure("192.168.2.190", "tok", "");

  TEST_ASSERT_EQUAL(static_cast<int>(PowerOffResult::Unreachable),
                    static_cast<int>(tv.powerOff()));
}

void test_query_power(void) {
  MockTransport t;
  t.info = kInfoStandby;
  SamsungTvController tv(t);
  tv.configure("192.168.2.190", "tok", "");
  TEST_ASSERT_EQUAL(static_cast<int>(SamsungPower::Standby),
                    static_cast<int>(tv.queryPower()));
}

int runUnityTests(void) {
  UNITY_BEGIN();
  RUN_TEST(test_base64_known_vectors);
  RUN_TEST(test_remote_path_without_token);
  RUN_TEST(test_remote_path_with_token);
  RUN_TEST(test_remote_path_escapes_base64_symbols);
  RUN_TEST(test_key_payload);
  RUN_TEST(test_ws_event_token_in_data);
  RUN_TEST(test_ws_event_numeric_token);
  RUN_TEST(test_ws_event_token_in_client_attributes);
  RUN_TEST(test_ws_event_without_token);
  RUN_TEST(test_ws_event_rejects_garbage);
  RUN_TEST(test_parse_power_state);
  RUN_TEST(test_parse_wifi_mac);
  RUN_TEST(test_pair_success_stores_and_reports_credentials);
  RUN_TEST(test_pair_prompt_not_accepted);
  RUN_TEST(test_pair_connect_without_token_fails);
  RUN_TEST(test_pair_unreachable);
  RUN_TEST(test_pair_empty_host_rejected_without_io);
  RUN_TEST(test_configure_with_token_is_paired);
  RUN_TEST(test_power_off_sends_key_power_when_on);
  RUN_TEST(test_power_off_skips_when_standby);
  RUN_TEST(test_power_off_skips_when_state_unknown);
  RUN_TEST(test_power_off_unreachable_tv);
  RUN_TEST(test_power_off_not_paired);
  RUN_TEST(test_power_off_rejected_token);
  RUN_TEST(test_power_off_websocket_failure);
  RUN_TEST(test_query_power);
  return UNITY_END();
}

#ifdef INSOMNIATV_NATIVE
int main(void) {
  return runUnityTests();
}
#endif
