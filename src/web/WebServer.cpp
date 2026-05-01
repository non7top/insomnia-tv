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
    // Placeholder JSON response
    request->send(200, "application/json", "{\"status\":\"ok\"}");
  });
}
#else
WebServer::WebServer(uint16_t port) {}
void WebServer::begin() {}
void WebServer::setupRoutes() {}
#endif

}  // namespace InsomniaTV
