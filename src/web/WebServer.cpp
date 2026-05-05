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

WebServer::WebServer(uint16_t port, SamsungTvDiscovery& discovery,
                     ConfigManager& configMgr)
    : _server(port), _discovery(discovery), _configMgr(configMgr) {
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
        body { font-family: sans-serif; margin: 0; padding: 0; background: #f4f4f9; color: #333; }
        .tab { overflow: hidden; background-color: #333; }
        .tab button { background-color: inherit; border: none; outline: none; cursor: pointer; padding: 14px 16px; transition: 0.3s; color: white; }
        .tab button:hover { background-color: #555; }
        .tab button.active { background-color: #4CAF50; }
        .tabcontent { display: none; padding: 20px; }
        fieldset { margin-bottom: 15px; border: 1px solid #ccc; padding: 15px; background: white; border-radius: 5px; }
        legend { font-weight: bold; padding: 0 5px; }
        table { width: 100%; border-collapse: collapse; background: white; border-radius: 5px; overflow: hidden; }
        th, td { border: 1px solid #ddd; padding: 12px; text-align: left; }
        tr:nth-child(even) { background-color: #f9f9f9; }
        button { cursor: pointer; padding: 8px 12px; border: none; border-radius: 4px; background: #4CAF50; color: white; }
        button:hover { background: #45a049; }
        button.delete { background: #f44336; }
        button.delete:hover { background: #da190b; }
        .modal { display:none; position:fixed; z-index:100; left:0; top:0; width:100%; height:100%; background-color:rgba(0,0,0,0.5); }
        .modal-content { background-color:#fefefe; margin:10% auto; padding:25px; border:1px solid #888; width:80%; max-width: 500px; border-radius: 8px; }
        .close { float:right; cursor:pointer; font-size: 24px; }
        label { display: block; margin: 10px 0 5px; font-weight: bold; }
        input[type=text], input[type=number], select { width: 100%; padding: 8px; border: 1px solid #ccc; border-radius: 4px; box-sizing: border-box; }
        .discovery-item { padding: 8px; border-bottom: 1px solid #eee; cursor: pointer; }
        .discovery-item:hover { background: #f0f0f0; }
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
</div>

<div id="Config" class="tabcontent">
  <h3>TV Discovery</h3>
  <button onclick="scanTV()">Scan Network</button>
  <ul id="tv-list"></ul>
  <h3>Firmware Update</h3>
  <form method="POST" action="/update" enctype="multipart/form-data">
    <input type="file" name="update">
    <input type="submit" value="Update">
  </form>
</div>

<div id="Sensors" class="tabcontent">
  <h3>Sensor Registry</h3>
  <button onclick="openModal()" style="margin-bottom: 15px;">Add New Sensor</button>
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

<div id="sensorModal" class="modal">
  <div class="modal-content">
    <span onclick="closeModal()" class="close">&times;</span>
    <h3>Add Sensor</h3>
    <form id="sensorForm">
      <label>Unique ID:</label>
      <input type="text" id="sensorId" placeholder="e.g., room_motion" required>

      <label>Sensor Type:</label>
      <select id="sensorType" onchange="updateFormFields()">
        <option value="gpio_input">GPIO Input (Digital)</option>
        <option value="gpio_analog">GPIO Analog (ADC)</option>
        <option value="ping">Ping (ICMP)</option>
        <option value="http">HTTP (GET)</option>
        <option value="upnp">UPnP (Discovery)</option>
      </select>

      <div id="dynamicFields"></div>

      <button type="button" onclick="saveSensor()" style="margin-top: 20px; width: 100%;">Save Sensor</button>
    </form>
  </div>
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

    document.getElementById(tabName).style.display = "block";
    evt.currentTarget.className += " active";

    if (tabName === 'Status') loadStatus();
    if (tabName === 'Sensors') loadSensors();
}

function loadStatus() {
    fetch('/api/status').then(r => r.json()).then(data => {
        for (const [key, value] of Object.entries(data)) {
            const el = document.getElementById('sys-' + key);
            if (el) el.textContent = value;
        }
    });
}

function loadSensors() {
    fetch('/api/sensors').then(r => r.json()).then(data => {
        const tbody = document.getElementById('sensor-table-body');
        tbody.innerHTML = '';
        data.forEach(s => {
            const tr = document.createElement('tr');
            tr.id = 'sensor-row-' + s.id;
            tr.innerHTML = `
                <td>${s.id}</td>
                <td>${s.type}</td>
                <td>${s.value}</td>
                <td>${s.available ? '🟢' : '🔴'}</td>
                <td>
                    <button onclick="testSensor('${s.id}')">Test</button>
                    <button class="delete" onclick="deleteSensor('${s.id}')">Delete</button>
                </td>`;
            tbody.appendChild(tr);
        });
    });
}

function openModal() { document.getElementById('sensorModal').style.display = 'block'; updateFormFields(); }
function closeModal() { document.getElementById('sensorModal').style.display = 'none'; }

function updateFormFields() {
    const type = document.getElementById('sensorType').value;
    const fields = document.getElementById('dynamicFields');
    fields.innerHTML = '';

    if (type === 'gpio_input') {
        fields.innerHTML = `
            <label>Pin Number:</label><input type="number" id="s_pin" value="0">
            <label>Pullup:</label><input type="checkbox" id="s_pullup" checked>
            <label>Debounce (ms):</label><input type="number" id="s_debounce" value="50">`;
    } else if (type === 'gpio_analog') {
        fields.innerHTML = `
            <label>Pin Number:</label><input type="number" id="s_pin" value="0">
            <label>Scale:</label><input type="number" id="s_scale" step="0.001" value="1.0">
            <label>Offset:</label><input type="number" id="s_offset" step="0.001" value="0.0">`;
    } else if (type === 'ping') {
        fields.innerHTML = `<label>Target IP/Host:</label><input type="text" id="s_target" placeholder="192.168.1.1">`;
    } else if (type === 'http') {
        fields.innerHTML = `<label>URL:</label><input type="text" id="s_url" placeholder="http://api.local/status">`;
    } else if (type === 'upnp') {
        fields.innerHTML = `
            <label>Select Device:</label>
            <div id="upnp-list" style="border: 1px solid #ccc; max-height: 100px; overflow-y: auto;">Scanning...</div>
            <input type="hidden" id="s_target_name">`;
        fetchUpnpDevices();
    }
}

function fetchUpnpDevices() {
    fetch('/api/discovery').then(r => r.json()).then(data => {
        const list = document.getElementById('upnp-list');
        list.innerHTML = '';
        if (data.length === 0) list.innerHTML = '<div class="discovery-item">No TVs found. Scan in Config tab first.</div>';
        data.forEach(tv => {
            const div = document.createElement('div');
            div.className = 'discovery-item';
            div.textContent = `${tv.name} (${tv.ip})`;
            div.onclick = () => {
                document.getElementById('s_target_name').value = tv.name;
                Array.from(list.children).forEach(c => c.style.background = '');
                div.style.background = '#e0e0e0';
            };
            list.appendChild(div);
        });
    });
}

function saveSensor() {
    const id = document.getElementById('sensorId').value;
    const type = document.getElementById('sensorType').value;
    if (!id) return alert('ID is required');

    const config = { id, type };
    if (type === 'gpio_input') {
        config.pin = parseInt(document.getElementById('s_pin').value);
        config.pullup = document.getElementById('s_pullup').checked;
        config.debounce_ms = parseInt(document.getElementById('s_debounce').value);
    } else if (type === 'gpio_analog') {
        config.pin = parseInt(document.getElementById('s_pin').value);
        config.scale = parseFloat(document.getElementById('s_scale').value);
        config.offset = parseFloat(document.getElementById('s_offset').value);
    } else if (type === 'ping') {
        config.target_ip = document.getElementById('s_target').value;
    } else if (type === 'http') {
        config.url = document.getElementById('s_url').value;
    } else if (type === 'upnp') {
        config.target_name = document.getElementById('s_target_name').value;
    }

    fetch('/api/sensors', {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify(config)
    }).then(r => {
        if (r.ok) { closeModal(); loadSensors(); }
        else alert('Failed to save');
    });
}

function deleteSensor(id) {
    if (!confirm('Delete sensor ' + id + '?')) return;
    fetch('/api/sensors?id=' + id, { method: 'DELETE' }).then(() => loadSensors());
}

function testSensor(id) {
    fetch('/api/sensors/test', {
        method: 'POST',
        headers: {'Content-Type': 'application/x-www-form-urlencoded'},
        body: 'id=' + id
    }).then(r => r.json()).then(data => {
        alert('Test result for ' + data.id + ': ' + (data.value ? 'HIGH/OK' : 'LOW/FAIL'));
    });
}

function scanTV() {
    document.getElementById('tv-list').innerHTML = '<li>Scanning...</li>';
    fetch('/api/scan', {method: 'POST'}).then(() => setTimeout(loadDiscovery, 3000));
}
function loadDiscovery() {
    fetch('/api/discovery').then(r => r.json()).then(data => {
        let list = document.getElementById('tv-list');
        list.innerHTML = '';
        data.forEach(tv => {
            let li = document.createElement('li');
            li.innerHTML = `<b>${tv.name}</b> (${tv.ip}) - ${tv.model}`;
            list.appendChild(li);
        });
    });
}

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
      obj["value"] = sensor->read();
    }
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  _server.on("/api/sensors", HTTP_POST, [](AsyncWebServerRequest* request) {
    if (!request->authenticate("admin", "insomnia")) {
      return request->requestAuthentication();
    }
  }, NULL, [this](AsyncWebServerRequest* request, uint8_t* data, size_t len,
              size_t index, size_t total) {
    JsonDocument doc;
    deserializeJson(doc, data, len);

    // Save to configuration
    Config cfg = _configMgr.get();
    JsonDocument sensorDoc;
    deserializeJson(sensorDoc, cfg.sensorsJson);

    // Check if it already exists to avoid duplicates
    bool found = false;
    for (JsonObject s : sensorDoc.as<JsonArray>()) {
        if (s["id"] == doc["id"]) {
            s.set(doc.as<JsonObject>());
            found = true;
            break;
        }
    }
    if (!found) sensorDoc.add(doc);

    serializeJson(sensorDoc, cfg.sensorsJson);
    _configMgr.set(cfg);
    _configMgr.save();

    // Re-register immediately
    SensorManager::instance().init(cfg.sensorsJson, _discovery);

    request->send(200, "application/json", "{\"status\":\"ok\"}");
  });

  _server.on("/api/sensors", HTTP_DELETE, [this](AsyncWebServerRequest* request) {
    if (!request->authenticate("admin", "insomnia")) {
      return request->requestAuthentication();
    }
    if (request->hasParam("id")) {
      String id = request->getParam("id")->value();

      // Remove from configuration
      Config cfg = _configMgr.get();
      JsonDocument sensorDoc;
      deserializeJson(sensorDoc, cfg.sensorsJson);

      JsonArray array = sensorDoc.as<JsonArray>();
      for (size_t i = 0; i < array.size(); i++) {
          if (array[i]["id"] == id) {
              array.remove(i);
              break;
          }
      }

      serializeJson(sensorDoc, cfg.sensorsJson);
      _configMgr.set(cfg);
      _configMgr.save();

      // For now, simple re-init (might need a clear() in SensorManager if instances linger)
      SensorManager::instance().init(cfg.sensorsJson, _discovery);

      request->send(200, "application/json", "{\"status\":\"ok\"}");
      return;
    }
    request->send(400);
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
WebServer::WebServer(uint16_t port, SamsungTvDiscovery& discovery,
                     ConfigManager& configMgr)
    : _discovery(discovery), _configMgr(configMgr) {}
void WebServer::begin() {}
void WebServer::setupRoutes() {}
#endif

}  // namespace InsomniaTV
