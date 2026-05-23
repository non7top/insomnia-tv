/*
 * insomniaTV - ESP32 Smart IR Sleep Controller
 *
 * Main application entry point
 * Copyright 2026 insomniaTV Contributors
 */

#ifndef UNIT_TEST

#include <Arduino.h>
#include <Ticker.h>

#include <cstring>
#include <string>

#include "config/ConfigManager.h"
#include "discovery/SamsungTvDiscovery.h"
#include "hal/SystemClock.h"
#include "network/WifiSetup.h"
#include "sensors/SensorManager.h"
#include "state/SleepStateMachine.h"
#include "tv/TvStateMachine.h"
#include "web/WebServer.h"

// Onboard LED (active-low) for nologo C3 super mini
#define STATUS_LED_PIN 8

static InsomniaTV::ConfigManager configMgr;
static InsomniaTV::WifiSetup wifiSetup;
static InsomniaTV::SamsungTvDiscovery tvDiscovery;
static InsomniaTV::WebServer webServer(80, tvDiscovery, configMgr);
static Ticker heartbeat;
static InsomniaTV::SystemClock systemClock;
static InsomniaTV::TvStateMachine* tvSm = nullptr;
static InsomniaTV::SleepStateMachine* sleepSm = nullptr;

void toggleStatusLed() {
  digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
}

void setup() {
  Serial.begin(115200);
  // Wait for Serial on USB boards
  uint32_t start = millis();
  while (!Serial && (millis() - start) < 2000) {
  }

  Serial.println("\n[insomniaTV] Initializing...");

  // Status LED heartbeat
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, HIGH);  // Off
  heartbeat.attach(0.5, toggleStatusLed);

  // Configuration
  Serial.println("[insomniaTV] Loading configuration...");
  InsomniaTV::ConfigStatus status = configMgr.load();
  if (status != InsomniaTV::ConfigStatus::Ok) {
    Serial.printf("[insomniaTV] Config load failed (%d), using defaults\n",
                  static_cast<int>(status));
  }

  // Validate current config
  std::string validationErr;
  if (!InsomniaTV::ConfigManager::validate(configMgr.get(), validationErr)) {
    Serial.printf("[insomniaTV] Config validation error: %s\n",
                  validationErr.c_str());
  }

  Serial.println("[insomniaTV] Initialization complete");

  // WiFi Setup
  wifiSetup.begin();

  // Sensor Framework
  Serial.println("[insomniaTV] Initializing sensor framework...");
  InsomniaTV::SensorManager::instance().init(
      configMgr.get().sensorsJson.c_str(), tvDiscovery);

  // TV State Machine — restore detection-sensor config from stored JSON
  Serial.println("[insomniaTV] Initializing TV state machine...");
  tvSm = new InsomniaTV::TvStateMachine(InsomniaTV::SensorManager::instance());
  {
    JsonDocument sensorsDoc;
    deserializeJson(sensorsDoc, configMgr.get().sensorsJson);
    JsonDocument tvDoc;
    tvDoc["hysteresis_count"] = 2;
    JsonArray detArr = tvDoc["detection_sensors"].to<JsonArray>();
    for (JsonObject s : sensorsDoc.as<JsonArray>()) {
      const char* t = s["type"] | "";
      int w = (strcmp(t, "upnp") == 0) ? 4 : (strcmp(t, "ping") == 0) ? 3 : 2;
      JsonObject ds = detArr.add<JsonObject>();
      ds["sensor_id"] = s["id"] | "";
      ds["weight"] = w;
      ds["enabled"] = true;
    }
    tvSm->begin(tvDoc);
  }

  // Sleep State Machine — inactivity timeout from behavior config
  uint32_t timeoutMs =
      static_cast<uint32_t>(configMgr.get().inactivityTimeoutMin) * 60000UL;
  sleepSm = new InsomniaTV::SleepStateMachine(systemClock, timeoutMs);
  sleepSm->setTvStateMachine(tvSm);

  webServer.setTvStateMachine(tvSm);

  // Web Server
  webServer.begin();
}

void loop() {
  wifiSetup.handleResetButton();
  InsomniaTV::SensorManager::instance().tick();
  if (sleepSm != nullptr) {
    sleepSm->tick();
  }
  delay(100);
}

#endif  // UNIT_TEST
