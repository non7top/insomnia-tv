// Copyright 2026 insomniaTV Contributors. All rights reserved.

#include "WebServer.h"

#include <ArduinoJson.h>

#include <cstdio>
#include <functional>
#include <string>

#include "../version.h"

#if defined(ARDUINO)
#include <AsyncEventSource.h>
#include <AsyncJson.h>
#include <ESP32Ping.h>
#include <HTTPClient.h>
#include <LittleFS.h>
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

void WebServer::setTvStateMachine(TvStateMachine* tvSm) {
  _tvSm = tvSm;
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
    <meta charset="utf-8">
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
        button:hover { opacity: .88; }
        button.delete { background: #f44336; }
        .modal { display:none; position:fixed; z-index:100; left:0; top:0; width:100%; height:100%; background-color:rgba(0,0,0,0.5); }
        .modal-content { background-color:#fefefe; margin:10% auto; padding:25px; border:1px solid #888; width:80%; max-width:500px; border-radius:8px; }
        .close { float:right; cursor:pointer; font-size:24px; line-height:1; }
        label { display:block; margin:10px 0 5px; font-weight:bold; }
        input[type=text], input[type=number], select { width:100%; padding:8px; border:1px solid #ccc; border-radius:4px; box-sizing:border-box; }
        .discovery-item { padding:8px; border-bottom:1px solid #eee; cursor:pointer; }
        .discovery-item:hover { background:#f0f0f0; }
        /* ── Wizard ───────────────────────────────────────────────── */
        .wizard-modal { max-width:560px; }
        .wz-progress { display:flex; align-items:center; justify-content:center; margin:4px 0 22px; gap:4px; }
        .wz-dot { width:26px; height:26px; border-radius:50%; background:#ddd; color:#aaa; display:flex; align-items:center; justify-content:center; font-size:12px; font-weight:bold; flex-shrink:0; transition:.3s; }
        .wz-dot.active { background:#4CAF50; color:#fff; }
        .wz-dot.done { background:#81c784; color:#fff; }
        .wz-line { flex:1; height:2px; background:#ddd; max-width:48px; transition:.3s; }
        .wz-line.done { background:#81c784; }
        .wz-footer { display:flex; gap:8px; justify-content:space-between; align-items:center; margin-top:20px; padding-top:15px; border-top:1px solid #eee; }
        .wz-footer-right { display:flex; gap:8px; }
        .tv-card { border:2px solid #ddd; border-radius:8px; padding:12px 16px; margin:8px 0; cursor:pointer; display:flex; align-items:center; gap:14px; transition:.15s; }
        .tv-card:hover { border-color:#90caf9; background:#e3f2fd; }
        .tv-card.selected { border-color:#4CAF50; background:#e8f5e9; }
        .tv-icon { font-size:32px; line-height:1; }
        .tv-name { font-weight:bold; font-size:15px; }
        .tv-meta { color:#777; font-size:13px; margin-top:2px; }
        .s-badge { font-size:11px; padding:2px 7px; border-radius:10px; background:#e0e0e0; color:#555; white-space:nowrap; flex-shrink:0; }
        .s-badge.ping { background:#e3f2fd; color:#1565c0; }
        .s-badge.upnp { background:#f3e5f5; color:#6a1b9a; }
        .s-badge.http { background:#e8f5e9; color:#2e7d32; }
        .wz-srow { display:flex; align-items:center; gap:10px; padding:10px 14px; border:1px solid #e0e0e0; border-radius:6px; margin:5px 0; background:#fafafa; }
        .wz-srow input[type=checkbox] { width:auto; flex-shrink:0; margin:0; cursor:pointer; }
        .wz-srow-info { flex:1; min-width:0; overflow:hidden; }
        .wz-srow-name { font-size:13px; color:#333; white-space:nowrap; overflow:hidden; text-overflow:ellipsis; }
        .wz-srow-id { font-size:11px; color:#aaa; margin-top:2px; font-family:monospace; white-space:nowrap; overflow:hidden; text-overflow:ellipsis; }
        .spinner { display:inline-block; width:18px; height:18px; border:3px solid #ddd; border-top-color:#4CAF50; border-radius:50%; animation:spin .7s linear infinite; vertical-align:middle; margin-right:8px; }
        @keyframes spin { to { transform:rotate(360deg); } }
        .btn-tv { background:#1565c0; }
        .btn-gray { background:#757575; }
        .done-icon { font-size:52px; display:block; text-align:center; margin:10px 0 6px; }
        .sensor-detail-row td { padding:0; border-bottom:2px solid #4CAF50; }
        .sensor-detail-inner { padding:12px 14px; background:#f9fff9; display:flex; flex-wrap:wrap; gap:10px; align-items:flex-end; }
        .sensor-detail-inner label { display:block; font-size:12px; font-weight:bold; color:#555; margin-bottom:3px; }
        .sensor-detail-inner input[type=text] { width:220px; padding:6px 8px; border:1px solid #bbb; border-radius:4px; font-size:13px; }
        .sensor-detail-inner .detail-field { display:flex; flex-direction:column; }
    </style>
</head>
<body>

<div class="tab">
  <button class="tablinks" onclick="openTab(event,'Status')">Status</button>
  <button class="tablinks" onclick="openTab(event,'Config')">Config</button>
  <button class="tablinks" onclick="openTab(event,'Sensors')">Sensors</button>
  <button class="tablinks" onclick="openTab(event,'Files')">Files</button>
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
  <div style="display:flex;gap:8px;margin-bottom:15px;flex-wrap:wrap;">
    <button class="btn-tv" onclick="openWizard()">&#128250; Add Smart TV</button>
    <button onclick="openModal()">+ Add Sensor</button>
  </div>
  <table>
    <thead>
      <tr><th>ID</th><th>Type</th><th>Value</th><th>Status</th><th>Actions</th></tr>
    </thead>
    <tbody id="sensor-table-body"></tbody>
  </table>
</div>

<div id="Files" class="tabcontent">
  <h3>File Manager</h3>
  <fieldset>
    <legend>Upload</legend>
    <label>Target path:</label>
    <input type="text" id="upload-path" placeholder="/config/insomnia_tv.json" style="margin-bottom:8px">
    <input type="file" id="upload-file" style="margin-bottom:8px">
    <button onclick="uploadFile()">Upload</button>
  </fieldset>
  <table>
    <thead><tr><th>Path</th><th>Size</th><th>Actions</th></tr></thead>
    <tbody id="files-table-body"></tbody>
  </table>
  <div id="file-editor" style="display:none;margin-top:20px;background:white;padding:15px;border-radius:5px;border:1px solid #ccc;">
    <h4 id="editor-title" style="margin-top:0"></h4>
    <textarea id="editor-content" style="width:100%;height:300px;font-family:monospace;font-size:13px;box-sizing:border-box;border:1px solid #ccc;border-radius:4px;padding:8px;"></textarea>
    <div style="margin-top:10px;display:flex;gap:8px;">
      <button id="editor-save-btn" onclick="saveFile()">Save</button>
      <button class="btn-gray" onclick="closeEditor()">Cancel</button>
    </div>
  </div>
</div>

<!-- ── Add Sensor modal ─────────────────────────────────────────── -->
<div id="sensorModal" class="modal">
  <div class="modal-content">
    <span onclick="closeModal()" class="close">&times;</span>
    <h3>Add Sensor</h3>
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
    <button type="button" onclick="saveSensor()" style="margin-top:20px;width:100%;">Save Sensor</button>
  </div>
</div>

<!-- ── Add Smart TV wizard ──────────────────────────────────────── -->
<div id="tvWizardModal" class="modal">
  <div class="modal-content wizard-modal">
    <span onclick="closeWizard()" class="close">&times;</span>
    <h3 style="margin-top:0">&#128250; Add Smart TV</h3>

    <div class="wz-progress">
      <div class="wz-dot active" id="wd1">1</div>
      <div class="wz-line" id="wl1"></div>
      <div class="wz-dot" id="wd2">2</div>
      <div class="wz-line" id="wl2"></div>
      <div class="wz-dot" id="wd3">3</div>
      <div class="wz-line" id="wl3"></div>
      <div class="wz-dot" id="wd4">&#10003;</div>
    </div>

    <!-- Step 1: intro / scan -->
    <div id="wpage1">
      <h4 style="margin-top:0">Scan Your Network</h4>
      <p>Click <b>Start Scan</b> to search your network for smart TVs. Samsung, Sony, LG and other UPnP-enabled TVs will be detected automatically.</p>
      <p style="color:#888;font-size:13px">Make sure your TV is powered on and connected to the same Wi-Fi network.</p>
    </div>

    <!-- Step 2: results -->
    <div id="wpage2" style="display:none">
      <h4 style="margin-top:0">Select Your TV</h4>
      <div id="wz-scan-status"><span class="spinner"></span>Scanning network&hellip;</div>
      <div id="wz-tv-list"></div>
    </div>

    <!-- Step 3: sensor preview -->
    <div id="wpage3" style="display:none">
      <h4 style="margin-top:0">Sensors to Create</h4>
      <p style="color:#555;font-size:14px;margin-top:0">The following sensors will monitor your TV. Edit the IDs or uncheck any you don&apos;t need.</p>
      <div id="wz-sensor-list"></div>
    </div>

    <!-- Step 4: done -->
    <div id="wpage4" style="display:none;text-align:center;padding:10px 0 6px">
      <span class="done-icon">&#9989;</span>
      <h4 id="wz-done-title" style="margin:4px 0 8px"></h4>
      <p id="wz-done-msg" style="color:#666;margin:0"></p>
    </div>

    <div class="wz-footer">
      <button id="wbtn-cancel" class="btn-gray" onclick="closeWizard()">Cancel</button>
      <div class="wz-footer-right">
        <button id="wbtn-back"   class="btn-gray" style="display:none" onclick="wzBack()">&#8592; Back</button>
        <button id="wbtn-rescan" class="btn-tv"   style="display:none" onclick="wzStartScan()">&#8635; Rescan</button>
        <button id="wbtn-scan"   class="btn-tv"   onclick="wzStartScan()">Start Scan</button>
        <button id="wbtn-add"    class="btn-tv"   style="display:none" onclick="wzCreate()">Add to Sensors &#8594;</button>
        <button id="wbtn-done"   style="display:none" onclick="closeWizard();loadSensors()">Done</button>
      </div>
    </div>
  </div>
</div>

<script>
// ── SSE live updates ────────────────────────────────────────────────────────
const source = new EventSource('/events');
source.addEventListener('sensor_update', function(e) {
    const d = JSON.parse(e.data);
    const row = document.getElementById('sensor-row-' + d.id);
    if (row) { row.cells[2].textContent = d.value; row.cells[3].textContent = d.available ? '🟢' : '🔴'; }
}, false);

// ── Tab routing ─────────────────────────────────────────────────────────────
function openTab(evt, name) {
    document.querySelectorAll('.tabcontent').forEach(t => t.style.display = 'none');
    document.querySelectorAll('.tablinks').forEach(b => b.classList.remove('active'));
    document.getElementById(name).style.display = 'block';
    evt.currentTarget.classList.add('active');
    if (name === 'Status')  loadStatus();
    if (name === 'Sensors') loadSensors();
    if (name === 'Files')   loadFiles();
}

// ── Status ──────────────────────────────────────────────────────────────────
function loadStatus() {
    fetch('/api/status').then(r => r.json()).then(data => {
        for (const [k, v] of Object.entries(data)) {
            const el = document.getElementById('sys-' + k);
            if (el) el.textContent = v;
        }
    });
}

// ── Sensor table ────────────────────────────────────────────────────────────
let sensorData = {};
function loadSensors() {
    fetch('/api/sensors').then(r => r.json()).then(data => {
        sensorData = {};
        data.forEach(s => { sensorData[s.id] = s; });
        const tbody = document.getElementById('sensor-table-body');
        tbody.innerHTML = '';
        data.forEach(s => {
            const tr = document.createElement('tr');
            tr.id = 'sensor-row-' + s.id;
            tr.style.cursor = 'pointer';
            tr.innerHTML = `<td>${s.id}</td><td>${s.type}</td><td>${s.value}</td>
              <td>${s.available ? '🟢' : '🔴'}</td>
              <td>
                <button onclick="event.stopPropagation();testSensor('${s.id}')">Test</button>
                <button class="delete" onclick="event.stopPropagation();deleteSensor('${s.id}')">Delete</button>
              </td>`;
            tr.onclick = () => toggleSensorDetail(s.id);
            tbody.appendChild(tr);
        });
    });
}

// ── Sensor expand/edit ────────────────────────────────────────────────────────
function toggleSensorDetail(id) {
    const existing = document.getElementById('sensor-detail-' + id);
    if (existing) { existing.remove(); return; }
    const s = sensorData[id];
    if (!s) return;
    const fields = buildEditFields(s);
    const detailTr = document.createElement('tr');
    detailTr.id = 'sensor-detail-' + id;
    detailTr.className = 'sensor-detail-row';
    detailTr.innerHTML = '<td colspan="5"><div class="sensor-detail-inner">' + fields +
        '<div class="detail-field" style="justify-content:flex-end;padding-bottom:2px">' +
        '<button onclick="saveSensorEdit(\'' + id + '\')" style="height:32px">Save</button></div>' +
        '</div></td>';
    const row = document.getElementById('sensor-row-' + id);
    row.parentNode.insertBefore(detailTr, row.nextSibling);
}
function buildEditFields(s) {
    let html = '';
    if (s.type === 'ping') {
        html += '<div class="detail-field"><label>IP / Hostname</label><input type="text" id="ef-target_ip-'+s.id+'" value="'+(s.target_ip||'')+'"></div>';
    } else if (s.type === 'http') {
        html += '<div class="detail-field"><label>URL</label><input type="text" id="ef-url-'+s.id+'" value="'+(s.url||'')+'"></div>';
    } else if (s.type === 'upnp') {
        html += '<div class="detail-field"><label>Device Name</label><input type="text" id="ef-target_name-'+s.id+'" value="'+(s.target_name||'')+'"></div>';
    } else if (s.type === 'gpio_input' || s.type === 'gpio_analog') {
        html += '<div class="detail-field"><label>Pin</label><input type="text" id="ef-pin-'+s.id+'" value="'+(s.pin!=null?s.pin:'')+'"></div>';
    }
    return html;
}
function saveSensorEdit(id) {
    const s = Object.assign({}, sensorData[id]);
    if (s.type === 'ping')        s.target_ip   = document.getElementById('ef-target_ip-'   + id)?.value || s.target_ip;
    else if (s.type === 'http')   s.url         = document.getElementById('ef-url-'         + id)?.value || s.url;
    else if (s.type === 'upnp')   s.target_name = document.getElementById('ef-target_name-' + id)?.value || s.target_name;
    else if (s.type === 'gpio_input' || s.type === 'gpio_analog')
                                  s.pin         = +document.getElementById('ef-pin-'         + id)?.value;
    fetch('/api/sensors', {method:'POST', headers:{'Content-Type':'application/json'}, body:JSON.stringify(s)})
        .then(() => { document.getElementById('sensor-detail-' + id)?.remove(); loadSensors(); });
}

// ── Add-sensor modal ─────────────────────────────────────────────────────────
function openModal()  { document.getElementById('sensorModal').style.display = 'block'; updateFormFields(); }
function closeModal() { document.getElementById('sensorModal').style.display = 'none'; }

function updateFormFields() {
    const type = document.getElementById('sensorType').value;
    const f = document.getElementById('dynamicFields');
    f.innerHTML = '';
    if (type === 'gpio_input') {
        f.innerHTML = `<label>Pin:</label><input type="number" id="s_pin" value="0">
          <label>Pullup:</label><input type="checkbox" id="s_pullup" checked>
          <label>Debounce (ms):</label><input type="number" id="s_debounce" value="50">`;
    } else if (type === 'gpio_analog') {
        f.innerHTML = `<label>Pin:</label><input type="number" id="s_pin" value="0">
          <label>Scale:</label><input type="number" id="s_scale" step="0.001" value="1.0">
          <label>Offset:</label><input type="number" id="s_offset" step="0.001" value="0.0">`;
    } else if (type === 'ping') {
        f.innerHTML = `<label>Target IP/Host:</label><input type="text" id="s_target" placeholder="192.168.1.1">`;
    } else if (type === 'http') {
        f.innerHTML = `<label>URL:</label><input type="text" id="s_url" placeholder="http://api.local/status">`;
    } else if (type === 'upnp') {
        f.innerHTML = `<label>Select Device:</label>
          <div id="upnp-list" style="border:1px solid #ccc;max-height:100px;overflow-y:auto;margin-bottom:10px;">Scanning...</div>
          <button type="button" onclick="scanTVForModal()" style="width:100%;margin-bottom:10px;background:#2196F3;">Scan Network</button>
          <input type="hidden" id="s_target_name">`;
        fetchUpnpDevices();
    }
}
function scanTVForModal() {
    document.getElementById('upnp-list').innerHTML = '<div class="discovery-item">Scanning...</div>';
    fetch('/api/scan', {method:'POST'}).then(() => setTimeout(fetchUpnpDevices, 3000));
}
function fetchUpnpDevices() {
    fetch('/api/discovery').then(r => r.json()).then(data => {
        const list = document.getElementById('upnp-list');
        list.innerHTML = '';
        if (!data.length) { list.innerHTML = '<div class="discovery-item">No TVs found.</div>'; return; }
        data.forEach(tv => {
            const div = document.createElement('div');
            div.className = 'discovery-item';
            div.textContent = tv.name + ' (' + tv.ip + ')';
            div.onclick = () => {
                document.getElementById('s_target_name').value = tv.name;
                list.querySelectorAll('.discovery-item').forEach(c => c.style.background = '');
                div.style.background = '#e0e0e0';
            };
            list.appendChild(div);
        });
    });
}
function saveSensor() {
    const id = document.getElementById('sensorId').value;
    const type = document.getElementById('sensorType').value;
    if (!id) { alert('ID is required'); return; }
    const cfg = {id, type};
    if (type === 'gpio_input')  { cfg.pin = +document.getElementById('s_pin').value; cfg.pullup = document.getElementById('s_pullup').checked; cfg.debounce_ms = +document.getElementById('s_debounce').value; }
    else if (type === 'gpio_analog') { cfg.pin = +document.getElementById('s_pin').value; cfg.scale = +document.getElementById('s_scale').value; cfg.offset = +document.getElementById('s_offset').value; }
    else if (type === 'ping')   { cfg.target_ip = document.getElementById('s_target').value; }
    else if (type === 'http')   { cfg.url = document.getElementById('s_url').value; }
    else if (type === 'upnp')   { cfg.target_name = document.getElementById('s_target_name').value; }
    fetch('/api/sensors', {method:'POST', headers:{'Content-Type':'application/json'}, body:JSON.stringify(cfg)})
      .then(r => { if (r.ok) { closeModal(); loadSensors(); } else alert('Failed to save'); });
}
function deleteSensor(id) {
    if (!confirm('Delete sensor ' + id + '?')) return;
    fetch('/api/sensors?id=' + id, {method:'DELETE'}).then(() => loadSensors());
}
function testSensor(id) {
    fetch('/api/sensors/test', {method:'POST', headers:{'Content-Type':'application/x-www-form-urlencoded'}, body:'id='+id})
      .then(r => r.json()).then(d => alert('Test: ' + d.id + ' = ' + (d.value ? 'HIGH / OK' : 'LOW / FAIL')));
}

// ── File Manager ─────────────────────────────────────────────────────────────
const TEXT_EXTS = new Set(['.json','.txt','.log','.yaml','.yml','.ini',
    '.csv','.html','.htm','.js','.css','.xml']);
let editingPath = null;

function loadFiles() {
    fetch('/api/files').then(r => r.json()).then(data => {
        const tbody = document.getElementById('files-table-body');
        tbody.innerHTML = '';
        if (!data.length) {
            tbody.innerHTML = '<tr><td colspan="3" style="text-align:center;color:#888">No files on filesystem</td></tr>';
            return;
        }
        data.forEach(f => {
            const dot = f.name.lastIndexOf('.');
            const ext = dot >= 0 ? f.name.substring(dot).toLowerCase() : '';
            const isText = TEXT_EXTS.has(ext);
            const dl = '/api/files/download?path=' + encodeURIComponent(f.name);
            const tr = document.createElement('tr');
            tr.innerHTML = '<td>' + f.name + '</td><td>' + fmtSize(f.size) + '</td><td>' +
                '<a href="' + dl + '" download><button>Download</button></a> ' +
                (isText ? '<button onclick="editFile(\'' + f.name.replace(/'/g,"\\'") + '\')">Edit</button> ' : '') +
                '<button class="delete" onclick="deleteFile(\'' + f.name.replace(/'/g,"\\'") + '\')">Delete</button>' +
                '</td>';
            tbody.appendChild(tr);
        });
    });
}
function fmtSize(b) {
    if (b < 1024) return b + ' B';
    if (b < 1048576) return (b / 1024).toFixed(1) + ' KB';
    return (b / 1048576).toFixed(1) + ' MB';
}
function editFile(path) {
    fetch('/api/files/edit?path=' + encodeURIComponent(path)).then(r => r.text()).then(content => {
        editingPath = path;
        document.getElementById('editor-title').textContent = 'Editing: ' + path;
        document.getElementById('editor-content').value = content;
        const btn = document.getElementById('editor-save-btn');
        btn.textContent = path === '/config/insomnia_tv.json' ? 'Save & Reload Config' : 'Save';
        document.getElementById('file-editor').style.display = 'block';
        document.getElementById('file-editor').scrollIntoView({behavior: 'smooth'});
    });
}
function saveFile() {
    if (!editingPath) return;
    fetch('/api/files/edit', {method: 'POST', headers: {'Content-Type': 'application/json'},
        body: JSON.stringify({path: editingPath, content: document.getElementById('editor-content').value})
    }).then(r => { if (r.ok) { closeEditor(); loadFiles(); } else alert('Save failed'); });
}
function closeEditor() { editingPath = null; document.getElementById('file-editor').style.display = 'none'; }
function deleteFile(path) {
    if (!confirm('Delete ' + path + '?')) return;
    fetch('/api/files?path=' + encodeURIComponent(path), {method: 'DELETE'}).then(() => loadFiles());
}
function uploadFile() {
    const fi = document.getElementById('upload-file');
    const pi = document.getElementById('upload-path');
    if (!fi.files.length) { alert('Select a file first'); return; }
    const targetPath = pi.value.trim() || ('/' + fi.files[0].name);
    const fd = new FormData();
    fd.append('file', fi.files[0]);
    fetch('/api/files/upload?path=' + encodeURIComponent(targetPath), {method: 'POST', body: fd})
        .then(r => { if (r.ok) { fi.value = ''; pi.value = ''; loadFiles(); } else alert('Upload failed'); });
}

// ── Config tab ───────────────────────────────────────────────────────────────
function scanTV() {
    document.getElementById('tv-list').innerHTML = '<li>Scanning...</li>';
    fetch('/api/scan', {method:'POST'}).then(() => setTimeout(loadDiscovery, 3000));
}
function loadDiscovery() {
    fetch('/api/discovery').then(r => r.json()).then(data => {
        const list = document.getElementById('tv-list');
        list.innerHTML = '';
        data.forEach(tv => {
            const li = document.createElement('li');
            li.innerHTML = '<b>' + tv.name + '</b> (' + tv.ip + ') - ' + tv.model;
            list.appendChild(li);
        });
    });
}

// ── Add Smart TV wizard ──────────────────────────────────────────────────────
let wzTv = null, wzSensors = [], wzPage = 1, wzPollTimer = null, wzPollCount = 0;

function openWizard() {
    wzTv = null; wzSensors = []; wzPage = 1; wzPollCount = 0;
    if (wzPollTimer) { clearTimeout(wzPollTimer); wzPollTimer = null; }
    document.getElementById('tvWizardModal').style.display = 'block';
    wzShowPage(1);
}
function closeWizard() {
    if (wzPollTimer) { clearTimeout(wzPollTimer); wzPollTimer = null; }
    document.getElementById('tvWizardModal').style.display = 'none';
}

function wzShowPage(n) {
    wzPage = n;
    [1,2,3,4].forEach(i => {
        document.getElementById('wpage' + i).style.display = i === n ? 'block' : 'none';
        const dot = document.getElementById('wd' + i);
        dot.className = 'wz-dot' + (i < n ? ' done' : i === n ? ' active' : '');
        if (i < 4) document.getElementById('wl' + i).className = 'wz-line' + (i < n ? ' done' : '');
    });
    document.getElementById('wbtn-cancel').style.display = n < 4 ? '' : 'none';
    document.getElementById('wbtn-back').style.display   = n === 3 ? '' : 'none';
    document.getElementById('wbtn-rescan').style.display = n === 2 ? '' : 'none';
    document.getElementById('wbtn-scan').style.display   = n === 1 ? '' : 'none';
    document.getElementById('wbtn-add').style.display    = n === 3 ? '' : 'none';
    document.getElementById('wbtn-done').style.display   = n === 4 ? '' : 'none';
}

function wzStartScan() {
    wzShowPage(2);
    document.getElementById('wz-tv-list').innerHTML = '';
    document.getElementById('wz-scan-status').innerHTML = '<span class="spinner"></span>Scanning network&hellip;';
    fetch('/api/scan', {method:'POST'});
    wzPollCount = 0;
    wzPoll();
}
function wzPoll() {
    wzPollCount++;
    fetch('/api/discovery').then(r => r.json()).then(all => {
        const tvs = all.filter(d =>
            !d.name.toLowerCase().includes('server') &&
            !d.model.toLowerCase().startsWith('dms') &&
            !d.model.toLowerCase().startsWith('nas')
        );
        if (tvs.length) {
            wzShowTvList(tvs);
        } else if (wzPollCount < 8) {
            wzPollTimer = setTimeout(wzPoll, 2000);
        } else {
            document.getElementById('wz-scan-status').innerHTML = 'No smart TVs found.';
            document.getElementById('wz-tv-list').innerHTML =
                '<p style="color:#888;font-size:14px">Make sure your TV is on and connected to the same network. ' +
                '<a href="#" onclick="wzStartScan();return false">Try again</a></p>';
        }
    });
}
function wzShowTvList(tvs) {
    document.getElementById('wz-scan-status').innerHTML =
        tvs.length + ' TV' + (tvs.length > 1 ? 's' : '') + ' found &mdash; select yours:';
    const list = document.getElementById('wz-tv-list');
    list.innerHTML = '';
    tvs.forEach(tv => {
        const card = document.createElement('div');
        card.className = 'tv-card';
        card.innerHTML =
            '<div class="tv-icon">&#128250;</div>' +
            '<div style="flex:1"><div class="tv-name">' + tv.name + '</div>' +
            '<div class="tv-meta">' + (tv.model || 'Smart TV') + ' &nbsp;&middot;&nbsp; ' + tv.ip + '</div></div>' +
            '<span style="color:#bbb;font-size:20px">&#8250;</span>';
        card.onclick = () => wzSelectTv(tv, card);
        list.appendChild(card);
    });
}
function wzSelectTv(tv, card) {
    wzTv = tv;
    document.querySelectorAll('.tv-card').forEach(c => c.classList.remove('selected'));
    card.classList.add('selected');
    setTimeout(() => { wzBuildSensors(tv); wzShowPage(3); }, 250);
}
function wzBuildSensors(tv) {
    const slug = tv.name.toLowerCase().replace(/[^a-z0-9]+/g, '_').replace(/^_|_$/g, '').substring(0, 16) || 'tv';
    wzSensors = [
        {id: slug+'_ping', type:'ping',  target_ip:   tv.ip,   label:'Ping', desc:'Network reachability · '+tv.ip, enabled:true},
        {id: slug+'_upnp', type:'upnp',  target_name: tv.name, label:'UPnP', desc:'Service discovery · '+tv.name, enabled:true},
        {id: slug+'_http', type:'http',  url:'http://'+tv.ip+':8001/api/v2/', label:'HTTP', desc:'Samsung SmartThings API (optional)', enabled:false},
    ];
    const list = document.getElementById('wz-sensor-list');
    list.innerHTML = '';
    wzSensors.forEach((s, i) => {
        const row = document.createElement('div');
        row.className = 'wz-srow';
        row.innerHTML =
            '<input type="checkbox" id="wsen' + i + '" ' + (s.enabled ? 'checked' : '') +
            ' onchange="wzSensors[' + i + '].enabled=this.checked">' +
            '<span class="s-badge ' + s.type + '">' + s.label + '</span>' +
            '<div class="wz-srow-info">' +
            '<div class="wz-srow-name">' + s.desc + '</div>' +
            '<div class="wz-srow-id">' + s.id + '</div>' +
            '</div>';
        list.appendChild(row);
    });
}
function wzBack() {
    wzShowPage(2);
    if (wzTv) {
        document.querySelectorAll('.tv-card').forEach(c => {
            if (c.querySelector('.tv-name') && c.querySelector('.tv-name').textContent === wzTv.name)
                c.classList.add('selected');
        });
    }
}
async function wzCreate() {
    const toCreate = wzSensors.filter(s => s.enabled && s.id.trim());
    let created = 0;
    for (const s of toCreate) {
        const p = {id: s.id.trim(), type: s.type};
        if (s.type === 'ping') p.target_ip   = s.target_ip;
        if (s.type === 'upnp') p.target_name = s.target_name;
        if (s.type === 'http') p.url         = s.url;
        const r = await fetch('/api/sensors', {method:'POST', headers:{'Content-Type':'application/json'}, body:JSON.stringify(p)});
        if (r.ok) created++;
    }
    document.getElementById('wz-done-title').textContent = created + ' sensor' + (created !== 1 ? 's' : '') + ' added!';
    document.getElementById('wz-done-msg').textContent   = 'Sensors for "' + wzTv.name + '" are now active in the registry.';
    wzShowPage(4);
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
      JsonDocument cfg = sensor->getConfig();
      for (JsonPair kv : cfg.as<JsonObject>()) {
        const char* k = kv.key().c_str();
        if (strcmp(k, "id") != 0 && strcmp(k, "type") != 0)
          obj[k] = kv.value();
      }
    }
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  _server.on(
      "/api/sensors", HTTP_POST,
      [](AsyncWebServerRequest* request) {
        if (!request->authenticate("admin", "insomnia")) {
          return request->requestAuthentication();
        }
      },
      NULL,
      [this](AsyncWebServerRequest* request, uint8_t* data, size_t len,
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
        if (!found)
          sensorDoc.add(doc);

        serializeJson(sensorDoc, cfg.sensorsJson);
        _configMgr.set(cfg);
        _configMgr.save();

        // Re-register immediately
        SensorManager::instance().init(cfg.sensorsJson, _discovery);

        request->send(200, "application/json", "{\"status\":\"ok\"}");
      });

  _server.on("/api/sensors", HTTP_DELETE,
             [this](AsyncWebServerRequest* request) {
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

                 // Re-init so the running SensorManager reflects the updated
                 // config.
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
            String response = "{\"id\":\"" + id +
                              "\", \"value\":" + (result ? "true" : "false") +
                              "}";
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

  // One-shot sensor probe without registering it in SensorManager.
  // Uses AsyncCallbackJsonWebHandler so the body is fully accumulated before
  // our handler is called — avoids the deferred-send race with _send().
  _server.on("/api/probe", HTTP_POST,
             [this](AsyncWebServerRequest* request, JsonVariant& json) {
               if (!request->authenticate("admin", "insomnia")) {
                 return request->requestAuthentication();
               }
               std::string type = json["type"] | "";
               bool available = false;
               int latencyMs = -1;

               if (type == "ping") {
                 std::string target = json["target_ip"] | "";
                 if (!target.empty()) {
                   uint32_t t = millis();
                   available = Ping.ping(target.c_str(), 1);
                   latencyMs = static_cast<int>(millis() - t);
                 }
               } else if (type == "http") {
                 std::string url = json["url"] | "";
                 if (!url.empty()) {
                   HTTPClient http;
                   http.begin(url.c_str());
                   http.setTimeout(3000);
                   uint32_t t = millis();
                   int code = http.GET();
                   latencyMs = static_cast<int>(millis() - t);
                   available = (code > 0 && code < 500);
                   http.end();
                 }
               } else if (type == "upnp") {
                 std::string target = json["target_name"] | "";
                 for (auto& tv : _discovery.getDiscoveredTvs()) {
                   if (tv.name == target) {
                     available = true;
                     break;
                   }
                 }
                 latencyMs = 0;
               }

               char buf[80];
               snprintf(buf, sizeof(buf),
                        "{\"available\":%s,\"latency_ms\":%d}",
                        available ? "true" : "false", latencyMs);
               request->send(200, "application/json", buf);
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

  // GET /api/tv-config — live sensor contributions and power state
  _server.on(
      "/api/tv-config", HTTP_GET, [this](AsyncWebServerRequest* request) {
        if (!request->authenticate("admin", "insomnia")) {
          return request->requestAuthentication();
        }
        JsonDocument doc;
        if (_tvSm != nullptr) {
          const char* states[] = {"UNKNOWN", "ON", "OFF", "TRANSITIONING"};
          int ps = static_cast<int>(_tvSm->getPowerState());
          doc["power_state"] = states[ps];
          JsonArray contribs = doc["contributions"].to<JsonArray>();
          for (const auto& c : _tvSm->getContributions()) {
            JsonObject o = contribs.add<JsonObject>();
            o["sensor_id"] = c.sensorId;
            o["enabled"] = c.enabled;
            o["available"] = c.available;
            o["vote"] = c.weightedVote;
          }
        }
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
      });

  // GET /api/files — recursive LittleFS listing
  _server.on("/api/files", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (!request->authenticate("admin", "insomnia")) {
      return request->requestAuthentication();
    }
    JsonDocument doc;
    JsonArray array = doc.to<JsonArray>();
    std::function<void(const String&)> walk;
    walk = [&walk, &array](const String& dir) {
      File d = LittleFS.open(dir);
      if (!d || !d.isDirectory()) {
        return;
      }
      File f = d.openNextFile();
      while (f) {
        String p = (dir == "/") ? String("/") + f.name()
                                : dir + "/" + f.name();
        if (f.isDirectory()) {
          walk(p);
        } else {
          JsonObject o = array.add<JsonObject>();
          o["name"] = p;
          o["size"] = static_cast<uint32_t>(f.size());
        }
        f = d.openNextFile();
      }
    };
    walk("/");
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  // GET /api/files/download?path=<path>
  _server.on("/api/files/download", HTTP_GET,
             [](AsyncWebServerRequest* request) {
               if (!request->authenticate("admin", "insomnia")) {
                 return request->requestAuthentication();
               }
               if (!request->hasParam("path")) {
                 request->send(400);
                 return;
               }
               String path = request->getParam("path")->value();
               if (!LittleFS.exists(path)) {
                 request->send(404);
                 return;
               }
               request->send(LittleFS, path, "application/octet-stream",
                             true);
             });

  // GET /api/files/edit?path=<path> — return file contents as plain text
  _server.on("/api/files/edit", HTTP_GET,
             [](AsyncWebServerRequest* request) {
               if (!request->authenticate("admin", "insomnia")) {
                 return request->requestAuthentication();
               }
               if (!request->hasParam("path")) {
                 request->send(400);
                 return;
               }
               String path = request->getParam("path")->value();
               if (!LittleFS.exists(path)) {
                 request->send(404);
                 return;
               }
               File f = LittleFS.open(path, "r");
               if (!f) {
                 request->send(500);
                 return;
               }
               if (f.size() > 65536) {
                 f.close();
                 request->send(413, "text/plain", "File too large to edit");
                 return;
               }
               String content = f.readString();
               f.close();
               request->send(200, "text/plain", content);
             });

  // POST /api/files/edit — atomic save; hot-reloads config if applicable
  _server.on(
      "/api/files/edit", HTTP_POST,
      [](AsyncWebServerRequest* request) {
        if (!request->authenticate("admin", "insomnia")) {
          return request->requestAuthentication();
        }
      },
      NULL,
      [this](AsyncWebServerRequest* request, uint8_t* data, size_t len,
             size_t index, size_t total) {
        JsonDocument doc;
        deserializeJson(doc, data, len);
        String path = doc["path"] | "";
        String content = doc["content"] | "";
        if (path.isEmpty()) {
          request->send(400);
          return;
        }
        String tmp = path + ".tmp";
        File f = LittleFS.open(tmp, "w");
        if (!f) {
          request->send(500);
          return;
        }
        f.print(content);
        f.close();
        LittleFS.remove(path);
        if (!LittleFS.rename(tmp, path)) {
          request->send(500);
          return;
        }
        if (path == ConfigManager::kConfigPath) {
          _configMgr.load();
        }
        request->send(200, "application/json", "{\"status\":\"ok\"}");
      });

  // DELETE /api/files?path=<path>
  _server.on("/api/files", HTTP_DELETE,
             [](AsyncWebServerRequest* request) {
               if (!request->authenticate("admin", "insomnia")) {
                 return request->requestAuthentication();
               }
               if (!request->hasParam("path")) {
                 request->send(400);
                 return;
               }
               String path = request->getParam("path")->value();
               if (LittleFS.remove(path)) {
                 request->send(200, "application/json",
                               "{\"status\":\"ok\"}");
               } else {
                 request->send(404);
               }
             });

  // POST /api/files/upload?path=<path> — atomic multipart upload
  _server.on(
      "/api/files/upload", HTTP_POST,
      [](AsyncWebServerRequest* request) {
        request->send(200, "application/json", "{\"status\":\"ok\"}");
      },
      [](AsyncWebServerRequest* request, String filename, size_t index,
         uint8_t* data, size_t len, bool final) {
        static File uploadFile;
        static String uploadPath;
        if (index == 0) {
          uploadPath = request->hasParam("path")
                           ? request->getParam("path")->value()
                           : "/" + filename;
          String tmp = uploadPath + ".tmp";
          LittleFS.remove(tmp);
          uploadFile = LittleFS.open(tmp, "w");
        }
        if (uploadFile) {
          uploadFile.write(data, len);
        }
        if (final) {
          if (uploadFile) {
            uploadFile.close();
            String tmp = uploadPath + ".tmp";
            LittleFS.remove(uploadPath);
            LittleFS.rename(tmp, uploadPath);
          }
        }
      });
}
#else
WebServer::WebServer(uint16_t port, SamsungTvDiscovery& discovery,
                     ConfigManager& configMgr)
    : _discovery(discovery), _configMgr(configMgr) {}
void WebServer::begin() {}
void WebServer::setTvStateMachine(TvStateMachine* tvSm) {
  _tvSm = tvSm;
}
void WebServer::setupRoutes() {}
#endif

}  // namespace InsomniaTV
