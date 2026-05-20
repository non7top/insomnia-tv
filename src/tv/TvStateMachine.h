// Copyright 2026 insomniaTV Contributors. All rights reserved.

#ifndef SRC_TV_TVSTATEMACHINE_H_
#define SRC_TV_TVSTATEMACHINE_H_

#include <ArduinoJson.h>

#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include "../sensors/SensorManager.h"

namespace InsomniaTV {

/**
 * @brief Determines TV power state by fusing readings from multiple sensors.
 *
 * Each registered detection sensor casts a weighted vote (+weight when its
 * read() is true, -weight when false).  Aggregated confidence is scaled to
 * [-10, +10].  A configurable hysteresis count prevents flapping.
 *
 * Confidence thresholds:
 *   > +3  → ON
 *   < -3  → OFF
 *   else  → UNKNOWN
 */
class TvStateMachine {
 public:
  enum class PowerState { UNKNOWN, ON, OFF, TRANSITIONING };

  struct SensorConfig {
    std::string sensorId;
    int weight;   // 1-5; higher = more influence
    bool enabled;
  };

  struct SensorContribution {
    std::string sensorId;
    bool rawValue;
    int weightedVote;  // weight * (+1 or -1); 0 when disabled or no reading
    bool enabled;
    bool available;    // false when no reading has been received yet
  };

  explicit TvStateMachine(SensorManager& sensorManager,
                          int hysteresisCount = 2);

  // Load detection sensor list from JSON config.
  // Expected shape: {"hysteresis_count": 2,
  //                  "detection_sensors": [{"sensor_id":"..","weight":3,"enabled":true}]}
  void begin(const JsonDocument& config);

  PowerState getPowerState() const;

  // Send a power on/off command via the registered callback.
  // Sets state to TRANSITIONING while the command is in flight.
  // Returns false if no power command callback has been set.
  bool sendPowerCommand(bool on);

  // Register a callback for power-command delivery (IR, UPnP, etc.)
  void setPowerCommandCallback(std::function<void(bool)> callback);

  void subscribe(std::function<void(PowerState)> callback);

  // Snapshot of each sensor's current contribution for UI / debug.
  std::vector<SensorContribution> getContributions() const;

  // Called by SensorManager when a sensor value changes.
  void onSensorUpdate(const std::string& sensorId, bool value);

 private:
  // Returns true if the current state changed (caller must hold mutex_).
  bool evaluateState();
  float calculateConfidence() const;  // caller must hold mutex_

  SensorManager& sensorManager_;
  int hysteresisCount_;

  PowerState currentState_;
  int consecutiveVotes_;
  PowerState lastVote_;

  std::vector<SensorConfig> detectionSensors_;
  std::map<std::string, bool> sensorValues_;

  std::vector<std::function<void(PowerState)>> subscribers_;
  std::function<void(bool)> powerCommandCallback_;

  mutable std::mutex mutex_;
};

}  // namespace InsomniaTV

#endif  // SRC_TV_TVSTATEMACHINE_H_
