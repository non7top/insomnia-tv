// Copyright 2026 insomniaTV Contributors. All rights reserved.

#ifndef SRC_MQTT_MQTTBRIDGE_H_
#define SRC_MQTT_MQTTBRIDGE_H_

#include "../hal/IMqttClient.h"
#include "../system/SystemStateManager.h"

namespace InsomniaTV {

/**
 * @brief Bridges system state changes to MQTT topics.
 *
 * Listens to system state updates (via SystemStateManager) and publishes them
 * as JSON payloads to a configured MQTT topic, enabling remote monitoring.
 */
class MqttBridge {
public:
  /**
   * @brief Constructs MqttBridge.
   * @param client The MQTT client HAL interface.
   * @param ssm The system state manager to observe.
   */
  MqttBridge(IMqttClient& client, const SystemStateManager& ssm);

  /**
   * @brief Serializes the current system state and publishes it to MQTT.
   */
  void publishStatus();

private:
  IMqttClient& _client;
  const SystemStateManager& _ssm;
};

}  // namespace InsomniaTV

#endif  // SRC_MQTT_MQTTBRIDGE_H_
