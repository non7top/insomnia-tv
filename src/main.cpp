// Copyright 2026 insomniaTV Contributors. All rights reserved.

#ifndef INSOMNIATV_NATIVE

#include <Arduino.h>

#include "config/ConfigManager.h"

static InsomniaTV::ConfigManager configMgr;

void setup() {
  Serial.begin(115200);
  Serial.println("[insomniaTV] booting...");

  InsomniaTV::ConfigStatus status = configMgr.load();
  if (status != InsomniaTV::ConfigStatus::Ok) {
    Serial.printf("[insomniaTV] config load failed: %d, using defaults\n",
                  static_cast<int>(status));
    configMgr.resetToDefaults();
  }

  String err;
  if (!InsomniaTV::ConfigManager::validate(configMgr.get(), err)) {
    Serial.printf("[insomniaTV] config validation error: %s\n", err.c_str());
  }

  Serial.println("[insomniaTV] initialization complete");
}

void loop() {
  // Phase 1: minimal loop -- tasks in later phases
  delay(1000);
}

#endif  // INSOMNIATV_NATIVE
