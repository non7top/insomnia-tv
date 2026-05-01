// Copyright 2026 insomniaTV Contributors. All rights reserved.

#include "WebServer.h"

#include <ArduinoJson.h>

namespace InsomniaTV {

#if defined(ARDUINO)
WebServer::WebServer(uint16_t port) : _server(port) {
  setupRoutes();
}

void WebServer::begin() {
  _server.begin();
}

void WebServer::setupRoutes() {
  _server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (!request->authenticate("admin", "insomnia")) {
      return request->requestAuthentication();
    }
    // Placeholder JSON response
    request->send(200, "application/json", "{\"status\":\"ok\"}");
  });

  _server.on("/wifi", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (!request->authenticate("admin", "insomnia")) {
      return request->requestAuthentication();
    }
    request->send(200, "text/html",
                  "<h1>WiFi Configuration</h1><p>Placeholder for WiFi setup. "
                  "Use physical button (10s) to reset.</p>");
  });
}

#else
WebServer::WebServer(uint16_t port) {}
void WebServer::begin() {}
void WebServer::setupRoutes() {}
#endif

}  // namespace InsomniaTV
