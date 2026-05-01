// Copyright 2026 insomniaTV Contributors. All rights reserved.

#include <unity.h>

#include <string>

#include "../../src/mqtt/MqttBridge.h"
#include "../../src/system/SystemStateManager.h"

// Define a mock client directly here to avoid redefinition issues
class MockMqttClient : public InsomniaTV::IMqttClient {
public:
  bool publishCalled = false;
  bool connect(const std::string& broker, uint16_t port,
               const std::string& clientId, const std::string& user,
               const std::string& password) override {
    return true;
  }
  void disconnect() override {}
  bool isConnected() const override { return true; }
  int16_t publish(const std::string& topic, const std::string& payload,
                  uint8_t qos = 0, bool retain = false) override {
    publishCalled = true;
    return 1;
  }
  int16_t subscribe(const std::string& topic, uint8_t qos = 0) override {
    return 1;
  }
  void unsubscribe(const std::string& topic) override {}
  void setCallback(InsomniaTV::MqttCallback callback) override {}
};

using InsomniaTV::MqttBridge;
using InsomniaTV::SystemStateManager;

void test_mqtt_bridge_publish() {
  MockMqttClient mockClient;
  SystemStateManager ssm;
  MqttBridge bridge(mockClient, ssm);

  bridge.publishStatus();
  // Verify client publish was called
  TEST_ASSERT_TRUE(mockClient.publishCalled);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_mqtt_bridge_publish);
  return UNITY_END();
}
