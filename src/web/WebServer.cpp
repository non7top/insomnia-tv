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
  _server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (!request->authenticate("admin", "insomnia")) {
      return request->requestAuthentication();
    }
    // Serve embedded index.html
    static const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>insomniaTV</title>
    <style>
        body { font-family: sans-serif; margin: 0; padding: 0; }
        .tab { overflow: hidden; background-color: #f1f1f1; }
        .tab button { background-color: inherit; border: none; outline: none; cursor: pointer; padding: 14px 16px; transition: 0.3s; }
        .tab button:hover { background-color: #ddd; }
        .tab button.active { background-color: #ccc; }
        .tabcontent { display: none; padding: 6px 12px; border: 1px solid #ccc; border-top: none; }
    </style>
</head>
<body>

<div class="tab">
  <button class="tablinks" onclick="openTab(event, 'Status')">Status</button>
  <button class="tablinks" onclick="openTab(event, 'Config')">Config</button>
</div>

<div id="Status" class="tabcontent">
  <h3>System Status</h3>
  <div id="status-content">Loading...</div>
</div>

<div id="Config" class="tabcontent">
  <h3>WiFi & TV Config</h3>
  <button onclick="scanTV()">Scan for TVs</button>
  <div id="discovery-content"></div>
</div>

<script>
function openTab(evt, tabName) {
    var i, tabcontent, tablinks;
    tabcontent = document.getElementsByClassName("tabcontent");
    for (i = 0; i < tabcontent.length; i++) { tabcontent[i].style.display = "none"; }
    tablinks = document.getElementsByClassName("tablinks");
    for (i = 0; i < tablinks.length; i++) { tablinks[i].className = tablinks[i].className.replace(" active", ""); }
    document.getElementById(tabName).style.display = "block";
    evt.currentTarget.className += " active";
}
function scanTV() {
    document.getElementById('discovery-content').innerText = 'Scanning...';
    fetch('/api/scan').then(r => r.json()).then(data => {
        document.getElementById('discovery-content').innerText = JSON.stringify(data);
    });
}
// Default open Status tab
document.getElementsByClassName('tablinks')[0].click();
</script>

</body>
</html>
)rawliteral";
    request->send(200, "text/html", index_html);
  });
...

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
