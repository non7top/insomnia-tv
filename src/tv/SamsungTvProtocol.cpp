// Copyright 2026 insomniaTV Contributors. All rights reserved.

#include "SamsungTvProtocol.h"

#include <ArduinoJson.h>

#include <cstdint>
#include <string>

namespace InsomniaTV {

std::string base64Encode(const std::string& in) {
  static const char kAlphabet[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string out;
  out.reserve((in.size() + 2) / 3 * 4);
  size_t i = 0;
  while (i + 2 < in.size()) {
    uint32_t n = (static_cast<uint8_t>(in[i]) << 16) |
                 (static_cast<uint8_t>(in[i + 1]) << 8) |
                 static_cast<uint8_t>(in[i + 2]);
    out += kAlphabet[(n >> 18) & 63];
    out += kAlphabet[(n >> 12) & 63];
    out += kAlphabet[(n >> 6) & 63];
    out += kAlphabet[n & 63];
    i += 3;
  }
  if (i + 1 == in.size()) {
    uint32_t n = static_cast<uint8_t>(in[i]) << 16;
    out += kAlphabet[(n >> 18) & 63];
    out += kAlphabet[(n >> 12) & 63];
    out += "==";
  } else if (i + 2 == in.size()) {
    uint32_t n = (static_cast<uint8_t>(in[i]) << 16) |
                 (static_cast<uint8_t>(in[i + 1]) << 8);
    out += kAlphabet[(n >> 18) & 63];
    out += kAlphabet[(n >> 12) & 63];
    out += kAlphabet[(n >> 6) & 63];
    out += '=';
  }
  return out;
}

std::string samsungRemotePath(const std::string& clientName,
                              const std::string& token) {
  std::string path = "/api/v2/channels/samsung.remote.control?name=";
  // '=' padding is left as-is (the form the TV accepted); '+' and '/' would
  // be misread in a query value.
  for (char c : base64Encode(clientName)) {
    if (c == '+') {
      path += "%2B";
    } else if (c == '/') {
      path += "%2F";
    } else {
      path += c;
    }
  }
  if (!token.empty()) {
    path += "&token=" + token;
  }
  return path;
}

std::string samsungKeyPayload(const std::string& key) {
  JsonDocument doc;
  doc["method"] = "ms.remote.control";
  JsonObject params = doc["params"].to<JsonObject>();
  params["Cmd"] = "Click";
  params["DataOfCmd"] = key;
  params["Option"] = "false";
  params["TypeOfRemote"] = "SendRemoteKey";
  std::string out;
  serializeJson(doc, out);
  return out;
}

static std::string tokenToString(JsonVariantConst v) {
  if (v.is<const char*>()) {
    return v.as<const char*>();
  }
  if (v.is<int64_t>()) {
    return std::to_string(v.as<int64_t>());
  }
  return "";
}

bool parseSamsungWsEvent(const std::string& json, SamsungWsEvent& out) {
  JsonDocument doc;
  if (deserializeJson(doc, json) || !doc["event"].is<const char*>()) {
    return false;
  }
  out.event = doc["event"].as<const char*>();
  out.token = tokenToString(doc["data"]["token"]);
  if (out.token.empty()) {
    JsonArrayConst clients = doc["data"]["clients"].as<JsonArrayConst>();
    for (JsonVariantConst client : clients) {
      std::string t = tokenToString(client["attributes"]["token"]);
      if (!t.empty()) {
        out.token = t;
        break;
      }
    }
  }
  return true;
}

SamsungPower parseSamsungPower(const std::string& json) {
  JsonDocument doc;
  if (deserializeJson(doc, json)) {
    return SamsungPower::Unknown;
  }
  std::string state = doc["device"]["PowerState"] | "";
  if (state == "on") {
    return SamsungPower::On;
  }
  if (state == "standby") {
    return SamsungPower::Standby;
  }
  return SamsungPower::Unknown;
}

std::string parseSamsungWifiMac(const std::string& json) {
  JsonDocument doc;
  if (deserializeJson(doc, json)) {
    return "";
  }
  return doc["device"]["wifiMac"] | "";
}

}  // namespace InsomniaTV
