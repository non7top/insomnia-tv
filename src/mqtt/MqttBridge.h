// Copyright 2026 insomniaTV Contributors. All rights reserved.

#ifndef SRC_MQTT_MQTTBRIDGE_H_
#define SRC_MQTT_MQTTBRIDGE_H_

#include "../hal/IMqttClient.h"
#include "../system/SystemStateManager.h"

namespace InsomniaTV {

/**
 * @brief Bridges system state changes to MQTT topics.
 */
class MqttBridge {
public:
  MqttBridge(IMqttClient& client, const SystemStateManager& ssm);
  void publishStatus();

private:
  IMqttClient& _client;
  const SystemStateManager& _ssm;
};

}  // namespace InsomniaTV

#endif  // SRC_MQTT_MQTTBRIDGE_H_
