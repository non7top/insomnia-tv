// Copyright 2026 insomniaTV Contributors. All rights reserved.

#include "MqttBridge.h"

#include <ArduinoJson.h>

namespace InsomniaTV {

MqttBridge::MqttBridge(IMqttClient& client, const SystemStateManager& ssm)
    : _client(client), _ssm(ssm) {}

void MqttBridge::publishStatus() {
  JsonDocument doc;
  const auto& state = _ssm.getState();
  doc["tvPower"] = static_cast<int>(state.tvPower);
  doc["presence"] = static_cast<int>(state.presence);
  doc["activity"] = static_cast<int>(state.activity);

  char buffer[256];
  serializeJson(doc, buffer);
  _client.publish("home/insomnia_tv/status", buffer);
}

}  // namespace InsomniaTV
