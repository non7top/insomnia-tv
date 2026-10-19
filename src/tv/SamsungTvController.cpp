// Copyright 2026 insomniaTV Contributors. All rights reserved.

#include "SamsungTvController.h"

#include <string>
#include <utility>

namespace InsomniaTV {

SamsungTvController::SamsungTvController(ISamsungTvTransport& transport)
    : transport_(transport), busy_(false), pairState_(PairState::Idle) {}

void SamsungTvController::configure(const std::string& host,
                                    const std::string& token,
                                    const std::string& mac) {
  std::lock_guard<std::mutex> lock(mutex_);
  host_ = host;
  token_ = token;
  mac_ = mac;
  pairState_ = token.empty() ? PairState::Idle : PairState::Paired;
}

void SamsungTvController::setCredentialsCallback(CredentialsCallback cb) {
  std::lock_guard<std::mutex> lock(mutex_);
  credentialsCb_ = std::move(cb);
}

bool SamsungTvController::isPaired() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return !host_.empty() && !token_.empty();
}

std::string SamsungTvController::host() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return host_;
}

std::string SamsungTvController::mac() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return mac_;
}

PairState SamsungTvController::pairState() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return pairState_;
}

std::string SamsungTvController::lastError() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return lastError_;
}

void SamsungTvController::setPairResult(PairState state,
                                        const std::string& error) {
  std::lock_guard<std::mutex> lock(mutex_);
  pairState_ = state;
  lastError_ = error;
}

bool SamsungTvController::pair(const std::string& host) {
  if (host.empty() || busy_.exchange(true)) {
    return false;
  }
  setPairResult(PairState::Waiting, "");

  SamsungWsResult ws = transport_.sendKey(host, "", "", kPairTimeoutMs);
  bool ok = false;
  if (ws.status == SamsungWsResult::Status::Ok && !ws.token.empty()) {
    std::string mac = parseSamsungWifiMac(transport_.fetchDeviceInfo(host));
    CredentialsCallback cb;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      host_ = host;
      token_ = ws.token;
      if (!mac.empty()) {
        mac_ = mac;
      }
      mac = mac_;
      cb = credentialsCb_;
    }
    if (cb) {
      cb(host, ws.token, mac);
    }
    setPairResult(PairState::Paired, "");
    ok = true;
  } else if (ws.status == SamsungWsResult::Status::Ok) {
    setPairResult(PairState::Failed, "TV did not return a pairing token");
  } else if (ws.status == SamsungWsResult::Status::TimedOut) {
    setPairResult(PairState::Failed, "Pairing prompt not accepted on the TV");
  } else if (ws.status == SamsungWsResult::Status::Unauthorized) {
    setPairResult(PairState::Failed, "TV denied the pairing request");
  } else {
    setPairResult(PairState::Failed, "TV unreachable on port 8002");
  }
  busy_ = false;
  return ok;
}

SamsungPower SamsungTvController::queryPower() {
  return parseSamsungPower(transport_.fetchDeviceInfo(host()));
}

bool SamsungTvController::pressVolumeDown(const std::string& host,
                                          const std::string& token,
                                          uint8_t steps) {
  for (uint8_t i = 0; i < steps; i++) {
    SamsungWsResult ws =
        transport_.sendKey(host, token, "KEY_VOLDOWN", kKeyTimeoutMs);
    if (ws.status != SamsungWsResult::Status::Ok) {
      return false;
    }
  }
  return true;
}

bool SamsungTvController::volumeDown(uint8_t steps) {
  std::lock_guard<std::mutex> op(opMutex_);
  std::string host;
  std::string token;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    host = host_;
    token = token_;
  }
  if (host.empty() || token.empty()) {
    return false;
  }
  return pressVolumeDown(host, token, steps);
}

bool SamsungTvController::rampStep(uint8_t step, uint8_t totalSteps,
                                   uint8_t typicalVolume) {
  std::lock_guard<std::mutex> op(opMutex_);
  std::string host;
  std::string token;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    host = host_;
    token = token_;
  }
  if (host.empty() || token.empty() || totalSteps == 0) {
    return false;
  }
  if (step <= 1) {
    rampStartVolume_ = transport_.getVolume(host);
  }
  if (step > totalSteps) {
    step = totalSteps;
  }
  if (rampStartVolume_ < 0) {
    int presses = typicalVolume * step / totalSteps -
                  typicalVolume * (step - 1) / totalSteps;
    if (step == totalSteps) {
      presses += kLastStepExtraPresses;
    }
    return pressVolumeDown(host, token, static_cast<uint8_t>(presses));
  }
  int target = rampStartVolume_ * (totalSteps - step) / totalSteps;
  return transport_.setVolume(host, target);
}

PowerOffResult SamsungTvController::powerOff() {
  std::lock_guard<std::mutex> op(opMutex_);
  PowerOffResult result;
  std::string host;
  std::string token;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    host = host_;
    token = token_;
  }

  if (host.empty() || token.empty()) {
    result = PowerOffResult::NotPaired;
  } else {
    std::string info = transport_.fetchDeviceInfo(host);
    SamsungPower power = parseSamsungPower(info);
    if (info.empty()) {
      result = PowerOffResult::Unreachable;
    } else if (power == SamsungPower::Standby) {
      result = PowerOffResult::AlreadyOff;
    } else if (power != SamsungPower::On) {
      result = PowerOffResult::StateUnknown;
    } else {
      SamsungWsResult ws =
          transport_.sendKey(host, token, "KEY_POWER", kKeyTimeoutMs);
      if (ws.status == SamsungWsResult::Status::Ok) {
        result = PowerOffResult::Sent;
      } else if (ws.status == SamsungWsResult::Status::Unauthorized) {
        result = PowerOffResult::Rejected;
      } else {
        result = PowerOffResult::Unreachable;
      }
    }
  }
  return result;
}

}  // namespace InsomniaTV
