// Copyright 2026 insomniaTV Contributors. All rights reserved.

#include "WebServer.h"

#include <ArduinoJson.h>
#if defined(ARDUINO)
#include <WiFi.h>
#endif
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
  <div id="status-content">
    <p><b>Status:</b> <span id="sys-status">Loading...</span></p>
    <p><b>Version:</b> <span id="sys-version"></span></p>
    <p><b>MAC:</b> <span id="sys-mac"></span></p>
    <p><b>SSID:</b> <span id="sys-ssid"></span></p>
    <p><b>RSSI:</b> <span id="sys-rssi"></span> dBm</p>
    <p><b>IP Address:</b> <span id="sys-ip"></span></p>
    <p><b>DHCP:</b> <span id="sys-dhcp"></span></p>
    <p><b>AP Name:</b> <span id="sys-ap"></span></p>
    <p><b>Chip ID:</b> <span id="sys-chipid"></span></p>
    <p><b>Free Heap:</b> <span id="sys-heap"></span></p>
    <p><b>WiFi Status:</b> <span id="sys-wifi"></span></p>
  </div>
</div>

<div id="Config" class="tabcontent">
  <h3>WiFi & TV Config</h3>
  <button onclick="scanTV()">Scan for TVs</button>
  <div id="discovery-content">
      <h4>Discovered TVs:</h4>
      <ul id="tv-list"></ul>
  </div>
</div>

<script>
function openTab(evt, tabName) {
    var i, tabcontent, tablinks;
    tabcontent = document.getElementsByClassName("tabcontent");
    for (i = 0; i < tabcontent.length; i++) { tabcontent[i].style.display = "none"; }
    tablinks = document.getElementsByClassName("tablinks");
    for (i = 0; i < tablinks.length; i++) { tablinks[i].className = tablinks[i].className.replace(" active", ""); }

    if (tabName === 'Status') {
        fetch('/api/status').then(r => r.json()).then(data => {
            for (const [key, value] of Object.entries(data)) {
                const el = document.getElementById('sys-' + key);
                if (el) el.textContent = value;
            }
        }).catch(err => {
            document.getElementById('sys-status').textContent = 'Error: ' + err;
        });
    } else if (tabName === 'Config') {
        loadDiscovery();
    }

    document.getElementById(tabName).style.display = "block";
    evt.currentTarget.className += " active";
}
function scanTV() {
    document.getElementById('tv-list').textContent = 'Scanning...';
    fetch('/api/scan').then(r => r.json()).then(data => {
        setTimeout(loadDiscovery, 3000);
    }).catch(err => {
        document.getElementById('tv-list').textContent = 'Error: ' + err;
    });
}
function loadDiscovery() {
    fetch('/api/discovery').then(r => r.json()).then(renderDiscovery).catch(err => {
        document.getElementById('tv-list').textContent = 'Error: ' + err;
    });
}
function renderDiscovery(data) {
    let list = document.getElementById('tv-list');
    list.innerHTML = '';
    data.forEach(tv => {
        let li = document.createElement('li');
        li.innerHTML = `<b>Name:</b> ${tv.name} | <b>Model:</b> ${tv.model} | <b>IP:</b> ${tv.ip}`;
        list.appendChild(li);
    });
}
// Default open Status tab
document.getElementsByClassName('tablinks')[0].click();
</script>

</body>
</html>
)rawliteral";
    AsyncWebServerResponse* res =
        request->beginResponse(200, "text/html", index_html);
    res->addHeader("Cache-Control", "no-store, no-cache, must-revalidate");
    request->send(res);
  });

  _server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (!request->authenticate("admin", "insomnia")) {
      return request->requestAuthentication();
    }
    JsonDocument doc;
    doc["status"] = "ok";
    doc["ip"] = WiFi.localIP().toString();
    doc["mac"] = WiFi.macAddress().c_str();
    doc["ssid"] = WiFi.SSID();
    doc["rssi"] = WiFi.RSSI();
    doc["dhcp"] = WiFi.getMode() == WIFI_STA ? "enabled" : "disabled";
    doc["ap"] = WiFi.softAPSSID();
    doc["version"] = INSOMNIATV_VERSION;
    doc["chipId"] = ESP.getEfuseMac();
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["wifiStatus"] =
        WiFi.status() == WL_CONNECTED ? "connected" : "disconnected";

    String response;
    serializeJson(doc, response);
    AsyncWebServerResponse* res =
        request->beginResponse(200, "application/json", response);
    res->addHeader("Cache-Control", "no-store, no-cache, must-revalidate");
    request->send(res);
  });

  _server.on("/api/scan", HTTP_GET, [this](AsyncWebServerRequest* request) {
    if (!request->authenticate("admin", "insomnia")) {
      return request->requestAuthentication();
    }

    // Run scan in a separate task
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
