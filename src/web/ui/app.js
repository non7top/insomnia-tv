
// ── SSE live updates ────────────────────────────────────────────────────────
const source = new EventSource('/events');
source.addEventListener('sensor_update', function(e) {
    const d = JSON.parse(e.data);
    const row = document.getElementById('sensor-row-' + d.id);
    if (row) { row.cells[2].textContent = d.value; row.cells[3].textContent = d.available ? '🟢' : '🔴'; }
}, false);

// ── Tab routing ─────────────────────────────────────────────────────────────
let statusTimer = null;
function openTab(evt, name) {
    if (statusTimer) { clearInterval(statusTimer); statusTimer = null; }
    if (name === 'Status') statusTimer = setInterval(loadStatus, 3000);
    document.querySelectorAll('.tabcontent').forEach(t => t.style.display = 'none');
    document.querySelectorAll('.tablinks').forEach(b => b.classList.remove('active'));
    document.getElementById(name).style.display = 'block';
    evt.currentTarget.classList.add('active');
    if (name === 'Status')  loadStatus();
    if (name === 'Sensors') loadSensors();
    if (name === 'Files')   loadFiles();
    if (name === 'Config')  loadTvControl();
}

// ── Status ──────────────────────────────────────────────────────────────────
const SLEEP_STATE_COLORS = {
    MONITORING: '#4CAF50', RAMPING: '#ff9800', VERIFYING: '#ff9800',
    POWERING_OFF: '#f44336', FALLBACK_OFF: '#f44336',
};
const TV_STATE_COLORS = {
    ON: '#4CAF50', OFF: '#757575', TRANSITIONING: '#ff9800', UNKNOWN: '#999',
};
const TRANSITIONAL_STATES = ['TRANSITIONING', 'RAMPING', 'VERIFYING', 'POWERING_OFF', 'FALLBACK_OFF'];
function setStateBadge(el, state, colors) {
    el.classList.toggle('pulse', TRANSITIONAL_STATES.includes(state));
    el.textContent = state || 'unknown';
    el.style.background = (colors && colors[state]) || '#999';
    el.style.color = '#fff';
    el.style.padding = '2px 8px';
    el.style.borderRadius = '10px';
}
function formatDuration(ms) {
    const s = Math.floor(ms / 1000);
    const d = Math.floor(s / 86400), h = Math.floor((s % 86400) / 3600), m = Math.floor((s % 3600) / 60);
    if (d > 0) return d + 'd ' + h + 'h ago';
    if (h > 0) return h + 'h ' + m + 'm ago';
    if (m > 0) return m + 'm ' + (s % 60) + 's ago';
    return s + 's ago';
}
function loadStatus() {
    fetch('/api/status').then(r => r.json()).then(data => {
        for (const [k, v] of Object.entries(data)) {
            if (k === 'sleepState' || k === 'msSinceSleepReset' || k === 'msSinceLastIrActivity') continue;
            const el = document.getElementById('sys-' + k);
            if (el) el.textContent = v;
        }
        const stateEl = document.getElementById('sys-sleepState');
        if (stateEl) setStateBadge(stateEl, data.sleepState, SLEEP_STATE_COLORS);
        const lastActEl = document.getElementById('sys-lastActivity');
        if (lastActEl) {
            lastActEl.textContent = data.msSinceSleepReset != null ?
                formatDuration(data.msSinceSleepReset) : 'unknown';
        }
    }).catch(() => {
        const el = document.getElementById('sys-status');
        if (el) el.textContent = 'Error: device unreachable';
    });
    fetch('/api/tv-config').then(r => r.json()).then(data => {
        const el = document.getElementById('tv-power_state');
        if (el) {
            setStateBadge(el, data.power_state, TV_STATE_COLORS);
            const icon = {ON: '\uD83D\uDCFA', OFF: '\u26AB', TRANSITIONING: '\u23F3'}[data.power_state];
            if (icon) el.textContent = icon + ' ' + el.textContent;
        }
        const contribEl = document.getElementById('tv-contributions');
        if (contribEl) {
            contribEl.innerHTML = (data.contributions || []).map(c => {
                const label = c.available ? (c.vote > 0 ? '+' + c.vote : c.vote) : 'n/a';
                return '<span class="s-badge" style="background:' +
                    (c.available ? '#e3f2fd' : '#eee') + ';color:' +
                    (c.available ? '#1565c0' : '#999') + '">' +
                    c.sensor_id + ': ' + label + '</span>';
            }).join('');
        }
    }).catch(() => {
        const el = document.getElementById('tv-power_state');
        if (el) setStateBadge(el, 'ERROR', {ERROR: '#f44336'});
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
                <button onclick="event.stopPropagation();testSensor('${s.id}', this)">Test</button>
                <button class="delete" onclick="event.stopPropagation();deleteSensor('${s.id}')">Delete</button>
              </td>`;
            tr.onclick = () => toggleSensorDetail(s.id);
            tbody.appendChild(tr);
        });
    }).catch(() => {
        document.getElementById('sensor-table-body').innerHTML =
            '<tr><td colspan="5" style="text-align:center;color:#f44336">Failed to load sensors</td></tr>';
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
    } else if (s.type === 'samsung_tv') {
        html += '<div class="detail-field"><label>TV IP / Hostname</label><input type="text" id="ef-host-'+s.id+'" value="'+(s.host||'')+'"></div>';
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
    else if (s.type === 'samsung_tv') s.host    = document.getElementById('ef-host-'        + id)?.value || s.host;
    else if (s.type === 'gpio_input' || s.type === 'gpio_analog')
                                  s.pin         = +document.getElementById('ef-pin-'         + id)?.value;
    fetch('/api/sensors', {method:'POST', headers:{'Content-Type':'application/json'}, body:JSON.stringify(s)})
        .then(r => { if (r.ok) { document.getElementById('sensor-detail-' + id)?.remove(); loadSensors(); } else alert('Failed to save'); })
        .catch(err => alert('Request failed: ' + err));
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
    } else if (type === 'samsung_tv') {
        f.innerHTML = `<label>TV IP/Host:</label><input type="text" id="s_host" placeholder="192.168.2.190">`;
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
    }).catch(err => {
        document.getElementById('upnp-list').innerHTML =
            '<div class="discovery-item" style="color:#f44336">Failed to load: ' + err + '</div>';
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
    else if (type === 'samsung_tv') { cfg.host = document.getElementById('s_host').value; }
    fetch('/api/sensors', {method:'POST', headers:{'Content-Type':'application/json'}, body:JSON.stringify(cfg)})
      .then(r => { if (r.ok) { closeModal(); loadSensors(); } else alert('Failed to save'); })
      .catch(err => alert('Request failed: ' + err));
}
function deleteSensor(id) {
    if (!confirm('Delete sensor ' + id + '?')) return;
    fetch('/api/sensors?id=' + id, {method:'DELETE'})
        .then(r => { if (r.ok) loadSensors(); else alert('Failed to delete'); })
        .catch(err => alert('Request failed: ' + err));
}
function testSensor(id, btn) {
    const original = btn.textContent;
    btn.disabled = true;
    btn.textContent = 'Testing…';
    fetch('/api/sensors/test', {method:'POST', headers:{'Content-Type':'application/x-www-form-urlencoded'}, body:'id='+id})
      .then(r => r.json()).then(d => alert('Test: ' + d.id + ' = ' + (d.value ? 'HIGH / OK' : 'LOW / FAIL')))
      .catch(err => alert('Test failed: ' + err))
      .finally(() => { btn.disabled = false; btn.textContent = original; });
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
    }).catch(() => {
        document.getElementById('files-table-body').innerHTML =
            '<tr><td colspan="3" style="text-align:center;color:#f44336">Failed to load files</td></tr>';
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
    }).catch(err => alert('Failed to open file: ' + err));
}
function saveFile() {
    if (!editingPath) return;
    fetch('/api/files/edit', {method: 'POST', headers: {'Content-Type': 'application/json'},
        body: JSON.stringify({path: editingPath, content: document.getElementById('editor-content').value})
    }).then(r => { if (r.ok) { closeEditor(); loadFiles(); } else alert('Save failed'); })
      .catch(err => alert('Request failed: ' + err));
}
function closeEditor() { editingPath = null; document.getElementById('file-editor').style.display = 'none'; }
function deleteFile(path) {
    if (!confirm('Delete ' + path + '?')) return;
    fetch('/api/files?path=' + encodeURIComponent(path), {method: 'DELETE'})
        .then(r => { if (r.ok) loadFiles(); else alert('Failed to delete'); })
        .catch(err => alert('Request failed: ' + err));
}
function uploadFile() {
    const fi = document.getElementById('upload-file');
    const pi = document.getElementById('upload-path');
    if (!fi.files.length) { alert('Select a file first'); return; }
    const targetPath = pi.value.trim() || ('/' + fi.files[0].name);
    const fd = new FormData();
    fd.append('file', fi.files[0]);
    fetch('/api/files/upload?path=' + encodeURIComponent(targetPath), {method: 'POST', body: fd})
        .then(r => { if (r.ok) { fi.value = ''; pi.value = ''; loadFiles(); } else alert('Upload failed'); })
        .catch(err => alert('Request failed: ' + err));
}

// ── System tab ──────────────────────────────────────────────────────────────
function rebootDevice() {
    if (!confirm('Reboot the device now?')) return;
    const msg = document.getElementById('sys-reboot-msg');
    fetch('/api/reboot', {method:'POST'})
        .then(r => { msg.textContent = r.ok ? 'Rebooting\u2026 reload this page in about 20 seconds.' : 'Reboot request failed: HTTP ' + r.status; })
        .catch(err => { msg.textContent = 'Request failed: ' + err; });
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
    }).catch(err => {
        document.getElementById('wz-scan-status').innerHTML = 'Scan failed.';
        document.getElementById('wz-tv-list').innerHTML =
            '<p style="color:#f44336;font-size:14px">' + err + ' ' +
            '<a href="#" onclick="wzStartScan();return false">Try again</a></p>';
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
function wzDeviceId(tv) {
    return tv.name.toLowerCase().replace(/[^a-z0-9]+/g, '_').replace(/^_|_$/g, '').substring(0, 24) || 'tv';
}
function wzBuildSensors(tv) {
    const slug = tv.name.toLowerCase().replace(/[^a-z0-9]+/g, '_').replace(/^_|_$/g, '').substring(0, 16) || 'tv';
    wzSensors = [
        {id: slug+'_ping', type:'ping',  target_ip:   tv.ip,   label:'Ping', desc:'Network reachability · '+tv.ip, enabled:true},
        {id: slug+'_upnp', type:'upnp',  target_name: tv.name, label:'UPnP', desc:'Service discovery · '+tv.name, enabled:true},
        {id: slug+'_power', type:'samsung_tv', host: tv.ip, label:'Power', desc:'Samsung power state (sees standby) · '+tv.ip, enabled:true},
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
    try {
        for (const s of toCreate) {
            const p = {id: s.id.trim(), type: s.type, device_id: wzDeviceId(wzTv)};
            if (s.type === 'ping') p.target_ip   = s.target_ip;
            if (s.type === 'upnp') p.target_name = s.target_name;
            if (s.type === 'http') p.url         = s.url;
            if (s.type === 'samsung_tv') p.host  = s.host;
            const r = await fetch('/api/sensors', {method:'POST', headers:{'Content-Type':'application/json'}, body:JSON.stringify(p)});
            if (r.ok) created++;
        }
    } catch (err) {
        alert('Failed to add sensors: ' + err);
        return;
    }
    document.getElementById('wz-done-title').textContent = created + ' sensor' + (created !== 1 ? 's' : '') + ' added!';
    document.getElementById('wz-done-msg').textContent   = 'Sensors for "' + wzTv.name + '" are now active in the registry.';
    wzShowPage(4);
}

// ── TV power control (Samsung pairing) ──────────────────────────────────────
function loadTvControl() {
    fetch('/api/tv/control').then(r => r.json()).then(d => {
        document.getElementById('tvc-status').textContent =
            d.paired ? 'Paired with ' + d.host + (d.mac ? ' (' + d.mac + ')' : '') : 'Not paired';
        if (d.host && !document.getElementById('tvc-host').value) document.getElementById('tvc-host').value = d.host;
    }).catch(err => { document.getElementById('tvc-status').textContent = 'Failed to load: ' + err; });
}
function pairTv(host, msgId, btnId) {
    const msg = document.getElementById(msgId), btn = document.getElementById(btnId);
    if (!host) { msg.textContent = 'Enter the TV IP address first.'; return; }
    btn.disabled = true;
    msg.textContent = 'Accept the "insomniaTV" prompt on your TV within 30 seconds\u2026';
    fetch('/api/tv/pair', {method:'POST', headers:{'Content-Type':'application/json'}, body:JSON.stringify({host:host})})
        .then(r => { if (!r.ok) throw new Error('HTTP ' + r.status); setTimeout(() => pairPoll(host, msg, btn, 0), 2000); })
        .catch(err => { msg.textContent = 'Pairing failed to start: ' + err; btn.disabled = false; });
}
function pairPoll(host, msg, btn, n) {
    fetch('/api/tv/control').then(r => r.json()).then(d => {
        if (d.pair_state === 'paired' && d.host === host) {
            msg.textContent = '\u2705 Paired. insomniaTV can now switch this TV off.';
            btn.disabled = false;
            loadTvControl();
        } else if (d.pair_state === 'failed') {
            msg.textContent = 'Pairing failed: ' + d.error;
            btn.disabled = false;
        } else if (n < 20) {
            setTimeout(() => pairPoll(host, msg, btn, n + 1), 2000);
        } else {
            msg.textContent = 'Timed out waiting for the TV.';
            btn.disabled = false;
        }
    }).catch(err => { msg.textContent = 'Status check failed: ' + err; btn.disabled = false; });
}
function testPowerOff() {
    const msg = document.getElementById('tvc-msg');
    if (!confirm('Send the power-off command to the TV now?')) return;
    fetch('/api/tv/power-off', {method:'POST'})
        .then(r => r.json().then(d => { msg.textContent = r.ok ? 'Power-off sent (only if the TV was on).' : (d.error || 'Failed'); }))
        .catch(err => { msg.textContent = 'Request failed: ' + err; });
}

document.getElementsByClassName('tablinks')[0].click();
