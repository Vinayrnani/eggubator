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
    .btn-auto { background: #2196F3; color: white; width: 100%; }
    .target-info { text-align: center; color: #555; font-size: 13px; }
    .chart-container { position: relative; height: 160px; width: 100%; }
    canvas { background: #fafafa; border-radius: 8px; touch-action: none; }
    .alert { background: #ff9800; color: white; padding: 10px; border-radius: 8px; margin-bottom: 15px; display: none; text-align: center; }
    .alert.show { display: block; animation: fadeIn 0.3s; }
    @keyframes fadeIn { from { opacity: 0; transform: translateY(-10px); } to { opacity: 1; transform: translateY(0); } }
    .version { text-align: center; color: rgba(255,255,255,0.8); font-size: 11px; margin-top: 10px; }
    .uptime-tag { background: rgba(255,255,255,0.3); padding: 6px 15px; border-radius: 20px; color: #333; font-size: 12px; font-weight: 600; }
    .stage-row { display: flex; align-items: center; justify-content: center; gap: 10px; margin-top: 8px; }
    .stage-row select { padding: 6px 10px; border: 2px solid #667eea; border-radius: 6px; font-size: 12px; font-weight: 600; color: #333; background: white; min-width: 200px; }
    .egg-turner-status { text-align: center; font-size: 11px; color: #f44336; font-weight: 600; margin-top: 4px; }
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
          <span class="device-label">Egg Turner</span>
          <span class="mode-text" id="servoModeText">AUTO</span>
          <label class="switch">
            <input type="checkbox" id="servoModeSwitch" onchange="toggleDeviceMode('servo')">
            <span class="slider"></span>
          </label>
         </div>
        <div class="uptime-tag" id="uptime">--</div>
      </div>
      <div class="stage-row">
        <select id="mainStageSelect" onchange="saveMainStage()">
          <option value="incubation">Incubation (Days 1-18)</option>
          <option value="lockdown">Lockdown (Days 19-21)</option>
        </select>
      </div>
      <div class="egg-turner-status" id="eggTurnerStatus"></div>
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
      <div class="target-info" id="targets">Target: 37.5°C | 60%</div>
    </div>
    <div class="card">
      <button class="btn btn-auto" onclick="checkOta()">Check Update</button>
      <div style="text-align:center;margin-top:8px;font-size:12px;color:#666;" id="otaStatus"></div>
      <div style="text-align:center;margin-top:4px;"><a href="/mock" style="color:#666;font-size:10px;text-decoration:none;">Settings & Simulation</a></div>
    </div>
    <div class="card">
      <div class="label">Temperature</div>
      <div class="chart-container"><canvas id="tempChart"></canvas></div>
    </div>
    <div class="card">
      <div class="label">Humidity</div>
      <div class="chart-container"><canvas id="humChart"></canvas></div>
    </div>
    <div class="card">
      <div class="label">Controls</div>
      <div class="chart-container"><canvas id="ctrlChart"></canvas></div>
    </div>
    <div class="version">v<span id="version">--</span></div>
  </div>
  <script>
    const devices = [
      { id: 'heater', name: 'Heater' },
      { id: 'atomizer', name: 'Atomizer' },
      { id: 'fan', name: 'Fan' },
      { id: 'servo', name: 'Egg Turner' }
    ];
    let charts = {};
    
    function initDevices() {
      const grid = document.getElementById('deviceGrid');
      grid.innerHTML = '';
      devices.forEach(d => {
        grid.innerHTML += '<div class="device-card"><div class="device-name">'+d.name+'</div><div class="device-status" id="'+d.id+'">OFF</div></div>';
      });
    }
    
    function createChart(id, label, color, yLabel) {
      const ctx = document.getElementById(id).getContext('2d');
      return new Chart(ctx, {
        type: 'line',
        data: { datasets: [{ label: label, borderColor: color, backgroundColor: color + '22', data: [], tension: 0.3, fill: true, pointRadius: 0 }] },
        options: {
          responsive: true,
          maintainAspectRatio: false,
          animation: false,
          scales: {
            x: { type: 'linear', position: 'bottom', title: { display: true, text: 'Time (s ago)', font: { size: 10 } } },
            y: { title: { display: true, text: yLabel, font: { size: 10 } } }
          },
          plugins: {
            zoom: {
              pan: { enabled: true, mode: 'x' },
              zoom: { wheel: { enabled: true }, pinch: { enabled: true }, mode: 'x' }
            },
            legend: { display: false }
          }
        }
      });
    }

    function createCtrlChart() {
      const ctx = document.getElementById('ctrlChart').getContext('2d');
      return new Chart(ctx, {
        type: 'line',
        data: {
          datasets: [
            { label: 'Heater', borderColor: '#f44336', data: [], stepped: true, pointRadius: 0 },
            { label: 'Spray', borderColor: '#2196F3', data: [], stepped: true, pointRadius: 0 },
            { label: 'Fan', borderColor: '#4CAF50', data: [], stepped: true, pointRadius: 0 }
          ]
        },
        options: {
          responsive: true,
          maintainAspectRatio: false,
          animation: false,
          scales: {
            x: { type: 'linear', position: 'bottom' },
            y: { min: -0.1, max: 1.1, ticks: { callback: v => v==1?'ON':(v==0?'OFF':'') } }
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

    function initCharts() {
      // chartjs-plugin-zoom should be automatically registered if included via UMD script tag
      charts.temp = createChart('tempChart', 'Temp', '#ff5722', '°C');
      charts.hum = createChart('humChart', 'Hum', '#2196F3', '%');
      charts.ctrl = createCtrlChart();
    }

    function updateData() {
      fetch('/data').then(r => r.json()).then(d => {
        document.getElementById('version').textContent = d.version;
        document.getElementById('temp').textContent = d.temperature.toFixed(1)+'°C';
        document.getElementById('hum').textContent = d.humidity.toFixed(1)+'%';
        document.getElementById('uptime').textContent = d.uptime;
        document.getElementById('mainStageSelect').value = d.stageLockdown ? 'lockdown' : 'incubation';
        const eggTurnerStatus = document.getElementById('eggTurnerStatus');
        eggTurnerStatus.textContent = d.stageLockdown ? 'Egg Turner: OFF (lockdown)' : '';
        
        devices.forEach(dev => {
          const el = document.getElementById(dev.id);
          el.textContent = d[dev.id] ? 'ON' : 'OFF';
          el.className = 'device-status ' + (d[dev.id] ? 'on' : 'off');
        });
        
        ['heater', 'atomizer', 'fan', 'servo'].forEach(dev => {
          const mode = d[dev + 'Mode'];
          const modeText = document.getElementById(dev + 'ModeText');
          const modeSwitch = document.getElementById(dev + 'ModeSwitch');
          modeText.textContent = mode === 0 ? 'OFF' : 'AUTO';
          modeText.className = 'mode-text ' + (mode === 0 ? 'killed' : 'auto');
          modeSwitch.checked = (mode === 0);
        });

        if (d.log && d.log.length > 0) {
          const now = d.log[d.log.length-1].t;
          const tempData = d.log.map(p => ({ x: (p.t - now)/1000, y: p.temp }));
          const humData = d.log.map(p => ({ x: (p.t - now)/1000, y: p.hum }));
          const hData = d.log.map(p => ({ x: (p.t - now)/1000, y: p.h }));
          const aData = d.log.map(p => ({ x: (p.t - now)/1000, y: p.a }));
          const fData = d.log.map(p => ({ x: (p.t - now)/1000, y: p.f }));

          charts.temp.data.datasets[0].data = tempData;
          charts.temp.update('none');
          charts.hum.data.datasets[0].data = humData;
          charts.hum.update('none');
          charts.ctrl.data.datasets[0].data = hData;
          charts.ctrl.data.datasets[1].data = aData;
          charts.ctrl.data.datasets[2].data = fData;
          charts.ctrl.update('none');
        }
      }).catch(e => console.error('Data fetch error:', e));
    }

    function toggleDeviceMode(device) {
      const modeSwitch = document.getElementById(device + 'ModeSwitch');
      const mode = modeSwitch.checked ? 'off' : 'auto';
      fetch('/control?device='+device+'&mode='+mode).then(() => updateData());
    }

    function saveMainStage() {
      const stage = document.getElementById('mainStageSelect').value;
      fetch('/mock/api?stageType=' + stage).then(() => updateData());
    }

    function checkOta() {
      document.getElementById('otaStatus').textContent = 'Checking...';
      fetch('/ota/check').then(r => r.json()).then(d => {
        document.getElementById('otaStatus').textContent = d.update ? 'Update available: '+d.version : 'Up to date';
        if (d.update) fetch('/ota/update');
      });
    }

    initDevices();
    initCharts();
    setInterval(updateData, 2000);
    updateData();
  </script>
</body>
</html>
)rawliteral";

const char MOCK_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Settings - EGGubator</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    * { box-sizing: border-box; }
    body { font-family: 'Segoe UI', Arial, sans-serif; margin: 0; padding: 0; background: linear-gradient(160deg, #1a1a2e 0%, #16213e 50%, #0f3460 100%); min-height: 100vh; }
    .container { max-width: 420px; margin: 0 auto; padding: 12px; }
    .card { background: rgba(255,255,255,0.95); padding: 16px; border-radius: 16px; box-shadow: 0 8px 32px rgba(0,0,0,0.3); margin-bottom: 12px; }
    .card-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 12px; padding-bottom: 8px; border-bottom: 2px solid #e94560; }
    .card-title { color: #1a1a2e; font-size: 14px; font-weight: 700; text-transform: uppercase; letter-spacing: 1px; }
    h1 { color: white; text-align: center; font-size: 28px; margin: 8px 0 12px 0; text-shadow: 0 2px 4px rgba(0,0,0,0.3); }
    .header { display: flex; justify-content: space-between; align-items: center; background: rgba(255,255,255,0.15); padding: 10px 16px; border-radius: 12px; margin-bottom: 12px; backdrop-filter: blur(10px); }
    .back-link { color: white; text-decoration: none; font-size: 14px; font-weight: 600; }
    .input-row { display: flex; align-items: center; justify-content: space-between; margin-bottom: 10px; }
    .input-row label { color: #333; font-size: 13px; font-weight: 600; }
    select, input[type="number"] { padding: 8px; border: 2px solid #ddd; border-radius: 8px; font-size: 14px; font-weight: 600; }
    .btn { padding: 10px; border: none; border-radius: 8px; cursor: pointer; font-size: 14px; font-weight: 600; width: 100%; background: #e94560; color: white; }
    .switch { position: relative; display: inline-block; width: 50px; height: 24px; }
    .switch input { opacity: 0; width: 0; height: 0; }
    .slider { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; transition: .4s; border-radius: 24px; }
    .slider:before { position: absolute; content: ""; height: 18px; width: 18px; left: 3px; bottom: 3px; background: white; transition: .4s; border-radius: 50%; }
    input:checked + .slider { background-color: #e94560; }
    input:checked + .slider:before { transform: translateX(26px); }
    .sys-row { display: flex; justify-content: space-between; padding: 5px 0; border-bottom: 1px solid #eee; font-size: 12px; }
  </style>
</head>
<body>
  <div class="container">
    <h1>EGGubator</h1>
    <div class="header"><a href="/" class="back-link">← Back</a></div>
    <div class="card">
      <div class="card-header"><span class="card-title">Timing Settings</span></div>
      <div class="input-row"><label>Log Interval:</label><select id="logInterval" onchange="saveSetting('logInterval')"><option value="5000">5s</option><option value="10000">10s</option><option value="30000">30s</option></select></div>
      <div class="input-row"><label>Egg Turner:</label><select id="eggTurnInterval" onchange="saveSetting('eggTurnInterval')"><option value="7200000">2h</option><option value="14400000">4h</option></select></div>
    </div>
    <div class="card">
      <div class="card-header"><span class="card-title">Simulation</span></div>
      <div class="input-row"><label>Mock Sensor</label><label class="switch"><input type="checkbox" id="mockSwitch" onchange="toggleSetting('enable')"><span class="slider"></span></label></div>
      <div class="input-row"><label>Auto Simulation</label><label class="switch"><input type="checkbox" id="autoSimSwitch" onchange="toggleSetting('autosim')"><span class="slider"></span></label></div>
      <div id="mockVals" style="display:none">
        <div class="input-row"><label>Temp:</label><input type="number" id="mTemp" step="0.1" value="37.5"></div>
        <div class="input-row"><label>Hum:</label><input type="number" id="mHum" step="0.1" value="60"></div>
        <button class="btn" onclick="setMock()">Apply</button>
      </div>
    </div>
    <div class="card">
      <div class="card-header"><span class="card-title">System</span></div>
      <div class="sys-row"><span>Heap</span><span id="sysHeap">--</span></div>
      <div class="sys-row"><span>CPU</span><span id="sysCpu">--</span></div>
      <div class="sys-row"><span>Uptime</span><span id="sysUptime">--</span></div>
    </div>
  </div>
  <script>
    function updateData() {
      fetch('/data').then(r => r.json()).then(d => {
        document.getElementById('mockSwitch').checked = d.mock;
        document.getElementById('autoSimSwitch').checked = d.autosim;
        document.getElementById('mockVals').style.display = d.mock ? 'block' : 'none';
        document.getElementById('sysHeap').textContent = Math.round(d.sys.heapFree/1024) + 'KB';
        document.getElementById('sysCpu').textContent = d.sys.cpu + '%';
        document.getElementById('sysUptime').textContent = d.uptime;
      });
    }
    function saveSetting(key) {
      fetch('/mock/api?' + key + '=' + document.getElementById(key).value);
    }
    function toggleSetting(key) {
      const el = document.getElementById(key == 'enable' ? 'mockSwitch' : 'autoSimSwitch');
      const val = el.checked ? 1 : 0;
      fetch('/mock/api?' + key + '=' + val).then(() => updateData());
    }
    function setMock() {
      fetch('/mock/api?temp=' + document.getElementById('mTemp').value + '&hum=' + document.getElementById('mHum').value);
    }
    setInterval(updateData, 3000);
    updateData();
  </script>
</body>
</html>
)rawliteral";

#endif
