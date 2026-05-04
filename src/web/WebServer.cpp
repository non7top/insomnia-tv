// Copyright 2026 insomniaTV Contributors. All rights reserved.

#include "WebServer.h"

#include <ArduinoJson.h>

#include <string>

#if defined(ARDUINO)
#include <AsyncEventSource.h>
#include <Update.h>
#include <WiFi.h>
#endif

#include "../sensors/SensorManager.h"

namespace InsomniaTV {

#if defined(ARDUINO)
AsyncEventSource events("/events");

WebServer::WebServer(uint16_t port, SamsungTvDiscovery& discovery)
    : _server(port), _discovery(discovery) {
  setupRoutes();
}

void WebServer::begin() {
  _server.addHandler(&events);
  _server.begin();

  // Periodic sensor reader task
  xTaskCreate(
      [](void* pvParameters) {
        for (;;) {
          auto sensors = SensorManager::instance().listSensors();
          for (auto const& s : sensors) {
            bool val = s->read();
            JsonDocument doc;
            doc["id"] = s->getId();
            doc["value"] = val;
            doc["available"] = s->isAvailable();
            String buf;
            serializeJson(doc, buf);
            events.send(buf.c_str(), "sensor_update", millis());
          }
          vTaskDelay(pdMS_TO_TICKS(5000));
        }
      },
      "sensor_poll", 4096, NULL, 1, NULL);
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
        .tabcontent { display: none; padding: 12px; border: 1px solid #ccc; border-top: none; }
        fieldset { margin-bottom: 15px; border: 1px solid #ccc; padding: 10px; }
        legend { font-weight: bold; }
        table { width: 100%; border-collapse: collapse; }
        th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }
        tr:nth-child(even) { background-color: #f2f2f2; }
    </style>
</head>
<body>

<div class="tab">
  <button class="tablinks" onclick="openTab(event, 'Status')">Status</button>
  <button class="tablinks" onclick="openTab(event, 'Config')">Config</button>
  <button class="tablinks" onclick="openTab(event, 'Sensors')">Sensors</button>
</div>

<div id="Status" class="tabcontent">
  <h3>System Status</h3>
  <fieldset>
    <legend>System</legend>
    <p><b>Status:</b> <span id="sys-status">Loading...</span></p>
    <p><b>Version:</b> <span id="sys-version"></span></p>
    <p><b>Chip ID:</b> <span id="sys-chipId"></span></p>
    <p><b>Free Heap:</b> <span id="sys-freeHeap"></span></p>
  </fieldset>
  <fieldset>
    <legend>Network</legend>
    <p><b>IP Address:</b> <span id="sys-ip"></span></p>
    <p><b>MAC:</b> <span id="sys-mac"></span></p>
    <p><b>DHCP:</b> <span id="sys-dhcp"></span></p>
    <p><b>AP Name:</b> <span id="sys-ap"></span></p>
  </fieldset>
  <fieldset>
    <legend>WiFi</legend>
    <p><b>Status:</b> <span id="sys-wifiStatus"></span></p>
    <p><b>SSID:</b> <span id="sys-ssid"></span></p>
    <p><b>RSSI:</b> <span id="sys-rssi"></span> dBm</p>
  </fieldset>
</div>

<div id="Config" class="tabcontent">
  <h3>WiFi & TV Config</h3>
  <button onclick="scanTV()">Scan for TVs</button>
  <div id="discovery-content">
      <h4>Discovered TVs:</h4>
      <ul id="tv-list"></ul>
  </div>
  <h3>Firmware Update</h3>
  <form method="POST" action="/update" enctype="multipart/form-data">
    <input type="file" name="update">
    <input type="submit" value="Update">
  </form>
</div>

<div id="Sensors" class="tabcontent">
  <h3>Sensor Registry</h3>
  <table>
    <thead>
      <tr>
        <th>ID</th>
        <th>Type</th>
        <th>Value</th>
        <th>Status</th>
        <th>Actions</th>
      </tr>
    </thead>
    <tbody id="sensor-table-body">
    </tbody>
  </table>
</div>

<script>
const source = new EventSource('/events');
source.addEventListener('sensor_update', function(e) {
    const data = JSON.parse(e.data);
    const row = document.getElementById('sensor-row-' + data.id);
    if (row) {
        row.cells[2].textContent = data.value;
        row.cells[3].textContent = data.available ? '🟢' : '🔴';
    }
}, false);

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
    } else if (tabName === 'Sensors') {
        loadSensors();
    }

    document.getElementById(tabName).style.display = "block";
    evt.currentTarget.className += " active";
}

function loadSensors() {
    fetch('/api/sensors').then(r => r.json()).then(data => {
        const tbody = document.getElementById('sensor-table-body');
        tbody.innerHTML = '';
        data.forEach(s => {
            const tr = document.createElement('tr');
            tr.id = 'sensor-row-' + s.id;
            tr.innerHTML = `<td>${s.id}</td><td>${s.type}</td><td>${s.value}</td><td>${s.available ? '🟢' : '🔴'}</td><td><button onclick="testSensor('${s.id}')">Test</button></td>`;
            tbody.appendChild(tr);
        });
    }).catch(err => {
        console.error('Error loading sensors:', err);
    });
}

function testSensor(id) {
    fetch('/api/sensors/test', {
        method: 'POST',
        headers: {'Content-Type': 'application/x-www-form-urlencoded'},
        body: 'id=' + id
    }).then(r => r.json()).then(data => {
        alert('Test result for ' + data.id + ': ' + data.value);
    }).catch(err => {
        alert('Test failed: ' + err);
    });
}

function scanTV() {
    document.getElementById('tv-list').textContent = 'Scanning...';
    fetch('/api/scan', {method: 'POST'}).then(r => r.json()).then(data => {
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

  _server.on("/api/sensors", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (!request->authenticate("admin", "insomnia")) {
      return request->requestAuthentication();
    }
    auto sensors = SensorManager::instance().listSensors();
    JsonDocument doc;
    JsonArray array = doc.to<JsonArray>();
    for (auto const& sensor : sensors) {
      JsonObject obj = array.add<JsonObject>();
      obj["id"] = sensor->getId();
      obj["type"] = sensor->getType();
      obj["available"] = sensor->isAvailable();
      // Use config to represent type-specific metadata if needed
      obj["value"] = sensor->read();
    }
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  _server.on(
      "/api/sensors/test", HTTP_POST, [](AsyncWebServerRequest* request) {
        if (!request->authenticate("admin", "insomnia")) {
          return request->requestAuthentication();
        }
        if (request->hasParam("id", true)) {
          String id = request->getParam("id", true)->value();
          auto sensor = SensorManager::instance().getSensor(id.c_str());
          if (sensor) {
            bool result = sensor->read();
            String response = "{\"id\":\"" + id + "\", \"value\":" +
                              (result ? "true" : "false") + "}";
            request->send(200, "application/json", response);
            return;
          }
        }
        request->send(404, "application/json", "{\"error\":\"Not found\"}");
      });

  _server.on(
      "/update", HTTP_POST,
      [](AsyncWebServerRequest* request) {
        bool shouldReboot = !Update.hasError();
        AsyncWebServerResponse* res = request->beginResponse(
            200, "text/plain", shouldReboot ? "OK" : "FAIL");
        res->addHeader("Connection", "close");
        request->send(res);
        if (shouldReboot)
          ESP.restart();
      },
      [](AsyncWebServerRequest* request, String filename, size_t index,
         uint8_t* data, size_t len, bool final) {
        if (!index) {
          if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            Update.printError(Serial);
          }
        }
        if (Update.write(data, len) != len) {
          Update.printError(Serial);
        }
        if (final) {
          if (!Update.end(true)) {
            Update.printError(Serial);
          }
        }
      });

  _server.on("/api/scan", HTTP_POST, [this](AsyncWebServerRequest* request) {
    if (!request->authenticate("admin", "insomnia")) {
      return request->requestAuthentication();
    }
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
}
#else
WebServer::WebServer(uint16_t port, SamsungTvDiscovery& discovery)
    : _discovery(discovery) {}
void WebServer::begin() {}
void WebServer::setupRoutes() {}
#endif

}  // namespace InsomniaTV
