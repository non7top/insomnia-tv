// Copyright 2026 insomniaTV Contributors. All rights reserved.

#ifndef SRC_TV_SAMSUNGTVCONTROLLER_H_
#define SRC_TV_SAMSUNGTVCONTROLLER_H_

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>

#include "SamsungTvProtocol.h"

namespace InsomniaTV {

struct SamsungWsResult {
  enum class Status { Ok, Unreachable, Unauthorized, TimedOut };
  Status status = Status::Unreachable;
  std::string token;  // set when the TV handed one out on connect
};

// Network I/O behind the controller, so its decisions are testable natively.
class ISamsungTvTransport {
public:
  virtual ~ISamsungTvTransport() = default;

  // Body of GET http://<host>:8001/api/v2/, empty if unreachable.
  virtual std::string fetchDeviceInfo(const std::string& host) = 0;

  // TV volume via UPnP RenderingControl, or -1 if it can't be read.
  virtual int getVolume(const std::string& host) = 0;
  virtual bool setVolume(const std::string& host, int volume) = 0;

  // Opens the remote-control WebSocket on 8002, waits up to `timeoutMs` for
  // the TV to accept the connection (a first connection shows an allow
  // prompt on screen), then presses `key` if non-empty.
  virtual SamsungWsResult sendKey(const std::string& host,
                                  const std::string& token,
                                  const std::string& key,
                                  uint32_t timeoutMs) = 0;
};

enum class PowerOffResult {
  Sent,
  AlreadyOff,
  NotPaired,
  Unreachable,
  StateUnknown,
  Rejected
};

enum class PairState { Idle, Waiting, Paired, Failed };

// Pairs with a Samsung Tizen TV and switches it off over the network.
// KEY_POWER toggles, so it is only sent while the TV reports PowerState "on".
class SamsungTvController {
public:
  using CredentialsCallback =
      std::function<void(const std::string& host, const std::string& token,
                         const std::string& mac)>;

  static constexpr uint32_t kPairTimeoutMs = 30000;
  static constexpr uint32_t kKeyTimeoutMs = 10000;
  static constexpr uint8_t kLastStepExtraPresses = 5;

  explicit SamsungTvController(ISamsungTvTransport& transport);

  void configure(const std::string& host, const std::string& token,
                 const std::string& mac);
  void setCredentialsCallback(CredentialsCallback cb);

  bool isPaired() const;
  std::string host() const;
  std::string mac() const;
  PairState pairState() const;
  std::string lastError() const;

  // Blocking; the TV needs its allow prompt accepted within kPairTimeoutMs.
  bool pair(const std::string& host);

  // Blocking; waits for any other TV operation in flight (a ramp step must
  // not make the final power-off fail).
  PowerOffResult powerOff();

  // Blocking; presses KEY_VOLDOWN `steps` times. False if not paired or a
  // press failed.
  bool volumeDown(uint8_t steps);

  // Blocking; one ramp step (1-based, of `totalSteps`) that lowers the volume
  // so it reaches 0 on the last step. The starting volume is read on step 1
  // and set exactly; if it can't be read, `typicalVolume` KEY_VOLDOWN presses
  // are spread across the steps, with a margin on the last one.
  bool rampStep(uint8_t step, uint8_t totalSteps, uint8_t typicalVolume);

  // Blocking; Unknown if the TV can't be reached.
  SamsungPower queryPower();

private:
  void setPairResult(PairState state, const std::string& error);
  bool pressVolumeDown(const std::string& host, const std::string& token,
                       uint8_t steps);

  ISamsungTvTransport& transport_;
  mutable std::mutex mutex_;
  std::atomic<bool> busy_;    // pairing in progress
  std::mutex opMutex_;        // serializes TV key operations
  int rampStartVolume_ = -1;  // read on ramp step 1; -1 if unknown
  std::string host_;
  std::string token_;
  std::string mac_;
  PairState pairState_;
  std::string lastError_;
  CredentialsCallback credentialsCb_;
};

}  // namespace InsomniaTV

#endif  // SRC_TV_SAMSUNGTVCONTROLLER_H_
