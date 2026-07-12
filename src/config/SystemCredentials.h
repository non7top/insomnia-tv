// Copyright 2026 insomniaTV Contributors. All rights reserved.

#ifndef SRC_CONFIG_SYSTEMCREDENTIALS_H_
#define SRC_CONFIG_SYSTEMCREDENTIALS_H_

namespace InsomniaTV {

// Single source of truth for the device's admin credentials, shared by the
// web Basic Auth (WebServer.cpp) and ArduinoOTA (WifiSetup.cpp) -- one
// system password, not two hardcoded literals that can drift apart.
//
// Hardcoded pending #51's proper per-device/NVS-backed credential system.
constexpr const char* kSystemUsername = "admin";
constexpr const char* kSystemPassword = "insomnia";

}  // namespace InsomniaTV

#endif  // SRC_CONFIG_SYSTEMCREDENTIALS_H_
