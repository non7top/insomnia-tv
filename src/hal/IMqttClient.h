// Copyright 2026 insomniaTV Contributors. All rights reserved.

#ifndef SRC_HAL_IMQTTCLIENT_H_
#define SRC_HAL_IMQTTCLIENT_H_

#if defined(ARDUINO) || defined(ESP32)
#include <Arduino.h>
#endif

#include <functional>
#include <string>

namespace InsomniaTV {

// MQTT message callback: (topic, payload, length)
using MqttCallback = std::function<void(const char*, const char*, size_t)>;

// Abstract interface for MQTT client operations.
// Wraps AsyncMqttClient for ESP32; mockable for native tests.
class IMqttClient {
public:
  virtual ~IMqttClient() = default;

  // Connect to broker, returns true on success
  virtual bool connect(const std::string& broker, uint16_t port,
                       const std::string& clientId, const std::string& user,
                       const std::string& password) = 0;

  // Disconnect from broker
  virtual void disconnect() = 0;

  // Check if connected to broker
  virtual bool isConnected() const = 0;

  // Publish a message, returns message ID or -1 on failure
  virtual int16_t publish(const std::string& topic, const std::string& payload,
                          uint8_t qos = 0, bool retain = false) = 0;

  // Subscribe to a topic, returns sub ID or -1 on failure
  virtual int16_t subscribe(const std::string& topic, uint8_t qos = 0) = 0;

  // Unsubscribe from a topic
  virtual void unsubscribe(const std::string& topic) = 0;

  // Set callback for incoming messages
  virtual void setCallback(MqttCallback callback) = 0;
};

}  // namespace InsomniaTV

#endif  // SRC_HAL_IMQTTCLIENT_H_
