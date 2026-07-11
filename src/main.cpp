/*
 * insomniaTV - ESP32 Smart IR Sleep Controller
 *
 * Main application entry point
 * Copyright 2026 insomniaTV Contributors
 */

#ifndef UNIT_TEST

#include <Arduino.h>
#include <LittleFS.h>

#include <cmath>
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

static constexpr int kStatusLedFreqHz = 5000;
static constexpr int kStatusLedResolutionBits = 8;
static constexpr uint32_t kStatusLedBreathPeriodMs = 4000;
// Kept low (out of 255) so the heartbeat reads as a faint pulse, not a blink.
static constexpr int kStatusLedMaxBrightness = 60;

static InsomniaTV::ConfigManager configMgr;
static InsomniaTV::WifiSetup wifiSetup;
static InsomniaTV::SamsungTvDiscovery tvDiscovery;
static InsomniaTV::WebServer webServer(80, tvDiscovery, configMgr);
static InsomniaTV::SystemClock systemClock;
static InsomniaTV::TvStateMachine* tvSm = nullptr;
static InsomniaTV::SleepStateMachine* sleepSm = nullptr;

void updateStatusLed() {
  float phase = (millis() % kStatusLedBreathPeriodMs) /
                static_cast<float>(kStatusLedBreathPeriodMs);
  float envelope =
      (1.0f - cosf(2.0f * static_cast<float>(M_PI) * phase)) / 2.0f;
  int brightness = static_cast<int>(envelope * kStatusLedMaxBrightness);
  ledcWrite(STATUS_LED_PIN, 255 - brightness);  // active-low
}

// Quick "bip-bip-bip-bip" blink while the WiFi config portal is waiting for
// setup, vs. the slow breathing heartbeat during normal operation.
void updatePortalBlinkLed() {
  bool on = (millis() / 150) % 2 == 0;
  ledcWrite(STATUS_LED_PIN, on ? 0 : 255);  // active-low
}

// Runs on its own task so the LED stays responsive even when loop() stalls
// on blocking sensor I/O (e.g. HttpSensor's 5s timeout).
void statusLedTask(void*) {
  for (;;) {
    if (wifiSetup.isPortalActive()) {
      updatePortalBlinkLed();
    } else {
      updateStatusLed();
    }
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

void setup() {
  // Lower power draw (less heat through the onboard 5V regulator); this
  // workload is nowhere near CPU-bound.
  setCpuFrequencyMhz(80);

  Serial.begin(115200);
  // Wait for Serial on USB boards
  uint32_t start = millis();
  while (!Serial && (millis() - start) < 2000) {
  }

  Serial.println("\n[insomniaTV] Initializing...");

  // Status LED — slow breathing heartbeat (active-low), on its own task
  ledcAttach(STATUS_LED_PIN, kStatusLedFreqHz, kStatusLedResolutionBits);
  ledcWrite(STATUS_LED_PIN, 255);  // start off
  xTaskCreate(statusLedTask, "status_led", 2048, NULL, 1, NULL);

  // Filesystem — must be mounted before any config or file-manager access
  if (!LittleFS.begin(true)) {
    Serial.println("[insomniaTV] LittleFS mount failed");
  }

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
