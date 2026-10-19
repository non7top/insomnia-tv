// Copyright 2026 insomniaTV Contributors. All rights reserved.

#include "WifiSetup.h"

#include <cstdio>
#include <string>

#include "../config/SystemCredentials.h"

#if defined(ARDUINO)
#include <ArduinoOTA.h>
#include <ESP32Ping.h>
#include <WiFi.h>
#include <WiFiManager.h>
#endif

namespace InsomniaTV {

#if defined(ARDUINO)
// GPIO 9 is the BOOT button on most C3 modules
static const int RESET_BUTTON_PIN = 9;
#endif

WifiSetup::WifiSetup() : _buttonPressStart(0), _isButtonPressed(false) {}

void WifiSetup::checkConnectivity() {
#if defined(ARDUINO)
  if (_portalActive) {
    return;
  }
  bool reachable = WiFi.status() == WL_CONNECTED &&
                   Ping.ping(WiFi.gatewayIP().toString().c_str(), 1);
  switch (_watchdog.onCheck(reachable)) {
    case WifiWatchdog::Action::Rejoin:
      Serial.println("[insomniaTV] Gateway unreachable, rejoining WiFi");
      WiFi.disconnect();
      WiFi.begin();
      break;
    case WifiWatchdog::Action::Restart:
      Serial.println("[insomniaTV] WiFi did not recover, restarting");
      ESP.restart();
      break;
    case WifiWatchdog::Action::None:
      break;
  }
#endif
}

void WifiSetup::begin() {
#if defined(ARDUINO)
  // Join the strongest access point with this SSID; the default takes the
  // first match, which can be a node that accepts the device but passes
  // nothing.
  WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN);
  WiFi.setSortMethod(WIFI_CONNECT_AP_BY_SIGNAL);

  WiFiManager wm;
  wm.setAPCallback([this](WiFiManager*) { _portalActive = true; });

  std::string apName = "insomnia-" + getChipId();

  // Attempt to connect or start AP
  if (!wm.autoConnect(apName.c_str())) {
    Serial.println("[insomniaTV] WiFi connection failed and portal timed out");
    ESP.restart();
  }
  _portalActive = false;
  WiFi.setSleep(true);  // modem-sleep between beacons; less regulator heat

  Serial.println("[insomniaTV] WiFi connected");
  Serial.print("[insomniaTV] IP address: ");
  Serial.println(WiFi.localIP());

  ArduinoOTA.setHostname(apName.c_str());
  ArduinoOTA.setPassword(kSystemPassword);
  ArduinoOTA.onStart([]() { Serial.println("OTA Start"); });
  ArduinoOTA.begin();

  pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);
#endif
}

void WifiSetup::handleResetButton() {
#if defined(ARDUINO)
  ArduinoOTA.handle();
  // Active low button
  bool currentVal = (digitalRead(RESET_BUTTON_PIN) == LOW);

  if (currentVal && !_isButtonPressed) {
    _isButtonPressed = true;
    _buttonPressStart = millis();
    Serial.println("[insomniaTV] Reset button pressed...");
  } else if (!currentVal && _isButtonPressed) {
    _isButtonPressed = false;
    _buttonPressStart = 0;
  }

  if (_isButtonPressed && (millis() - _buttonPressStart > 10000)) {
    Serial.println(
        "[insomniaTV] Reset button held for 10s! Clearing WiFi settings...");
    WiFiManager wm;
    wm.resetSettings();
    ESP.restart();
  }
#endif
}

bool WifiSetup::isPortalActive() const {
  return _portalActive;
}

std::string WifiSetup::getChipId() {
#if defined(ARDUINO)
  uint64_t chipid = ESP.getEfuseMac();
  char idStr[13];
  snprintf(idStr, sizeof(idStr), "%04X%08lX",
           static_cast<uint16_t>(chipid >> 32), static_cast<uint32_t>(chipid));

  return std::string(idStr);
#else
  return "NATIVE_CHIP";
#endif
}

}  // namespace InsomniaTV
