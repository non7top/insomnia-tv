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
  enum class State { UNINITIALIZED, READY, ERROR, RETRYING };

  virtual std::string getId() const = 0;
  virtual std::string getType() const = 0;
  virtual bool read() = 0;
  virtual JsonDocument getConfig() = 0;
  virtual void setConfig(const JsonDocument& cfg) = 0;

  virtual State getState() const { return state_; }
  virtual void setState(State state) { state_ = state; }

  virtual bool isAvailable() const { return state_ == State::READY; }

  // Groups sensors under the device they belong to (e.g. "living_room_tv").
  // Pure metadata for now: runtime stays single-device (#59).
  std::string getDeviceId() const { return deviceId_; }
  void setDeviceId(const std::string& deviceId) { deviceId_ = deviceId; }
  virtual ~Sensor() = default;

protected:
  State state_ = State::UNINITIALIZED;
  std::string deviceId_;
};

}  // namespace InsomniaTV

#endif  // SRC_SENSORS_SENSOR_H_
