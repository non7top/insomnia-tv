// Copyright 2026 insomniaTV Contributors. All rights reserved.

#ifndef SRC_SENSORS_SENSOR_H_
#define SRC_SENSORS_SENSOR_H_

#include <ArduinoJson.h>

#include <string>

namespace InsomniaTV {

/**
 * @brief Abstract base class for all sensors (hardware and software).
 */
class Sensor {
public:
  virtual std::string getId() const = 0;
  virtual std::string getType()
      const = 0;            // "gpio_input", "upnp", "ping", etc.
  virtual bool read() = 0;  // Returns current state/value
  virtual JsonDocument getConfig() = 0;
  virtual void setConfig(const JsonDocument& cfg) = 0;
  virtual bool isAvailable() const { return true; }  // Online/connected status
  virtual ~Sensor() = default;
};

}  // namespace InsomniaTV

#endif  // SRC_SENSORS_SENSOR_H_
