#include <Arduino.h>
#include "config/ConfigManager.h"

using namespace InsomniaTV;

static ConfigManager configMgr;

void setup() {
    Serial.begin(115200);
    Serial.println("[insomniaTV] booting...");

    ConfigStatus status = configMgr.load();
    if (status != ConfigStatus::Ok) {
        Serial.printf("[insomniaTV] config load failed: %d, using defaults\n", static_cast<int>(status));
        configMgr.resetToDefaults();
    }

    String err;
    if (!ConfigManager::validate(configMgr.get(), err)) {
        Serial.printf("[insomniaTV] config validation error: %s\n", err.c_str());
    }

    Serial.println("[insomniaTV] initialization complete");
}

void loop() {
    // Phase 1: minimal loop — FreeRTOS tasks and IR drivers in later phases
    delay(1000);
}
