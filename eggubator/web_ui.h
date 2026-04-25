#ifndef WEB_UI_H
#define WEB_UI_H

#include <Arduino.h>

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>EGGubator</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <script src="https://cdn.jsdelivr.net/npm/chart.js@4.4.1/dist/chart.umd.min.js"></script>
  <script src="https://cdn.jsdelivr.net/npm/hammerjs@2.0.8"></script>
  <script src="https://cdn.jsdelivr.net/npm/chartjs-plugin-zoom@2.0.1/dist/chartjs-plugin-zoom.min.js"></script>
  <style>
    * { box-sizing: border-box; }
    body { font-family: 'Segoe UI', Arial, sans-serif; margin: 0; padding: 0; background: #f0f2f5; color: #1c1e21; }
    .container { max-width: 800px; margin: 0 auto; padding: 15px; }
    .card { background: white; padding: 20px; border-radius: 12px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); margin-bottom: 20px; }
    h1 { color: #1877f2; text-align: center; margin: 0 0 20px 0; }
    .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(150px, 1fr)); gap: 15px; }
    .stat-card { background: #f8f9fa; padding: 15px; border-radius: 10px; text-align: center; }
    .stat-label { font-size: 14px; color: #65676b; }
    .stat-value { font-size: 24px; font-weight: bold; margin-top: 5px; }
    .on { color: #42b72a; }
    .off { color: #f02849; }
    .controls { display: grid; grid-template-columns: repeat(auto-fit, minmax(120px, 1fr)); gap: 10px; margin-top: 15px; }
    .btn { padding: 10px; border: none; border-radius: 6px; cursor: pointer; font-weight: 600; transition: background 0.2s; }
    .btn-auto { background: #e7f3ff; color: #1877f2; }
    .btn-off { background: #ffe9ea; color: #f02849; }
    .chart-box { height: 250px; margin-top: 20px; }
    canvas { touch-action: pan-y; }
    .footer { text-align: center; font-size: 12px; color: #8a8d91; margin-top: 20px; }
    .stage-badge { display: inline-block; padding: 4px 12px; border-radius: 20px; font-size: 12px; font-weight: bold; text-transform: uppercase; }
    .badge-incubation { background: #e7f3ff; color: #1877f2; }
    .badge-lockdown { background: #fef2d8; color: #925e0d; }
    .sys-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; font-size: 13px; color: #65676b; }
    .sys-item { display: flex; justify-content: space-between; padding: 5px 0; border-bottom: 1px solid #f0f2f5; }
  </style>
</head>
<body>
  <div class="container">
    <h1>EGGubator</h1>
    
    <div class="card">
      <div style="display:flex; justify-content: space-between; align-items: center; margin-bottom: 15px;">
        <div id="stageBadge" class="stage-badge">Loading...</div>
        <div style="font-size: 14px; color: #65676b;">Uptime: <span id="uptime">--</span></div>
      </div>
      <div class="grid">
        <div class="stat-card">
          <div class="stat-label">Temperature</div>
          <div class="stat-value" id="temp">--°C</div>
          <div style="font-size:11px; color:#8a8d91">Target: <span id="targetTemp">--</span>°C</div>
        </div>
        <div class="stat-card">
          <div class="stat-label">Humidity</div>
          <div class="stat-value" id="hum">--%</div>
          <div style="font-size:11px; color:#8a8d91">Target: <span id="targetHum">--</span>%</div>
        </div>
        <div class="stat-card">
          <div class="stat-label">Heater</div>
          <div class="stat-value" id="heaterStat">--</div>
        </div>
        <div class="stat-card">
          <div class="stat-label">Atomizer</div>
          <div class="stat-value" id="atomizerStat">--</div>
        </div>
      </div>
    </div>

    <div class="card">
      <h3>Device Controls</h3>
      <div class="controls">
        <div>
          <div style="font-size:12px;margin-bottom:5px">Heater</div>
          <button id="btn-heater" class="btn" onclick="toggleMode('heater')">AUTO</button>
        </div>
        <div>
          <div style="font-size:12px;margin-bottom:5px">Atomizer</div>
          <button id="btn-atomizer" class="btn" onclick="toggleMode('atomizer')">AUTO</button>
        </div>
        <div>
          <div style="font-size:12px;margin-bottom:5px">Fan</div>
          <button id="btn-fan" class="btn" onclick="toggleMode('fan')">AUTO</button>
        </div>
        <div>
          <div style="font-size:12px;margin-bottom:5px">Turner</div>
          <button id="btn-servo" class="btn" onclick="toggleMode('servo')">AUTO</button>
        </div>
      </div>
      <div style="margin-top:20px; display:flex; gap:10px;">
        <select id="stageSelect" style="flex:1; padding:8px; border-radius:6px; border:1px solid #ddd;">
          <option value="incubation">Incubation Stage (Days 1-18)</option>
          <option value="lockdown">Lockdown Stage (Days 19-21)</option>
        </select>
        <button class="btn btn-auto" onclick="setStage()">Set Stage</button>
      </div>
    </div>

    <div class="card">
      <h3>System Information</h3>
      <div class="sys-grid">
        <div class="sys-item"><span>IP Address:</span><span id="ip">--</span></div>
        <div class="sys-item"><span>WiFi Signal:</span><span id="rssi">-- dBm</span></div>
        <div class="sys-item"><span>Free Heap:</span><span id="heap">-- KB</span></div>
        <div class="sys-item"><span>RAM Logs:</span><span id="logCount">--</span></div>
        <div class="sys-item"><span>Firmware:</span><span id="version">--</span></div>
        <div class="sys-item"><span>Uptime:</span><span id="uptimeSys">--</span></div>
      </div>
    </div>

    <div class="card">
      <h3>Environmental History</h3>
      <div class="chart-box"><canvas id="envChart"></canvas></div>
    </div>

    <div class="card">
      <h3>System Activity</h3>
      <div class="chart-box"><canvas id="actChart"></canvas></div>
    </div>

    <div class="footer">
      EGGubator | <a href="/mock" style="color:#1877f2;text-decoration:none">Advanced Settings</a>
    </div>
  </div>

  <script>
    let envChart, actChart;
    let currentModes = {};

    function decodeLogs(hex, logCount) {
      if (!hex) return [];
      const bytes = new Uint8Array(hex.match(/.{1,2}/g).map(byte => parseInt(byte, 16)));
      const entries = [];
      for (let i = 0; i < logCount; i++) {
        const offset = i * 7;
        if (offset + 7 > bytes.length) break;
        const ts = bytes[offset] | (bytes[offset+1] << 8) | (bytes[offset+2] << 16) | (bytes[offset+3] << 24);
        const temp = bytes[offset+4] / 10 + 20;
        const hum = bytes[offset+5];
        const states = bytes[offset+6];
        
        entries.push({
          t: ts,
          temp: temp,
          hum: hum,
          h: states & 1,
          a: (states >> 1) & 1,
          f: (states >> 2) & 1,
          s: ((states >> 3) & 3) === 2 ? -1 : ((states >> 3) & 3)
        });
      }
      return entries;
    }

    function initCharts() {
      const envCtx = document.getElementById('envChart').getContext('2d');
      envChart = new Chart(envCtx, {
        type: 'line',
        data: {
          datasets: [
            { label: 'Temp (°C)', borderColor: '#f02849', data: [], yAxisID: 'y' },
            { label: 'Hum (%)', borderColor: '#1877f2', data: [], yAxisID: 'y1' }
          ]
        },
        options: {
          responsive: true, maintainAspectRatio: false,
          scales: {
            x: { type: 'linear', title: { display: true, text: 'Time (s ago)' } },
            y: { type: 'linear', display: true, position: 'left', title: { display: true, text: 'Temp' } },
            y1: { type: 'linear', display: true, position: 'right', grid: { drawOnChartArea: false }, title: { display: true, text: 'Hum' } }
          },
          plugins: {
            zoom: {
              pan: { enabled: true, mode: 'x' },
              zoom: { wheel: { enabled: true }, pinch: { enabled: true }, mode: 'x' }
            }
          }
        }
      });

      const actCtx = document.getElementById('actChart').getContext('2d');
      actChart = new Chart(actCtx, {
        type: 'line',
        data: {
          datasets: [
            { label: 'Heater', borderColor: '#f02849', data: [], stepped: true },
            { label: 'Atomizer', borderColor: '#1877f2', data: [], stepped: true },
            { label: 'Fan', borderColor: '#42b72a', data: [], stepped: true },
            { label: 'Turner', borderColor: '#925e0d', data: [], stepped: true }
          ]
        },
        options: {
          responsive: true, maintainAspectRatio: false,
          scales: {
            x: { type: 'linear' },
            y: { min: -1.2, max: 1.2, ticks: { callback: v => v===1?'ON':(v===0?'OFF':(v===-1?'REV':'')) } }
          },
          plugins: {
            zoom: {
              pan: { enabled: true, mode: 'x' },
              zoom: { wheel: { enabled: true }, pinch: { enabled: true }, mode: 'x' }
            }
          }
        }
      });
    }

    function update() {
      fetch('/data').then(r => r.json()).then(d => {
        document.getElementById('temp').textContent = d.temperature.toFixed(1) + '°C';
        document.getElementById('hum').textContent = d.humidity.toFixed(1) + '%';
        document.getElementById('targetTemp').textContent = d.targetTemp.toFixed(1);
        document.getElementById('targetHum').textContent = d.targetHum.toFixed(1);
        document.getElementById('uptime').textContent = d.uptime;
        document.getElementById('uptimeSys').textContent = d.uptime;
        document.getElementById('heap').textContent = Math.round(d.heapFree/1024);
        document.getElementById('logCount').textContent = d.logCount;
        document.getElementById('version').textContent = d.version;
        document.getElementById('ip').textContent = d.ip;
        document.getElementById('rssi').textContent = d.rssi;
        
        const heaterStat = document.getElementById('heaterStat');
        heaterStat.textContent = d.heater ? 'ON' : 'OFF';
        heaterStat.className = 'stat-value ' + (d.heater ? 'on' : 'off');

        const atomizerStat = document.getElementById('atomizerStat');
        atomizerStat.textContent = d.atomizer ? 'ON' : 'OFF';
        atomizerStat.className = 'stat-value ' + (d.atomizer ? 'on' : 'off');

        const badge = document.getElementById('stageBadge');
        badge.textContent = d.stageLockdown ? 'Lockdown Stage' : 'Incubation Stage';
        badge.className = 'stage-badge ' + (d.stageLockdown ? 'badge-lockdown' : 'badge-incubation');

        ['heater', 'atomizer', 'fan', 'servo'].forEach(key => {
          const mode = d[key + 'Mode'];
          const btn = document.getElementById('btn-' + key);
          btn.textContent = mode === 1 ? 'AUTO' : 'OFF';
          btn.className = 'btn ' + (mode === 1 ? 'btn-auto' : 'btn-off');
          currentModes[key] = mode;
        });

        const logs = decodeLogs(d.logs, d.logCount);
        if (logs.length > 0) {
          const now = logs[logs.length-1].t;
          envChart.data.datasets[0].data = logs.map(l => ({ x: (l.t - now)/1000, y: l.temp }));
          envChart.data.datasets[1].data = logs.map(l => ({ x: (l.t - now)/1000, y: l.hum }));
          envChart.update('none');

          actChart.data.datasets[0].data = logs.map(l => ({ x: (l.t - now)/1000, y: l.h }));
          actChart.data.datasets[1].data = logs.map(l => ({ x: (l.t - now)/1000, y: l.a }));
          actChart.data.datasets[2].data = logs.map(l => ({ x: (l.t - now)/1000, y: l.f }));
          actChart.data.datasets[3].data = logs.map(l => ({ x: (l.t - now)/1000, y: l.s }));
          actChart.update('none');
        }
      });
    }

    function toggleMode(dev) {
      const newMode = currentModes[dev] === 1 ? 'off' : 'auto';
      fetch(`/control?device=${dev}&mode=${newMode}`).then(() => update());
    }

    function setStage() {
      const s = document.getElementById('stageSelect').value;
      fetch(`/mock/api?stageType=${s}`).then(() => update());
    }

    initCharts();
    update();
    setInterval(update, 5000);
  </script>
</body>
</html>
)rawliteral";

const char MOCK_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Advanced Settings</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: sans-serif; padding: 20px; background: #f0f2f5; }
    .card { background: white; padding: 20px; border-radius: 8px; max-width: 400px; margin: 0 auto 20px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
    .row { margin-bottom: 15px; display: flex; justify-content: space-between; align-items: center; }
    input, select { padding: 5px; }
    button { padding: 10px; width: 100%; cursor: pointer; }
    a { display: block; text-align: center; margin-top: 20px; color: #1877f2; text-decoration: none; }
  </style>
</head>
<body>
  <div class="card">
    <h3>Intervals</h3>
    <div class="row">
      <label>Log Interval (ms)</label>
      <input type="number" id="logInterval" value="10000">
    </div>
    <button onclick="save('logInterval')">Update Log Interval</button>
    <div class="row" style="margin-top:10px">
      <label>Egg Turn (ms)</label>
      <input type="number" id="eggTurnInterval" value="7200000">
    </div>
    <button onclick="save('eggTurnInterval')">Update Turn Interval</button>
  </div>

  <div class="card">
    <h3>Simulation</h3>
    <div class="row">
      <label>Mock Sensor</label>
      <input type="checkbox" id="mockEnable" onchange="toggle('enable')">
    </div>
    <div class="row">
      <label>Auto Sim</label>
      <input type="checkbox" id="autoSim" onchange="toggle('autosim')">
    </div>
    <div class="row">
      <label>Temp</label>
      <input type="number" id="mTemp" step="0.1" value="37.5">
    </div>
    <div class="row">
      <label>Hum</label>
      <input type="number" id="mHum" step="0.1" value="60">
    </div>
    <button onclick="setMock()">Set Mock Values</button>
  </div>
  
  <a href="/">← Back to Dashboard</a>

  <script>
    function load() {
      fetch('/mock/api').then(r => r.json()).then(d => {
        document.getElementById('logInterval').value = d.logInterval;
        document.getElementById('eggTurnInterval').value = d.eggTurnInterval;
        document.getElementById('mockEnable').checked = d.enabled;
        document.getElementById('autoSim').checked = d.autosim;
        document.getElementById('mTemp').value = d.temp;
        document.getElementById('mHum').value = d.hum;
      });
    }
    function save(key) {
      const val = document.getElementById(key).value;
      fetch(`/mock/api?${key}=${val}`).then(load);
    }
    function toggle(key) {
      const val = document.getElementById(key === 'enable' ? 'mockEnable' : 'autoSim').checked ? 1 : 0;
      fetch(`/mock/api?${key}=${val}`).then(load);
    }
    function setMock() {
      const t = document.getElementById('mTemp').value;
      const h = document.getElementById('mHum').value;
      fetch(`/mock/api?temp=${t}&hum=${h}`).then(load);
    }
    load();
  </script>
</body>
</html>
)rawliteral";

#endif
