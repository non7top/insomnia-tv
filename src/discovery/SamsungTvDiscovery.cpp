// Copyright 2026 insomniaTV Contributors. All rights reserved.

#include "SamsungTvDiscovery.h"

#if defined(ARDUINO)
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#endif

#include <algorithm>
#include <string>
#include <vector>

namespace InsomniaTV {

SamsungTvDiscovery::SamsungTvDiscovery() : _isScanning(false) {}

void SamsungTvDiscovery::scan() {
#if defined(ARDUINO)
  {
    std::lock_guard<std::mutex> lock(_mutex);
    if (_isScanning) return;
    _isScanning = true;
  }

  WiFiUDP udp;
  IPAddress ssdpIp(239, 255, 255, 250);
  const uint16_t ssdpPort = 1900;

  const char* mSearch =
      "M-SEARCH * HTTP/1.1\r\n"
      "HOST: 239.255.255.250:1900\r\n"
      "MAN: \"ssdp:discover\"\r\n"
      "MX: 2\r\n"
      "ST: urn:schemas-upnp-org:device:MediaRenderer:1\r\n"
      "\r\n";

  udp.beginMulticast(ssdpIp, ssdpPort);
  udp.beginPacket(ssdpIp, ssdpPort);
  udp.write(reinterpret_cast<const uint8_t*>(mSearch), strlen(mSearch));
  udp.endPacket();

  uint32_t start = millis();
  while (millis() - start < 3000) {  // Scan for 3 seconds
    int packetSize = udp.parsePacket();
    if (packetSize) {
      char packetBuffer[1024];
      int len = udp.read(packetBuffer, sizeof(packetBuffer) - 1);
      if (len > 0) {
        packetBuffer[len] = 0;
        parseSsdpResponse(std::string(packetBuffer));
      }
    }
    delay(10);
  }

  {
    std::lock_guard<std::mutex> lock(_mutex);
    _isScanning = false;
  }
#endif
}

bool SamsungTvDiscovery::isScanning() const {
  std::lock_guard<std::mutex> lock(_mutex);
  return _isScanning;
}

std::vector<SamsungTvInfo> SamsungTvDiscovery::getDiscoveredTvs() const {
  std::lock_guard<std::mutex> lock(_mutex);
  return _discoveredTvs;
}

void SamsungTvDiscovery::clear() {
  std::lock_guard<std::mutex> lock(_mutex);
  _discoveredTvs.clear();
}

void SamsungTvDiscovery::parseSsdpResponse(const std::string& response) {
  size_t locPos = response.find("LOCATION: ");
  if (locPos == std::string::npos) locPos = response.find("location: ");
  if (locPos == std::string::npos) return;

  size_t endPos = response.find("\r\n", locPos);
  if (endPos == std::string::npos) return;

  std::string location = response.substr(locPos + 10, endPos - (locPos + 10));

  // Check if we already have this location
  bool exists = false;
  {
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = std::find_if(_discoveredTvs.begin(), _discoveredTvs.end(),
                           [&location](const SamsungTvInfo& info) {
                             return info.location == location;
                           });
    if (it != _discoveredTvs.end()) exists = true;
  }

  if (!exists) {
    SamsungTvInfo newTv;
    newTv.location = location;

    // Extract IP from location
    size_t ipStart = location.find("//");
    if (ipStart != std::string::npos) {
      size_t ipEnd = location.find(":", ipStart + 2);
      if (ipEnd == std::string::npos) ipEnd = location.find("/", ipStart + 2);
      if (ipEnd != std::string::npos) {
        newTv.ip = location.substr(ipStart + 2, ipEnd - (ipStart + 2));
      }
    }

    fetchDeviceMetadata(newTv);
    if (!newTv.name.empty()) {
      std::lock_guard<std::mutex> lock(_mutex);
      _discoveredTvs.push_back(newTv);
    }
  }
}

void SamsungTvDiscovery::fetchDeviceMetadata(SamsungTvInfo& tv) {
#if defined(ARDUINO)
  HTTPClient http;
  http.begin(tv.location.c_str());
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    std::string payload = http.getString().c_str();

    // Very basic XML extraction for friendlyName and modelName
    size_t fnStart = payload.find("<friendlyName>");
    size_t fnEnd = payload.find("</friendlyName>");
    if (fnStart != std::string::npos && fnEnd != std::string::npos) {
      tv.name = payload.substr(fnStart + 14, fnEnd - (fnStart + 14));
    }

    size_t mnStart = payload.find("<modelName>");
    size_t mnEnd = payload.find("</modelName>");
    if (mnStart != std::string::npos && mnEnd != std::string::npos) {
      tv.model = payload.substr(mnStart + 11, mnEnd - (mnStart + 11));
    }

    size_t mNumStart = payload.find("<modelNumber>");
    size_t mNumEnd = payload.find("</modelNumber>");
    if (mNumStart != std::string::npos && mNumEnd != std::string::npos) {
      std::string modelNumber =
          payload.substr(mNumStart + 13, mNumEnd - (mNumStart + 13));
      if (!modelNumber.empty()) {
        tv.model += " (" + modelNumber + ")";
      }
    }
  }
  http.end();
#endif
}

}  // namespace InsomniaTV
