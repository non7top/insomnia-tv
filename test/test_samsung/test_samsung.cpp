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
  int volume = -1;  // returned by getVolume; -1 = unreadable
  std::vector<int> volumeSets;
  bool setVolumeOk = true;

  std::string fetchDeviceInfo(const std::string&) override {
    infoCalls++;
    return info;
  }
  int getVolume(const std::string&) override { return volume; }
  bool setVolume(const std::string&, int v) override {
    volumeSets.push_back(v);
    return setVolumeOk;
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

void test_volume_down_presses_the_key_once_per_step(void) {
  MockTransport t;
  t.ws.status = SamsungWsResult::Status::Ok;
  SamsungTvController tv(t);
  tv.configure("192.168.2.190", "tok", "");

  TEST_ASSERT_TRUE(tv.volumeDown(3));
  TEST_ASSERT_EQUAL_INT(3, static_cast<int>(t.keys.size()));
  for (size_t i = 0; i < t.keys.size(); i++) {
    TEST_ASSERT_EQUAL_STRING("KEY_VOLDOWN", t.keys[i].c_str());
    TEST_ASSERT_EQUAL_STRING("tok", t.tokens[i].c_str());
  }
}

void test_volume_down_not_paired_sends_nothing(void) {
  MockTransport t;
  SamsungTvController tv(t);
  TEST_ASSERT_FALSE(tv.volumeDown(1));
  TEST_ASSERT_EQUAL_INT(0, static_cast<int>(t.keys.size()));
}

void test_volume_down_stops_at_the_first_failed_press(void) {
  MockTransport t;
  t.ws.status = SamsungWsResult::Status::Unreachable;
  SamsungTvController tv(t);
  tv.configure("192.168.2.190", "tok", "");

  TEST_ASSERT_FALSE(tv.volumeDown(5));
  TEST_ASSERT_EQUAL_INT(1, static_cast<int>(t.keys.size()));
}

void test_power_off_still_works_after_volume_steps(void) {
  MockTransport t;
  t.info = kInfoOn;
  t.ws.status = SamsungWsResult::Status::Ok;
  SamsungTvController tv(t);
  tv.configure("192.168.2.190", "tok", "");

  TEST_ASSERT_TRUE(tv.volumeDown(2));
  TEST_ASSERT_EQUAL(static_cast<int>(PowerOffResult::Sent),
                    static_cast<int>(tv.powerOff()));
  TEST_ASSERT_EQUAL_STRING("KEY_POWER", t.keys.back().c_str());
}

void test_volume_soap_get_has_no_desired_volume(void) {
  std::string x = InsomniaTV::samsungVolumeSoap("GetVolume", 7);
  TEST_ASSERT_TRUE(x.find("<u:GetVolume ") != std::string::npos);
  TEST_ASSERT_TRUE(x.find("RenderingControl:1") != std::string::npos);
  TEST_ASSERT_TRUE(x.find("<Channel>Master</Channel>") != std::string::npos);
  TEST_ASSERT_TRUE(x.find("DesiredVolume") == std::string::npos);
}

void test_volume_soap_set_carries_the_volume(void) {
  std::string x = InsomniaTV::samsungVolumeSoap("SetVolume", 12);
  TEST_ASSERT_TRUE(x.find("<u:SetVolume ") != std::string::npos);
  TEST_ASSERT_TRUE(x.find("<DesiredVolume>12</DesiredVolume>") !=
                   std::string::npos);
  TEST_ASSERT_TRUE(x.find("</u:SetVolume>") != std::string::npos);
}

void test_parse_volume(void) {
  TEST_ASSERT_EQUAL_INT(
      28, InsomniaTV::parseSamsungVolume(
              "<u:GetVolumeResponse><CurrentVolume>28</CurrentVolume>"
              "</u:GetVolumeResponse>"));
  TEST_ASSERT_EQUAL_INT(
      0, InsomniaTV::parseSamsungVolume("<CurrentVolume>0</CurrentVolume>"));
  TEST_ASSERT_EQUAL_INT(-1, InsomniaTV::parseSamsungVolume(""));
  TEST_ASSERT_EQUAL_INT(
      -1, InsomniaTV::parseSamsungVolume("<CurrentVolume>x</CurrentVolume>"));
  TEST_ASSERT_EQUAL_INT(
      -1, InsomniaTV::parseSamsungVolume("<CurrentVolume></CurrentVolume>"));
}

void test_ramp_sets_exact_volumes_ending_at_zero(void) {
  MockTransport t;
  t.volume = 30;
  SamsungTvController tv(t);
  tv.configure("192.168.2.190", "tok", "");

  for (uint8_t step = 1; step <= 3; step++) {
    TEST_ASSERT_TRUE(tv.rampStep(step, 3, 25));
  }
  TEST_ASSERT_EQUAL_INT(3, static_cast<int>(t.volumeSets.size()));
  TEST_ASSERT_EQUAL_INT(20, t.volumeSets[0]);
  TEST_ASSERT_EQUAL_INT(10, t.volumeSets[1]);
  TEST_ASSERT_EQUAL_INT(0, t.volumeSets[2]);
  TEST_ASSERT_EQUAL_INT(0, static_cast<int>(t.keys.size()));
}

void test_ramp_rereads_the_volume_on_each_new_ramp(void) {
  MockTransport t;
  SamsungTvController tv(t);
  tv.configure("192.168.2.190", "tok", "");

  t.volume = 20;
  tv.rampStep(1, 2, 25);
  t.volume = 40;
  tv.rampStep(1, 2, 25);
  TEST_ASSERT_EQUAL_INT(10, t.volumeSets[0]);
  TEST_ASSERT_EQUAL_INT(20, t.volumeSets[1]);
}

void test_ramp_falls_back_to_spread_key_presses(void) {
  MockTransport t;
  t.volume = -1;
  t.ws.status = SamsungWsResult::Status::Ok;
  SamsungTvController tv(t);
  tv.configure("192.168.2.190", "tok", "");

  // 25 clicks over 3 steps: 8 + 8 + 9, plus a margin on the last step.
  TEST_ASSERT_TRUE(tv.rampStep(1, 3, 25));
  TEST_ASSERT_EQUAL_INT(8, static_cast<int>(t.keys.size()));
  TEST_ASSERT_TRUE(tv.rampStep(2, 3, 25));
  TEST_ASSERT_EQUAL_INT(16, static_cast<int>(t.keys.size()));
  TEST_ASSERT_TRUE(tv.rampStep(3, 3, 25));
  TEST_ASSERT_EQUAL_INT(25 + SamsungTvController::kLastStepExtraPresses,
                        static_cast<int>(t.keys.size()));
  TEST_ASSERT_EQUAL_INT(0, static_cast<int>(t.volumeSets.size()));
}

void test_ramp_not_paired_does_nothing(void) {
  MockTransport t;
  t.volume = 30;
  SamsungTvController tv(t);
  TEST_ASSERT_FALSE(tv.rampStep(1, 3, 25));
  TEST_ASSERT_EQUAL_INT(0, static_cast<int>(t.volumeSets.size()));
}

void test_ramp_reports_a_failed_set(void) {
  MockTransport t;
  t.volume = 30;
  t.setVolumeOk = false;
  SamsungTvController tv(t);
  tv.configure("192.168.2.190", "tok", "");
  TEST_ASSERT_FALSE(tv.rampStep(1, 3, 25));
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
  RUN_TEST(test_volume_down_presses_the_key_once_per_step);
  RUN_TEST(test_volume_down_not_paired_sends_nothing);
  RUN_TEST(test_volume_down_stops_at_the_first_failed_press);
  RUN_TEST(test_power_off_still_works_after_volume_steps);
  RUN_TEST(test_volume_soap_get_has_no_desired_volume);
  RUN_TEST(test_volume_soap_set_carries_the_volume);
  RUN_TEST(test_parse_volume);
  RUN_TEST(test_ramp_sets_exact_volumes_ending_at_zero);
  RUN_TEST(test_ramp_rereads_the_volume_on_each_new_ramp);
  RUN_TEST(test_ramp_falls_back_to_spread_key_presses);
  RUN_TEST(test_ramp_not_paired_does_nothing);
  RUN_TEST(test_ramp_reports_a_failed_set);
  return UNITY_END();
}

#ifdef INSOMNIATV_NATIVE
int main(void) {
  return runUnityTests();
}
#endif
