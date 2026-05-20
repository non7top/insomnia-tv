// Copyright 2026 insomniaTV Contributors. All rights reserved.

#include "TvStateMachine.h"

namespace InsomniaTV {

TvStateMachine::TvStateMachine(SensorManager& sensorManager,
                                int hysteresisCount)
    : sensorManager_(sensorManager),
      hysteresisCount_(hysteresisCount),
      currentState_(PowerState::UNKNOWN),
      consecutiveVotes_(0),
      lastVote_(PowerState::UNKNOWN) {
  sensorManager_.subscribeValueChange(
      [this](const std::string& id, bool value) {
        this->onSensorUpdate(id, value);
      });
}

void TvStateMachine::begin(const JsonDocument& config) {
  std::lock_guard<std::mutex> lock(mutex_);
  detectionSensors_.clear();
  sensorValues_.clear();
  consecutiveVotes_ = 0;
  lastVote_ = PowerState::UNKNOWN;
  currentState_ = PowerState::UNKNOWN;

  if (config["hysteresis_count"].is<int>()) {
    hysteresisCount_ = config["hysteresis_count"].as<int>();
  }

  JsonArrayConst arr = config["detection_sensors"].as<JsonArrayConst>();
  if (!arr.isNull()) {
    for (JsonObjectConst sc : arr) {
      SensorConfig cfg;
      cfg.sensorId = sc["sensor_id"] | "";
      cfg.weight = sc["weight"] | 1;
      cfg.enabled = sc["enabled"] | true;
      if (!cfg.sensorId.empty()) {
        detectionSensors_.push_back(cfg);
      }
    }
  }
}

TvStateMachine::PowerState TvStateMachine::getPowerState() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return currentState_;
}

bool TvStateMachine::sendPowerCommand(bool on) {
  std::function<void(bool)> cb;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!powerCommandCallback_) return false;
    currentState_ = PowerState::TRANSITIONING;
    cb = powerCommandCallback_;
  }
  cb(on);
  return true;
}

void TvStateMachine::setPowerCommandCallback(std::function<void(bool)> callback) {
  std::lock_guard<std::mutex> lock(mutex_);
  powerCommandCallback_ = callback;
}

void TvStateMachine::subscribe(std::function<void(PowerState)> callback) {
  std::lock_guard<std::mutex> lock(mutex_);
  subscribers_.push_back(callback);
}

std::vector<TvStateMachine::SensorContribution>
TvStateMachine::getContributions() const {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<SensorContribution> result;
  for (const auto& sc : detectionSensors_) {
    SensorContribution contrib;
    contrib.sensorId = sc.sensorId;
    contrib.enabled = sc.enabled;
    auto it = sensorValues_.find(sc.sensorId);
    contrib.available = (it != sensorValues_.end());
    contrib.rawValue = contrib.available ? it->second : false;
    contrib.weightedVote =
        (sc.enabled && contrib.available)
            ? sc.weight * (contrib.rawValue ? +1 : -1)
            : 0;
    result.push_back(contrib);
  }
  return result;
}

void TvStateMachine::onSensorUpdate(const std::string& sensorId, bool value) {
  PowerState newState;
  bool changed = false;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    sensorValues_[sensorId] = value;
    changed = evaluateState();
    newState = currentState_;
  }
  if (changed) {
    for (auto& cb : subscribers_) cb(newState);
  }
}

float TvStateMachine::calculateConfidence() const {
  int votes = 0;
  int maxVotes = 0;
  for (const auto& sc : detectionSensors_) {
    if (!sc.enabled) continue;
    auto it = sensorValues_.find(sc.sensorId);
    if (it == sensorValues_.end()) continue;
    votes += sc.weight * (it->second ? +1 : -1);
    maxVotes += sc.weight;
  }
  if (maxVotes == 0) return 0.0f;
  return static_cast<float>(votes) / maxVotes * 10.0f;
}

bool TvStateMachine::evaluateState() {
  float confidence = calculateConfidence();

  PowerState vote = PowerState::UNKNOWN;
  if (confidence > 3.0f) {
    vote = PowerState::ON;
  } else if (confidence < -3.0f) {
    vote = PowerState::OFF;
  }

  if (vote == lastVote_) {
    consecutiveVotes_++;
  } else {
    lastVote_ = vote;
    consecutiveVotes_ = 1;
  }

  if (consecutiveVotes_ >= hysteresisCount_ && vote != currentState_) {
    currentState_ = vote;
    return true;
  }
  return false;
}

}  // namespace InsomniaTV
