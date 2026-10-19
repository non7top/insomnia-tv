// Copyright 2026 insomniaTV Contributors. All rights reserved.

#ifndef SRC_TV_SAMSUNGTVPROTOCOL_H_
#define SRC_TV_SAMSUNGTVPROTOCOL_H_

#include <string>

namespace InsomniaTV {

// Pure helpers for the Samsung Tizen remote API: the REST device-info
// endpoint on port 8001 and the remote-control WebSocket on port 8002.
// No I/O here so they run in native tests.

enum class SamsungPower { Unknown, On, Standby };

struct SamsungWsEvent {
  std::string event;  // e.g. "ms.channel.connect"
  std::string token;  // empty unless the TV handed one out
};

std::string base64Encode(const std::string& in);

// Path (with query) of the remote-control WebSocket endpoint. `token` may be
// empty for the first, unpaired connection.
std::string samsungRemotePath(const std::string& clientName,
                              const std::string& token);

// JSON frame that presses `key` (e.g. "KEY_POWER") once.
std::string samsungKeyPayload(const std::string& key);

// Parses one WebSocket text frame; false when it isn't a JSON object with an
// "event" field.
bool parseSamsungWsEvent(const std::string& json, SamsungWsEvent& out);

// SOAP envelope for a UPnP RenderingControl volume action on the master
// channel: "GetVolume" or "SetVolume" (desiredVolume is used for SetVolume).
std::string samsungVolumeSoap(const std::string& action, int desiredVolume);

// Value of <CurrentVolume> in a GetVolume response, or -1 if absent.
int parseSamsungVolume(const std::string& xml);

// Reads device.PowerState from the GET /api/v2/ response.
SamsungPower parseSamsungPower(const std::string& json);

// Reads device.wifiMac from the GET /api/v2/ response; empty if absent.
std::string parseSamsungWifiMac(const std::string& json);

}  // namespace InsomniaTV

#endif  // SRC_TV_SAMSUNGTVPROTOCOL_H_
