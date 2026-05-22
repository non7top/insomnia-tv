// Copyright 2026 insomniaTV Contributors. All rights reserved.

#include "WebServer.h"
#include <ArduinoJson.h>
#include <string>

#include "../version.h"

#if defined(ARDUINO)
#include <AsyncEventSource.h>
#include <ESP32Ping.h>
#include <HTTPClient.h>
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
        .wz-footer { display:flex; gap:8px; justify-content:flex-end; margin-top:20px; padding-top:15px; border-top:1px solid #eee; }
        .tv-card { border:2px solid #ddd; border-radius:8px; padding:12px 16px; margin:8px 0; cursor:pointer; display:flex; align-items:center; gap:14px; transition:.15s; }
        .tv-card:hover { border-color:#4CAF50; background:#f9fff9; }
        .tv-card.selected { border-color:#4CAF50; background:#e8f5e9; }
        .tv-icon { font-size:32px; line-height:1; }
        .tv-name { font-weight:bold; font-size:15px; }
        .tv-meta { color:#777; font-size:13px; margin-top:2px; }
        .sensor-preview-row { display:flex; align-items:center; gap:8px; padding:9px 10px; border:1px solid #e8e8e8; border-radius:6px; margin:6px 0; background:#fafafa; }
        .sensor-preview-row input[type=checkbox] { width:auto; margin:0; flex-shrink:0; }
        .s-badge { font-size:11px; padding:2px 7px; border-radius:10px; background:#e0e0e0; color:#555; white-space:nowrap; }
        .s-badge.ping { background:#e3f2fd; color:#1565c0; }
        .s-badge.upnp { background:#f3e5f5; color:#6a1b9a; }
        .s-badge.http { background:#e8f5e9; color:#2e7d32; }
        .s-id-wrap { flex:1; min-width:0; }
        .s-id-wrap input { padding:4px 6px; font-size:13px; border:1px solid #ccc; border-radius:4px; width:100%; box-sizing:border-box; }
        .s-desc { color:#999; font-size:11px; margin-top:3px; white-space:nowrap; overflow:hidden; text-overflow:ellipsis; }
        .spinner { display:inline-block; width:18px; height:18px; border:3px solid #ddd; border-top-color:#4CAF50; border-radius:50%; animation:spin .7s linear infinite; vertical-align:middle; margin-right:8px; }
        @keyframes spin { to { transform:rotate(360deg); } }
        .btn-tv { background:#1565c0; }
        .btn-gray { background:#757575; }
        .done-icon { font-size:52px; display:block; text-align:center; margin:10px 0 6px; }
    </style>
</head>
<body>

<div class="tab">
  <button class="tablinks" onclick="openTab(event,'Status')">Status</button>
  <button class="tablinks" onclick="openTab(event,'Config')">Config</button>
  <button class="tablinks" onclick="openTab(event,'Sensors')">Sensors</button>
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
      <button id="wbtn-back"   class="btn-gray" style="display:none" onclick="wzBack()">&#8592; Back</button>
      <button id="wbtn-scan"   class="btn-tv"   onclick="wzStartScan()">Start Scan</button>
      <button id="wbtn-add"    class="btn-tv"   style="display:none" onclick="wzCreate()">Add to Sensors &#8594;</button>
      <button id="wbtn-done"   style="display:none" onclick="closeWizard();loadSensors()">Done</button>
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
function loadSensors() {
    fetch('/api/sensors').then(r => r.json()).then(data => {
        const tbody = document.getElementById('sensor-table-body');
        tbody.innerHTML = '';
        data.forEach(s => {
            const tr = document.createElement('tr');
            tr.id = 'sensor-row-' + s.id;
            tr.innerHTML = `<td>${s.id}</td><td>${s.type}</td><td>${s.value}</td>
              <td>${s.available ? '🟢' : '🔴'}</td>
              <td>
                <button onclick="testSensor('${s.id}')">Test</button>
                <button class="delete" onclick="deleteSensor('${s.id}')">Delete</button>
              </td>`;
            tbody.appendChild(tr);
        });
    });
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
    document.getElementById('wbtn-back').style.display   = (n === 2 || n === 3) ? '' : 'none';
    document.getElementById('wbtn-scan').style.display   = n === 1 ? '' : 'none';
    document.getElementById('wbtn-add').style.display    = n === 3 ? '' : 'none';
    document.getElementById('wbtn-done').style.display   = n === 4 ? '' : 'none';
}

function wzStartScan() {
    wzShowPage(2);
    document.getElementById('wz-tv-list').innerHTML = '';
    document.getElementById('wz-scan-status').innerHTML =
        '<span class=”spinner”></span>Scanning network&hellip;' +
        '&nbsp;<a href=”#” style=”font-size:13px;color:#aaa” onclick=”wzStartScan();return false”>Rescan</a>';
    fetch('/api/scan', {method:'POST'});
    wzPollCount = 0;
    wzPoll();
}
function wzPoll() {
    wzPollCount++;
    fetch('/api/discovery').then(r => r.json()).then(all => {
        // Filter out non-TV devices (media servers, NAS, etc.)
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
            document.getElementById('wz-scan-status').innerHTML =
                'No smart TVs found. &nbsp;<a href=”#” onclick=”wzStartScan();return false”>Rescan</a>';
            document.getElementById('wz-tv-list').innerHTML =
                '<p style=”color:#888;font-size:14px”>Make sure your TV is on and connected to the same Wi-Fi.</p>';
        }
    });
}
function wzShowTvList(tvs) {
    document.getElementById('wz-scan-status').innerHTML =
        tvs.length + ' TV' + (tvs.length > 1 ? 's' : '') + ' found &mdash; select yours:' +
        '&nbsp;<a href=”#” style=”font-size:13px;color:#888” onclick=”wzStartScan();return false”>Rescan</a>';
    const list = document.getElementById('wz-tv-list');
    list.innerHTML = '';
    tvs.forEach(tv => {
        const card = document.createElement('div');
        card.className = 'tv-card';
        card.innerHTML =
            '<div class=”tv-icon”>&#128250;</div>' +
            '<div style=”flex:1”><div class=”tv-name”>' + tv.name + '</div>' +
            '<div class=”tv-meta”>' + (tv.model || 'Smart TV') + ' &nbsp;&middot;&nbsp; ' + tv.ip + '</div></div>' +
            '<span style=”color:#bbb;font-size:20px”>&#8250;</span>';
        card.onclick = () => wzSelectTv(tv, card);
        list.appendChild(card);
    });
}
function wzSelectTv(tv, card) {
    wzTv = tv;
    document.querySelectorAll('.tv-card').forEach(c => c.classList.remove('selected'));
    card.classList.add('selected');
    setTimeout(() => { wzBuildSensors(tv); wzShowPage(3); wzProbeAll(); }, 250);
}
function wzBuildSensors(tv) {
    const slug = tv.name.toLowerCase().replace(/[^a-z0-9]+/g, '_').replace(/^_|_$/g, '').substring(0, 16) || 'tv';
    wzSensors = [
        {id:slug+'_ping', type:'ping', target_ip:tv.ip,   label:'Ping', desc:'Network reachability \xb7 '+tv.ip,        enabled:true},
        {id:slug+'_upnp', type:'upnp', target_name:tv.name,label:'UPnP', desc:'Service discovery \xb7 '+tv.name,         enabled:true},
        {id:slug+'_http', type:'http', url:'http://'+tv.ip+':8001/api/v2/', label:'HTTP', desc:'Samsung SmartThings API', enabled:false},
    ];
    const list = document.getElementById('wz-sensor-list');
    list.innerHTML = '';
    wzSensors.forEach((s, i) => {
        const row = document.createElement('div');
        row.className = 'sensor-preview-row';
        row.innerHTML =
            '<input type=”checkbox” id=”wsen'+i+'” '+(s.enabled?'checked':'')+
            ' onchange=”wzSensors['+i+'].enabled=this.checked”>'+
            '<span class=”s-badge '+s.type+'”>'+s.label+'</span>'+
            '<div class=”s-id-wrap”>'+
              '<input type=”text” value=”'+s.id+'” oninput=”wzSensors['+i+'].id=this.value”>'+
              '<div class=”s-desc”>'+s.desc+'</div>'+
            '</div>'+
            '<span id=”wprobe-'+i+'” style=”min-width:72px;text-align:right;font-size:13px;flex-shrink:0”>'+
              '<span class=”spinner”></span>'+
            '</span>';
        list.appendChild(row);
    });
}
function wzProbeAll() {
    wzSensors.forEach((s, i) => wzProbeOne(s, i));
}
function wzProbeOne(s, i) {
    const el = document.getElementById('wprobe-' + i);
    if (!el) return;
    el.innerHTML = '<span class=”spinner”></span>';
    const p = {type: s.type};
    if (s.type === 'ping') p.target_ip   = s.target_ip;
    if (s.type === 'upnp') p.target_name = s.target_name;
    if (s.type === 'http') p.url         = s.url;
    fetch('/api/probe', {method:'POST', headers:{'Content-Type':'application/json'}, body:JSON.stringify(p)})
        .then(r => r.json())
        .then(d => {
            const ms = d.latency_ms >= 0 ? ' ' + d.latency_ms + 'ms' : '';
            el.textContent = d.available ? ('🟢' + ms) : '🔴 fail';
        })
        .catch(() => { el.textContent = '🔴 err'; });
}
function wzBack() {
    if (wzPage === 3) {
        wzShowPage(2);
        if (wzTv) {
            document.querySelectorAll('.tv-card').forEach(c => {
                if (c.querySelector('.tv-name') && c.querySelector('.tv-name').textContent === wzTv.name)
                    c.classList.add('selected');
            });
        }
    } else if (wzPage === 2) {
        wzShowPage(1);
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
    document.getElementById('wz-done-msg').textContent   = 'Sensors for “' + wzTv.name + '” are now active in the registry.';
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

  // One-shot sensor probe without registering it in SensorManager.
  // Runs blocking network calls in a dedicated task so the async web server
  // is not stalled.
  _server.on(
      "/api/probe", HTTP_POST,
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

        struct ProbeCtx {
          AsyncWebServerRequest* req;
          std::string type;
          std::string target;   // ip for ping, friendly name for upnp
          std::string url;      // for http
          SamsungTvDiscovery* discovery;
        };
        auto* ctx = new ProbeCtx;
        ctx->req = request;
        ctx->type = doc["type"] | "";
        ctx->target = ctx->type == "ping"
                          ? std::string(doc["target_ip"] | "")
                          : std::string(doc["target_name"] | "");
        ctx->url = doc["url"] | "";
        ctx->discovery = &_discovery;

        xTaskCreate(
            [](void* arg) {
              auto* ctx = static_cast<ProbeCtx*>(arg);
              bool available = false;
              int latencyMs = -1;

              if (ctx->type == "ping" && !ctx->target.empty()) {
                uint32_t t = millis();
                available = Ping.ping(ctx->target.c_str(), 1);
                latencyMs = static_cast<int>(millis() - t);
              } else if (ctx->type == "http" && !ctx->url.empty()) {
                HTTPClient http;
                http.begin(ctx->url.c_str());
                http.setTimeout(3000);
                uint32_t t = millis();
                int code = http.GET();
                latencyMs = static_cast<int>(millis() - t);
                available = (code > 0 && code < 500);
                http.end();
              } else if (ctx->type == "upnp") {
                for (auto& tv : ctx->discovery->getDiscoveredTvs()) {
                  if (tv.name == ctx->target) { available = true; break; }
                }
                latencyMs = 0;
              }

              char buf[80];
              snprintf(buf, sizeof(buf),
                       "{\"available\":%s,\"latency_ms\":%d}",
                       available ? "true" : "false", latencyMs);
              ctx->req->send(200, "application/json", buf);
              delete ctx;
              vTaskDelete(NULL);
            },
            "probe_task", 6144, ctx, 1, NULL);
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
