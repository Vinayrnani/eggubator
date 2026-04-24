#ifndef WEB_UI_H
#define WEB_UI_H

#define CHARTJS_ASSET_URL "https://cdn.jsdelivr.net/npm/chart.js"
#define CHARTJS_ZOOM_URL "https://cdn.jsdelivr.net/npm/chartjs-plugin-zoom"

const char WEB_ROOT_HTML[] PROGMEM =
R"webui(
<!DOCTYPE html>
<html>
<head>
  <title>EGGubator</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    * { box-sizing: border-box; }
    body { font-family: 'Segoe UI', Arial, sans-serif; margin: 0; padding: 0; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); min-height: 100vh; }
    .container { max-width: 600px; margin: 0 auto; padding: 15px; }
    .card { background: white; padding: 15px; border-radius: 12px; box-shadow: 0 4px 15px rgba(0,0,0,0.2); margin-bottom: 15px; }
    h1 { color: white; text-align: center; font-size: 28px; margin: 10px 0 15px 0; }
    h1 span { color: #ffd700; }
    .header { display: grid; grid-template-columns: 1fr 1fr; gap: 8px; background: rgba(255,255,255,0.25); padding: 12px 15px; border-radius: 12px; margin-bottom: 15px; }
    .header .uptime-tag { grid-column: span 2; text-align: center; margin-top: 5px; }
    .mode-group { display: flex; align-items: center; gap: 6px; background: white; padding: 8px 12px; border-radius: 8px; box-shadow: 0 1px 3px rgba(0,0,0,0.1); }
    .mode-group label { font-weight: 600; color: #333; font-size: 13px; }
    .device-label { font-weight: 600; color: #555; font-size: 11px; min-width: 65px; }
    .switch { position: relative; display: inline-block; width: 52px; height: 26px; }
    .switch input { opacity: 0; width: 0; height: 0; }
    .slider { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: #e0e0e0; transition: .3s; border-radius: 26px; }
    .slider:before { position: absolute; content: ""; height: 22px; width: 22px; left: 2px; bottom: 2px; background: white; transition: .3s; border-radius: 50%; box-shadow: 0 1px 3px rgba(0,0,0,0.2); }
    input:checked + .slider { background-color: #4CAF50; }
    input:checked + .slider:before { transform: translateX(26px); }
    .mode-text { font-size: 10px; font-weight: 700; text-transform: uppercase; letter-spacing: 0.5px; }
    .mode-text.killed { color: #f44336; }
    .mode-text.auto { color: #4CAF50; }
    .info-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; }
    .stat-box { text-align: center; }
    .label { color: #777; font-size: 12px; }
    .stat { font-size: 26px; font-weight: bold; margin: 4px 0; }
    .on { color: #4CAF50; }
    .off { color: #f44336; }
    .device-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; }
    .device-card { background: #f8f9fa; padding: 12px; border-radius: 10px; text-align: center; }
    .device-name { font-size: 12px; color: #666; margin-bottom: 4px; }
    .device-status { font-size: 18px; font-weight: bold; margin-bottom: 8px; }
    .btn-group { display: flex; gap: 5px; justify-content: center; }
    .btn { padding: 8px 16px; margin: 0; border: none; border-radius: 6px; cursor: pointer; font-size: 13px; font-weight: 500; transition: opacity 0.2s; }
    .btn:disabled { opacity: 0.5; cursor: not-allowed; filter: grayscale(50%); }
    .btn-on { background: #4CAF50; color: white; }
    .btn-off { background: #f44336; color: white; }
    .btn-auto { background: #2196F3; color: white; width: 100%; }
    .target-info { text-align: center; color: #555; font-size: 13px; }
    .chart-wrap { position: relative; height: 200px; }
    canvas { background: #fafafa; border-radius: 8px; }
    .legend { display: flex; flex-wrap: wrap; justify-content: center; gap: 12px; margin-top: 8px; font-size: 11px; }
    .legend-item { display: flex; align-items: center; gap: 4px; }
    .legend-dot { width: 10px; height: 10px; border-radius: 50%; }
    .time-label { text-align: center; font-size: 10px; color: #999; margin-top: 4px; }
    .alert { background: #ff9800; color: white; padding: 10px; border-radius: 8px; margin-bottom: 15px; display: none; text-align: center; }
    .alert.show { display: block; animation: fadeIn 0.3s; }
    @keyframes fadeIn { from { opacity: 0; transform: translateY(-10px); } to { opacity: 1; transform: translateY(0); } }
    .version { text-align: center; color: rgba(255,255,255,0.8); font-size: 11px; margin-top: 10px; }
    .uptime-tag { background: rgba(255,255,255,0.3); padding: 6px 15px; border-radius: 20px; color: #333; font-size: 12px; font-weight: 600; }
    .stage-row { display: flex; align-items: center; justify-content: center; gap: 10px; margin-top: 8px; }
    .stage-row select { padding: 6px 10px; border: 2px solid #667eea; border-radius: 6px; font-size: 12px; font-weight: 600; color: #333; background: white; min-width: 200px; }
    .egg-turner-status { text-align: center; font-size: 11px; color: #f44336; font-weight: 600; margin-top: 4px; }
    .countdown-timer { background: rgba(255,255,255,0.3); padding: 4px 10px; border-radius: 12px; font-size: 11px; font-weight: 600; color: #333; }
    @media (max-width: 400px) { .device-grid { grid-template-columns: 1fr; } .info-grid { grid-template-columns: 1fr; } }
  </style>
</head>
<body>
  <div class="container">
    <h1><span>🥚</span> EGGubator</h1>
    <div class="header">
      <div class="mode-group">
        <span class="device-label">Heater</span>
        <span class="mode-text" id="heaterModeText">AUTO</span>
        <label class="switch">
          <input type="checkbox" id="heaterModeSwitch" onchange="toggleDeviceMode('heater')">
          <span class="slider"></span>
        </label>
      </div>
      <div class="mode-group">
        <span class="device-label">Spray</span>
        <span class="mode-text" id="atomizerModeText">AUTO</span>
        <label class="switch">
          <input type="checkbox" id="atomizerModeSwitch" onchange="toggleDeviceMode('atomizer')">
          <span class="slider"></span>
        </label>
      </div>
      <div class="mode-group">
        <span class="device-label">Fan</span>
        <span class="mode-text" id="fanModeText">AUTO</span>
        <label class="switch">
          <input type="checkbox" id="fanModeSwitch" onchange="toggleDeviceMode('fan')">
          <span class="slider"></span>
        </label>
      </div>
      <div class="mode-group">
        <span class="device-label">Turner</span>
        <span class="mode-text" id="servoModeText">AUTO</span>
        <label class="switch">
          <input type="checkbox" id="servoModeSwitch" onchange="toggleDeviceMode('servo')">
          <span class="slider"></span>
        </label>
      </div>
    </div>
    <div class="stage-row">
      <select id="mainStageSelect" onchange="saveMainStage()">
        <option value="incubation">Incubation (Days 1-18)</option>
        <option value="lockdown">Lockdown (Days 19-21)</option>
      </select>
    </div>
    <div class="egg-turner-status" id="eggTurnerStatus"></div>
    <div class="header" style="display:flex;justify-content:space-between;align-items:center;margin-top:10px;">
      <div class="uptime-tag" id="uptime">--</div>
      <a href="/mock" style="color:white;text-decoration:none;font-size:12px;font-weight:bold;">Settings →</a>
    </div>
    <div class="alert" id="alertBox"></div>
    <div class="card">
      <div class="info-grid">
        <div class="stat-box"><div class="label">Temperature</div><div class="stat" id="temp">--°C</div></div>
        <div class="stat-box"><div class="label">Humidity</div><div class="stat" id="hum">--%</div></div>
      </div>
    </div>
    <div class="card">
      <div class="device-grid" id="deviceGrid"></div>
    </div>
    <div class="card">
      <div class="target-info" id="targets">Target: --°C | --%</div>
    </div>
    <div class="card">
      <div class="label">Temperature (°C)</div>
      <div class="chart-wrap"><canvas id="tempChart"></canvas></div>
    </div>
    <div class="card">
      <div class="label">Humidity (%)</div>
      <div class="chart-wrap"><canvas id="humChart"></canvas></div>
    </div>
    <div class="card">
      <div class="label">Controls State</div>
      <div class="chart-wrap"><canvas id="ctrlChart"></canvas></div>
      <div class="legend" id="ctrlLegend"></div>
    </div>
    <div class="version">v<span id="version">--</span></div>
  </div>
)webui"
"<script src=\"" CHARTJS_ASSET_URL "\"></script>\n"
"<script src=\"" CHARTJS_ZOOM_URL "\"></script>\n"
R"webui(
  <script>
    const devices = [
      { id: 'heater', name: 'Heater' },
      { id: 'atomizer', name: 'Atomizer' },
      { id: 'fan', name: 'Fan' },
      { id: 'servo', name: 'Egg Turner' }
    ];
    const ctrlDevices = [
      { key: 'h', name: 'Heater', color: '#f44336' },
      { key: 'a', name: 'Atomizer', color: '#2196F3' },
      { key: 'f', name: 'Fan', color: '#4CAF50' },
      { key: 's', name: 'Turner', color: '#FF9800' }
    ];

    let charts = {};
    let currentTargetTemp = 37.5;
    let currentTargetHumidity = 60.0;

    function initDevices() {
      const grid = document.getElementById('deviceGrid');
      grid.innerHTML = devices.map(d => `<div class="device-card"><div class="device-name">${d.name}</div><div class="device-status" id="${d.id}">OFF</div></div>`).join('');
    }

    function formatTimeLabel(ms) {
      const s = Math.floor(ms / 1000);
      if (s < 60) return s + 's';
      const m = Math.floor(s / 60);
      if (m < 60) return m + 'm';
      const h = Math.floor(m / 60);
      return h + 'h';
    }

    function initCharts() {
      const commonOpts = {
        responsive: true,
        maintainAspectRatio: false,
        animation: false,
        scales: {
          x: {
            type: 'linear',
            ticks: { callback: v => formatTimeLabel(v), maxTicksLimit: 6 }
          },
          y: { beginAtZero: false }
        },
        plugins: {
          legend: { display: false },
          zoom: {
            zoom: {
              wheel: { enabled: true },
              pinch: { enabled: true },
              mode: 'x'
            },
            pan: { enabled: true, mode: 'x' }
          }
        }
      };

      charts.temp = new Chart(document.getElementById('tempChart'), {
        type: 'line',
        data: { datasets: [{ label: 'Temp', borderColor: '#ff5722', data: [], tension: 0.2, pointRadius: 0 }] },
        options: commonOpts
      });

      charts.hum = new Chart(document.getElementById('humChart'), {
        type: 'line',
        data: { datasets: [{ label: 'Hum', borderColor: '#2196F3', data: [], tension: 0.2, pointRadius: 0 }] },
        options: commonOpts
      });

      charts.ctrl = new Chart(document.getElementById('ctrlChart'), {
        type: 'line',
        data: { datasets: ctrlDevices.map(d => ({ label: d.name, borderColor: d.color, data: [], stepped: true, pointRadius: 0 })) },
        options: { ...commonOpts, scales: { ...commonOpts.scales, y: { min: 0, max: 1, ticks: { stepSize: 1 } } } }
      });

      document.getElementById('ctrlLegend').innerHTML = ctrlDevices.map(d => `<div class="legend-item"><div class="legend-dot" style="background:${d.color}"></div><span>${d.name}</span></div>`).join('');
    }

    async function updateData() {
      try {
        const response = await fetch('/data');
        const data = await response.json();
        
        document.getElementById('version').textContent = data.version;
        document.getElementById('temp').textContent = data.temperature.toFixed(1) + '°C';
        document.getElementById('hum').textContent = data.humidity.toFixed(1) + '%';
        document.getElementById('uptime').textContent = data.uptime;
        document.getElementById('mainStageSelect').value = data.stageLockdown ? 'lockdown' : 'incubation';
        document.getElementById('targets').textContent = `Target: ${data.targetTemp.toFixed(1)}°C | ${data.targetHumidity.toFixed(0)}%`;
        
        devices.forEach(d => {
          const el = document.getElementById(d.id);
          el.textContent = data[d.id] ? 'ON' : 'OFF';
          el.className = 'device-status ' + (data[d.id] ? 'on' : 'off');
        });

        ['heater', 'atomizer', 'fan', 'servo'].forEach(d => {
          const mode = data[d + 'Mode'];
          document.getElementById(d + 'ModeText').textContent = mode === 0 ? 'OFF' : 'AUTO';
          document.getElementById(d + 'ModeText').className = 'mode-text ' + (mode === 0 ? 'killed' : 'auto');
          document.getElementById(d + 'ModeSwitch').checked = mode === 0;
        });

        if (data.log && data.log.length > 0) {
          charts.temp.data.datasets[0].data = data.log.map(p => ({ x: p.t, y: p.temp }));
          charts.hum.data.datasets[0].data = data.log.map(p => ({ x: p.t, y: p.hum }));
          ctrlDevices.forEach((d, i) => {
            charts.ctrl.data.datasets[i].data = data.log.map(p => ({ x: p.t, y: p[d.key] ? 1 : 0 }));
          });
          Object.values(charts).forEach(c => c.update('none'));
        }
      } catch (e) { console.error(e); }
    }

    function toggleDeviceMode(device) {
      const mode = document.getElementById(device + 'ModeSwitch').checked ? 'off' : 'auto';
      fetch(`/control?device=${device}&mode=${mode}`).then(() => updateData());
    }

    function saveMainStage() {
      fetch(`/mock/api?stageType=${document.getElementById('mainStageSelect').value}`).then(() => updateData());
    }

    initDevices();
    initCharts();
    updateData();
    setInterval(updateData, 5000);
  </script>
</body>
</html>
)webui";

const char WEB_MOCK_HTML[] PROGMEM = R"webui(
<!DOCTYPE html>
<html>
<head>
  <title>Settings - EGGubator</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    * { box-sizing: border-box; }
    body { font-family: 'Segoe UI', Arial, sans-serif; margin: 0; padding: 0; background: linear-gradient(160deg, #1a1a2e 0%, #16213e 50%, #0f3460 100%); min-height: 100vh; }
    .container { max-width: 420px; margin: 0 auto; padding: 12px; }
    .card { background: rgba(255,255,255,0.95); padding: 16px; border-radius: 16px; shadow: 0 8px 32px rgba(0,0,0,0.3); margin-bottom: 12px; }
    .card-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 12px; padding-bottom: 8px; border-bottom: 2px solid #e94560; }
    .card-title { color: #1a1a2e; font-size: 14px; font-weight: 700; text-transform: uppercase; }
    h1 { color: white; text-align: center; font-size: 28px; margin: 8px 0 12px 0; }
    .header { display: flex; justify-content: space-between; align-items: center; background: rgba(255,255,255,0.15); padding: 10px 16px; border-radius: 12px; margin-bottom: 12px; }
    .back-link { color: white; text-decoration: none; font-size: 14px; font-weight: 600; }
    .input-row { display: flex; align-items: center; justify-content: space-between; margin-bottom: 10px; }
    select, input { padding: 8px; border: 2px solid #e94560; border-radius: 8px; }
    .btn { padding: 10px; border: none; border-radius: 8px; cursor: pointer; font-weight: 600; width: 100%; background: #e94560; color: white; }
    .switch { position: relative; display: inline-block; width: 50px; height: 24px; }
    .switch input { opacity: 0; width: 0; height: 0; }
    .slider { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; transition: .4s; border-radius: 24px; }
    .slider:before { position: absolute; content: ""; height: 18px; width: 18px; left: 3px; bottom: 3px; background-color: white; transition: .4s; border-radius: 50%; }
    input:checked + .slider { background-color: #e94560; }
    input:checked + .slider:before { transform: translateX(26px); }
    .sys-row { display: flex; justify-content: space-between; padding: 6px 0; border-bottom: 1px solid #eee; font-size: 12px; }
  </style>
</head>
<body>
  <div class="container">
    <h1>Settings</h1>
    <div class="header"><a href="/" class="back-link">← Back</a></div>
    <div class="card">
      <div class="card-header"><span class="card-title">Configuration</span></div>
      <div class="input-row"><label>Log Interval</label><select id="logInterval" onchange="saveSetting('logInterval')">
        <option value="5000">5s</option><option value="10000">10s</option><option value="30000">30s</option><option value="60000">1m</option>
      </select></div>
      <div class="input-row"><label>Turner Interval</label><select id="eggTurnInterval" onchange="saveSetting('eggTurnInterval')">
        <option value="3600000">1h</option><option value="7200000">2h</option><option value="14400000">4h</option>
      </select></div>
    </div>
    <div class="card">
      <div class="card-header"><span class="card-title">Simulation</span></div>
      <div class="input-row"><span>Mock Sensor</span><label class="switch"><input type="checkbox" id="mockSwitch" onchange="toggleMock()"><span class="slider"></span></label></div>
      <div class="input-row"><span>Auto Sim</span><label class="switch"><input type="checkbox" id="autoSimSwitch" onchange="toggleAutoSim()"><span class="slider"></span></label></div>
    </div>
    <div class="card">
      <div class="card-header"><span class="card-title">System</span></div>
      <div class="sys-row"><span>Heap</span><span id="sysHeap">--</span></div>
      <div class="sys-row"><span>Uptime</span><span id="sysUptime">--</span></div>
    </div>
    <button class="btn" onclick="fetch('/reboot')">Reboot Device</button>
  </div>
  <script>
    function updateData() {
      fetch('/data').then(r => r.json()).then(d => {
        document.getElementById('sysHeap').textContent = Math.round(d.sys.heapFree/1024) + ' KB free';
        document.getElementById('sysUptime').textContent = d.uptime;
        document.getElementById('mockSwitch').checked = d.mock;
        document.getElementById('autoSimSwitch').checked = d.autosim;
      });
      fetch('/mock/api').then(r => r.json()).then(d => {
        document.getElementById('logInterval').value = d.logInterval;
        document.getElementById('eggTurnInterval').value = d.eggTurnInterval;
      });
    }
    function saveSetting(key) {
      const val = document.getElementById(key).value;
      fetch(`/mock/api?${key}=${val}`);
    }
    function toggleMock() { fetch(`/mock/api?enable=${document.getElementById('mockSwitch').checked ? 1 : 0}`).then(() => updateData()); }
    function toggleAutoSim() { fetch(`/mock/api?autosim=${document.getElementById('autoSimSwitch').checked ? 1 : 0}`).then(() => updateData()); }
    updateData();
    setInterval(updateData, 5000);
  </script>
</body>
</html>
)webui";

#endif
