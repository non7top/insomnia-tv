// Copyright 2026 insomniaTV Contributors. All rights reserved.

#if defined(ARDUINO)

#include "ArduinoSamsungTransport.h"

#include <Arduino.h>
#include <HTTPClient.h>
#include <WebSocketsClient.h>

#include <string>

namespace InsomniaTV {

static const char kClientName[] = "insomniaTV";
static const uint16_t kRestPort = 8001;
static const uint16_t kWsPort = 8002;
static const uint16_t kUpnpPort = 9197;
static const uint32_t kHttpTimeoutMs = 3000;
// Gives the key frame time to leave the socket before we close it.
static const uint32_t kFlushMs = 300;

std::string ArduinoSamsungTransport::fetchDeviceInfo(const std::string& host) {
  HTTPClient http;
  http.setTimeout(kHttpTimeoutMs);
  std::string url =
      "http://" + host + ":" + std::to_string(kRestPort) + "/api/v2/";
  if (!http.begin(url.c_str())) {
    return "";
  }
  std::string body;
  if (http.GET() == 200) {
    body = http.getString().c_str();
  }
  http.end();
  return body;
}

// UPnP RenderingControl on the TV's media renderer; POSTs one SOAP action and
// returns the response body (empty on failure).
static std::string volumeSoap(const std::string& host,
                              const std::string& action, int volume) {
  HTTPClient http;
  http.setTimeout(kHttpTimeoutMs);
  std::string url = "http://" + host + ":" + std::to_string(kUpnpPort) +
                    "/upnp/control/RenderingControl1";
  if (!http.begin(url.c_str())) {
    return "";
  }
  http.addHeader("Content-Type", "text/xml; charset=\"utf-8\"");
  std::string soapAction =
      "\"urn:schemas-upnp-org:service:RenderingControl:1#" + action + "\"";
  http.addHeader("SOAPACTION", soapAction.c_str());
  std::string body;
  if (http.POST(samsungVolumeSoap(action, volume).c_str()) == 200) {
    body = http.getString().c_str();
  }
  http.end();
  return body;
}

int ArduinoSamsungTransport::getVolume(const std::string& host) {
  return parseSamsungVolume(volumeSoap(host, "GetVolume", 0));
}

bool ArduinoSamsungTransport::setVolume(const std::string& host, int volume) {
  return !volumeSoap(host, "SetVolume", volume).empty();
}

SamsungWsResult ArduinoSamsungTransport::sendKey(const std::string& host,
                                                 const std::string& token,
                                                 const std::string& key,
                                                 uint32_t timeoutMs) {
  SamsungWsResult result;
  bool done = false;
  bool socketUp = false;
  WebSocketsClient ws;

  ws.onEvent([&](WStype_t type, uint8_t* payload, size_t length) {
    if (type == WStype_CONNECTED) {
      socketUp = true;
    } else if (type == WStype_TEXT) {
      SamsungWsEvent ev;
      if (!parseSamsungWsEvent(
              std::string(reinterpret_cast<char*>(payload), length), ev)) {
        return;
      }
      Serial.printf("[SamsungTv] ws event %s\n", ev.event.c_str());
      if (ev.event == "ms.channel.connect") {
        result.status = SamsungWsResult::Status::Ok;
        result.token = ev.token;
        if (!key.empty()) {
          bool sent = ws.sendTXT(samsungKeyPayload(key).c_str());
          Serial.printf("[SamsungTv] sent %s: %s\n", key.c_str(),
                        sent ? "ok" : "failed");
        }
        done = true;
      } else if (ev.event == "ms.channel.unauthorized") {
        result.status = SamsungWsResult::Status::Unauthorized;
        done = true;
      } else if (ev.event == "ms.channel.timeOut") {
        result.status = SamsungWsResult::Status::TimedOut;
        done = true;
      }
    }
  });
  // The lib retries a failed connection on its own; one attempt per call.
  ws.setReconnectInterval(timeoutMs + kFlushMs);
  // Tizen presents a self-signed certificate; the lib skips verification
  // when no fingerprint or CA is given. The TV drops the connection if the
  // lib's default "Sec-WebSocket-Protocol: arduino" header is sent, so the
  // protocol is empty (the header is then omitted).
  ws.beginSSL(host.c_str(), kWsPort,
              samsungRemotePath(kClientName, token).c_str(), "", "");

  uint32_t deadline = millis() + timeoutMs;
  while (!done && static_cast<int32_t>(deadline - millis()) > 0) {
    ws.loop();
    delay(10);
  }
  if (!done) {
    result.status = socketUp ? SamsungWsResult::Status::TimedOut
                             : SamsungWsResult::Status::Unreachable;
  } else if (!key.empty()) {
    uint32_t flushEnd = millis() + kFlushMs;
    while (static_cast<int32_t>(flushEnd - millis()) > 0) {
      ws.loop();
      delay(10);
    }
  }
  ws.disconnect();
  return result;
}

}  // namespace InsomniaTV

#endif  // ARDUINO
