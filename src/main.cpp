/*
 * insomniaTV - ESP32 Smart IR Sleep Controller
 *
 * Main application entry point
 * Copyright 2026 insomniaTV Contributors
 */

#ifndef UNIT_TEST

#include <Arduino.h>
#include <Ticker.h>

#include "config/ConfigManager.h"

// Onboard LED (active-low) for nologo C3 super mini
#define STATUS_LED_PIN 8

static InsomniaTV::ConfigManager configMgr;
static Ticker heartbeat;

void toggleStatusLed() {
  digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
}

void setup() {
  Serial.begin(115200);
  // Wait for Serial on USB boards
  uint32_t start = millis();
  while (!Serial && (millis() - start) < 2000) { ; }

  Serial.println("\n[insomniaTV] Initializing...");

  // Status LED heartbeat
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, HIGH); // Off
  heartbeat.attach(0.5, toggleStatusLed);

  // Configuration
  Serial.println("[insomniaTV] Loading configuration...");
  InsomniaTV::ConfigStatus status = configMgr.load();
  if (status != InsomniaTV::ConfigStatus::Ok) {
    Serial.printf("[insomniaTV] Config load failed (%d), using defaults\n", (int)status);
  }

  // Validate current config
  std::string validationErr;
  if (!InsomniaTV::ConfigManager::validate(configMgr.get(), validationErr)) {
    Serial.printf("[insomniaTV] Config validation error: %s\n", validationErr.c_str());
  }

  Serial.println("[insomniaTV] Initialization complete");
}

void loop() {
  // Phase 1: minimal loop
  // Later phases will add task scheduling here
  delay(1000);
}

#endif // UNIT_TEST
