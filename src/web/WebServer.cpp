// Copyright 2026 insomniaTV Contributors. All rights reserved.

#include "WebServer.h"

#include <ArduinoJson.h>

#include <string>

namespace InsomniaTV {

#if defined(ARDUINO)
WebServer::WebServer(uint16_t port, SamsungTvDiscovery& discovery)
    : _server(port), _discovery(discovery) {
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
    request->send(200, "application/json", "{\"status\":\"ok\"}");
  });

  _server.on("/api/scan", HTTP_GET, [this](AsyncWebServerRequest* request) {
    if (!request->authenticate("admin", "insomnia")) {
      return request->requestAuthentication();
    }

    // Run scan in a separate task to avoid blocking AsyncTCP
    // and causing stack overflow
    xTaskCreate(
        [](void* pvParameters) {
          auto* self = static_cast<WebServer*>(pvParameters);
          self->_discovery.clear();
          self->_discovery.scan();
          vTaskDelete(NULL);
        },
        "discovery_task", 4096, this, 1, NULL);

    request->send(200, "application/json", "{\"status\":\"scanning\"}");
  });

  _server.on("/api/discovery", HTTP_GET,
             [this](AsyncWebServerRequest* request) {
               if (!request->authenticate("admin", "insomnia")) {
                 return request->requestAuthentication();
               }

               JsonDocument doc;
               JsonArray array = doc.to<JsonArray>();
               for (const auto& tv : _discovery.getDiscoveredTvs()) {
                 JsonObject obj = array.add<JsonObject>();
                 obj["name"] = tv.name;
                 obj["model"] = tv.model;
                 obj["ip"] = tv.ip;
               }

               String response;
               serializeJson(doc, response);
               request->send(200, "application/json", response);
             });

  _server.on("/discovery", HTTP_GET, [this](AsyncWebServerRequest* request) {
    if (!request->authenticate("admin", "insomnia")) {
      return request->requestAuthentication();
    }
    std::string html = "<h1>Samsung TV Discovery</h1>";
    if (_discovery.isScanning()) {
      html += "<p>Scan in progress... <a href='/discovery'>Refresh</a></p>";
    } else {
      html += "<ul>";
      for (const auto& tv : _discovery.getDiscoveredTvs()) {
        html += "<li><b>Name:</b> " + tv.name + " | <b>Model:</b> " + tv.model +
                " | <b>IP:</b> " + tv.ip + "</li>";
      }
      html += "</ul><p><a href='/api/scan'>Start New Scan</a></p>";
    }
    request->send(200, "text/html", html.c_str());
  });

  _server.on("/wifi", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (!request->authenticate("admin", "insomnia")) {
      return request->requestAuthentication();
    }
    request->send(200, "text/html",
                  "<h1>WiFi Configuration</h1><p>Placeholder for WiFi "
                  "setup. Use physical button (10s) to reset.</p>");
  });
}
#else
WebServer::WebServer(uint16_t port, SamsungTvDiscovery& discovery)
    : _discovery(discovery) {}
void WebServer::begin() {}
void WebServer::setupRoutes() {}
#endif

}  // namespace InsomniaTV
