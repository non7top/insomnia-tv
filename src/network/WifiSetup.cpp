// Copyright 2026 insomniaTV Contributors. All rights reserved.

#include "WifiSetup.h"

#include <cstdio>
#include <string>

#if defined(ARDUINO)
#include <ArduinoOTA.h>
#include <WiFi.h>
#include <WiFiManager.h>
#endif

namespace InsomniaTV {

#if defined(ARDUINO)
// GPIO 9 is the BOOT button on most C3 modules
static const int RESET_BUTTON_PIN = 9;
#endif

WifiSetup::WifiSetup() : _buttonPressStart(0), _isButtonPressed(false) {}

void WifiSetup::begin() {
#if defined(ARDUINO)
  WiFiManager wm;

  std::string apName = "insomnia-" + getChipId();

  // Attempt to connect or start AP
  if (!wm.autoConnect(apName.c_str())) {
    Serial.println("[insomniaTV] WiFi connection failed and portal timed out");
    ESP.restart();
  }

  Serial.println("[insomniaTV] WiFi connected");
  Serial.print("[insomniaTV] IP address: ");
  Serial.println(WiFi.localIP());

  ArduinoOTA.setHostname(apName.c_str());
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
