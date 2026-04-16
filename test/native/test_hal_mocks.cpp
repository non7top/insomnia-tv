// Copyright 2026 insomniaTV Contributors. All rights reserved.

#include <unity.h>

#include "hal/IClock.h"
#include "hal/IFileSystem.h"
#include "hal/IIrDriver.h"
#include "hal/IMqttClient.h"
#include "hal/INetworkChecker.h"

// ---------------------------------------------------------------------------
// Mock implementations for native testing
// ---------------------------------------------------------------------------

class MockIrDriver : public InsomniaTV::IIrDriver {
public:
  void begin() override { began_ = true; }
  bool send(const String&, uint64_t, uint16_t) override {
    sendCount_++;
    return true;
  }
  void receive() override { receiving_ = true; }
  uint16_t* learn_raw(uint16_t& out_len) override {
    out_len = 0;
    return nullptr;
  }
  bool hasDecoded() const override { return false; }
  String lastProtocol() const override { return String(); }
  uint64_t lastCode() const override { return 0; }
  uint16_t lastBits() const override { return 0; }

  bool began_ = false;
  bool receiving_ = false;
  int sendCount_ = 0;
};

class MockNetworkChecker : public InsomniaTV::INetworkChecker {
public:
  int32_t ping(const String&) override { return pingResult_; }
  int32_t httpGet(const String&) override { return httpResult_; }
  void setTimeout(uint32_t ms) override { timeout_ = ms; }
  bool isConnected() const override { return connected_; }

  int32_t pingResult_ = 10;
  int32_t httpResult_ = 200;
  uint32_t timeout_ = 1000;
  bool connected_ = true;
};

class MockClock : public InsomniaTV::IClock {
public:
  uint32_t nowMs() const override { return currentMs_; }
  void advanceMs(uint32_t ms) override { currentMs_ += ms; }

  mutable uint32_t currentMs_ = 0;
};

class MockFileSystem : public InsomniaTV::IFileSystem {
public:
  bool mount() override { return mountResult_; }
  String readJson(const String&) override { return jsonString_; }
  bool writeJson(const String&, const String&) override { return writeResult_; }
  bool uploadFile(const String&, const uint8_t*, size_t) override {
    return uploadResult_;
  }
  int32_t downloadFile(const String&, uint8_t*, size_t) override {
    return downloadResult_;
  }
  bool exists(const String&) const override { return existsResult_; }
  bool remove(const String&) override { return removeResult_; }

  bool mountResult_ = true;
  String jsonString_ = "{}";
  bool writeResult_ = true;
  bool uploadResult_ = true;
  int32_t downloadResult_ = 0;
  bool existsResult_ = false;
  bool removeResult_ = true;
};

class MockMqttClient : public InsomniaTV::IMqttClient {
public:
  bool connect(const String&, uint16_t, const String&, const String&,
               const String&) override {
    connected_ = true;
    return true;
  }
  void disconnect() override { connected_ = false; }
  bool isConnected() const override { return connected_; }
  int16_t publish(const String&, const String&, uint8_t, bool) override {
    publishCount_++;
    return 1;
  }
  int16_t subscribe(const String&, uint8_t) override { return 1; }
  void unsubscribe(const String&) override {}
  void setCallback(InsomniaTV::MqttCallback) override { hasCallback_ = true; }

  bool connected_ = false;
  int publishCount_ = 0;
  bool hasCallback_ = false;
};

// ---------------------------------------------------------------------------
// Test: Mock IR driver can be instantiated and used
// ---------------------------------------------------------------------------
void test_mock_ir_driver_instantiation(void) {
  MockIrDriver driver;
  TEST_ASSERT_FALSE(driver.began_);
  driver.begin();
  TEST_ASSERT_TRUE(driver.began_);

  TEST_ASSERT_TRUE(driver.send("NEC", 0x20DF, 32));
  TEST_ASSERT_EQUAL_INT(1, driver.sendCount_);
}

// ---------------------------------------------------------------------------
// Test: Mock network checker responds correctly
// ---------------------------------------------------------------------------
void test_mock_network_checker(void) {
  MockNetworkChecker checker;
  TEST_ASSERT_TRUE(checker.isConnected());
  TEST_ASSERT_EQUAL_INT32(10, checker.ping("192.168.1.1"));
  TEST_ASSERT_EQUAL_INT32(200, checker.httpGet("http://192.168.1.1/status"));

  checker.connected_ = false;
  TEST_ASSERT_FALSE(checker.isConnected());
}

// ---------------------------------------------------------------------------
// Test: Mock clock time advances correctly
// ---------------------------------------------------------------------------
void test_mock_clock_advance(void) {
  MockClock clock;
  TEST_ASSERT_EQUAL_UINT32(0, clock.nowMs());

  clock.advanceMs(5000);
  TEST_ASSERT_EQUAL_UINT32(5000, clock.nowMs());

  clock.advanceMs(10000);
  TEST_ASSERT_EQUAL_UINT32(15000, clock.nowMs());
}

// ---------------------------------------------------------------------------
// Test: Mock filesystem operations
// ---------------------------------------------------------------------------
void test_mock_filesystem(void) {
  MockFileSystem fs;
  TEST_ASSERT_TRUE(fs.mount());
  TEST_ASSERT_FALSE(fs.exists("/test.json"));

  fs.existsResult_ = true;
  TEST_ASSERT_TRUE(fs.exists("/test.json"));

  TEST_ASSERT_TRUE(fs.writeJson("/test.json", "{}"));
  TEST_ASSERT_EQUAL_STRING("{}", fs.readJson("/test.json").c_str());
}

// ---------------------------------------------------------------------------
// Test: Mock MQTT client operations
// ---------------------------------------------------------------------------
void test_mock_mqtt_client(void) {
  MockMqttClient mqtt;
  TEST_ASSERT_FALSE(mqtt.isConnected());

  TEST_ASSERT_TRUE(mqtt.connect("broker.local", 1883, "test", "", ""));
  TEST_ASSERT_TRUE(mqtt.isConnected());

  TEST_ASSERT_EQUAL_INT(1, mqtt.publish("home/test", "{\"state\":\"on\"}"));
  TEST_ASSERT_EQUAL_INT(1, mqtt.publishCount_);

  mqtt.disconnect();
  TEST_ASSERT_FALSE(mqtt.isConnected());
}

// ---------------------------------------------------------------------------
// Test: All HAL interfaces are polymorphically usable
// ---------------------------------------------------------------------------
void test_hal_polymorphism(void) {
  InsomniaTV::IIrDriver* ir = new MockIrDriver();
  ir->begin();
  delete ir;

  InsomniaTV::INetworkChecker* net = new MockNetworkChecker();
  TEST_ASSERT_TRUE(net->isConnected());
  delete net;

  InsomniaTV::IClock* clk = new MockClock();
  TEST_ASSERT_EQUAL_UINT32(0, clk->nowMs());
  delete clk;

  InsomniaTV::IFileSystem* fs = new MockFileSystem();
  TEST_ASSERT_TRUE(fs->mount());
  delete fs;

  InsomniaTV::IMqttClient* mqtt = new MockMqttClient();
  TEST_ASSERT_FALSE(mqtt->isConnected());
  delete mqtt;
}

// ---------------------------------------------------------------------------
// Run all HAL tests
// ---------------------------------------------------------------------------
int runHalTests(void) {
  RUN_TEST(test_mock_ir_driver_instantiation);
  RUN_TEST(test_mock_network_checker);
  RUN_TEST(test_mock_clock_advance);
  RUN_TEST(test_mock_filesystem);
  RUN_TEST(test_mock_mqtt_client);
  RUN_TEST(test_hal_polymorphism);
  return 0;
}
