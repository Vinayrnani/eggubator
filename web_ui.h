#ifndef WEB_UI_H
#define WEB_UI_H

#include <Arduino.h>

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>EGGubator Dashboard</title>
  <script src="https://cdn.jsdelivr.net/npm/chart.js@4.4.1/dist/chart.umd.min.js"></script>
  <script src="https://cdn.jsdelivr.net/npm/hammerjs@2.0.8"></script>
  <script src="https://cdn.jsdelivr.net/npm/chartjs-plugin-zoom@2.0.1/dist/chartjs-plugin-zoom.min.js"></script>
  <style>
    :root {
      --primary: #1877f2;
      --primary-dark: #166fe5;
      --primary-soft: #e7f3ff;
      --bg: #f0f2f5;
      --card-bg: #ffffff;
      --text: #1c1e21;
      --text-muted: #65676b;
      --on: #42b72a;
      --off: #f02849;
      --idle: #8a8d91;
    }
    * { box-sizing: border-box; }
    body { font-family: 'Segoe UI', Roboto, Helvetica, Arial, sans-serif; margin: 0; background: var(--bg); color: var(--text); line-height: 1.5; }
    .container { max-width: 1400px; margin: 0 auto; padding: 20px; width: 100%; }
    .header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 24px; background: linear-gradient(135deg, var(--primary), var(--primary-dark)); padding: 20px 28px; border-radius: 20px; box-shadow: 0 4px 12px rgba(24, 119, 242, 0.3); color: white; }
    h1 { margin: 0; font-size: 26px; font-weight: 800; letter-spacing: -0.5px; color: white; }
    .card { background: var(--card-bg); padding: 28px; border-radius: 20px; box-shadow: 0 4px 20px rgba(0,0,0,0.04); margin-bottom: 24px; border: 1px solid rgba(0,0,0,0.05); }
    .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(140px, 1fr)); gap: 16px; }
    .stat-card { background: #ffffff; padding: 18px; border-radius: 16px; text-align: center; border: 1px solid #edf0f5; transition: all 0.2s ease; }
    .stat-card:hover { transform: translateY(-3px); border-color: var(--primary-soft); background: #fafafa; }
    .stat-label { font-size: 11px; color: var(--text-muted); font-weight: 800; text-transform: uppercase; letter-spacing: 1px; margin-bottom: 4px; }
    .stat-value { font-size: 24px; font-weight: 800; margin-top: 4px; }
    .target-val { font-size: 11px; color: #adb5bd; margin-top: 6px; font-weight: 600; }
    .on { color: var(--on); }
    .off { color: var(--off); }
    .idle { color: var(--idle); }
    .badge { display: inline-block; padding: 6px 16px; border-radius: 20px; font-size: 12px; font-weight: 800; text-transform: uppercase; }
    .badge-incubation { background: #e7f3ff; color: var(--primary); }
    .badge-lockdown { background: #fff4e5; color: #d97706; }
    select, button { padding: 12px 20px; border-radius: 12px; border: 1px solid #e0e4e9; font-weight: 700; font-size: 14px; cursor: pointer; transition: all 0.2s; outline: none; }
    button { background: var(--primary); color: white; border: none; box-shadow: 0 4px 8px rgba(24, 119, 242, 0.25); }
    button:hover { background: var(--primary-dark); transform: scale(1.02); }
    .refresh-control { display: flex; align-items: center; gap: 10px; font-size: 13px; color: rgba(255,255,255,0.9); font-weight: 600; }
    .refresh-control select { padding: 4px 8px; border-radius: 8px; background: rgba(255,255,255,0.2); border: 1px solid rgba(255,255,255,0.3); color: white; font-size: 12px; }
    .refresh-control select option { color: black; }
    .chart-box { position: relative; margin-top: 10px; width: 100%; display: flex; flex-direction: column; }
    .canvas-container { width: 100%; position: relative; }
    #envChartContainer { height: 350px; }
    #actChartContainer { height: 180px; border-top: 1px dashed #eee; margin-top: 10px; padding-top: 10px; }
    .footer { text-align: center; font-size: 13px; color: var(--text-muted); margin-top: 40px; padding: 30px 0; border-top: 1px solid #e0e4e9; }
    .footer a { color: var(--primary); text-decoration: none; font-weight: 700; }
    h3 { margin: 0 0 20px 0; font-size: 18px; font-weight: 800; color: #333; display: flex; align-items: center; gap: 8px; }
  </style>
</head>
<body>
  <div class="container">
    <div class="header">
      <h1>EGGubator</h1>
      <div class="refresh-control">
        <span>REFRESH RATE</span>
        <select id="refreshRate" onchange="updateRefreshInterval()">
          <option value="1000">1s</option>
          <option value="2000">2s</option>
          <option value="5000" selected>5s</option>
          <option value="10000">10s</option>
          <option value="30000">30s</option>
        </select>
      </div>
    </div>

    <div class="card">
      <div style="display:flex; justify-content: space-between; align-items: center; margin-bottom: 24px;">
        <div id="stageBadge" class="badge">Loading...</div>
        <div style="font-size: 14px; color: var(--text-muted); font-weight: 600;">UPTIME: <span id="uptime" style="color:var(--text)">--</span></div>
      </div>
      <div class="grid">
        <div class="stat-card">
          <div class="stat-label">Temperature</div>
          <div class="stat-value" id="temp">--°C</div>
          <div class="target-val">TARGET <span id="targetTemp">--</span>°C</div>
        </div>
        <div class="stat-card">
          <div class="stat-label">Humidity</div>
          <div class="stat-value" id="hum">--%</div>
          <div class="target-val">TARGET <span id="targetHum">--</span>%</div>
        </div>
        <div class="stat-card">
          <div class="stat-label">Heater</div>
          <div class="stat-value" id="heaterStat">--</div>
        </div>
        <div class="stat-card">
          <div class="stat-label">Atomizer</div>
          <div class="stat-value" id="atomizerStat">--</div>
        </div>
        <div class="stat-card">
          <div class="stat-label">Fan</div>
          <div class="stat-value" id="fanStat">--</div>
        </div>
        <div class="stat-card">
          <div class="stat-label">Turner</div>
          <div class="stat-value" id="turnerStat">--</div>
        </div>
      </div>
    </div>

    <div class="card">
      <h3>Incubation Stage</h3>
      <div style="display:flex; gap:12px;">
        <select id="stageSelect" style="flex:1; background: #fcfdfe;">
          <option value="incubation">Incubation Stage (Days 1-18)</option>
          <option value="lockdown">Lockdown Stage (Days 19-21)</option>
        </select>
        <button onclick="setStage()">Update Device Stage</button>
      </div>
    </div>

    <div class="card">
      <h3>History & Device Activity</h3>
      <div class="chart-box">
        <div id="envChartContainer" class="canvas-container"><canvas id="envChart"></canvas></div>
        <div id="actChartContainer" class="canvas-container"><canvas id="actChart"></canvas></div>
      </div>
    </div>

    <div class="footer">
      <strong>EGGubator System</strong> &copy; 2025 | <a href="/mock">Device Status & Advanced Config &rarr;</a>
    </div>
  </div>

  <script>
    let envChart, actChart;
    let refreshTimer;

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
        entries.push({ t: ts, temp, hum, h: states & 1, a: (states >> 1) & 1, f: (states >> 2) & 1, s: ((states >> 3) & 3) === 2 ? -1 : ((states >> 3) & 3) });
      }
      return entries;
    }

    function initCharts() {
      const sync = (chart) => {
        const otherChart = chart === envChart ? actChart : envChart;
        if (!otherChart) return;
        otherChart.options.scales.x.min = chart.options.scales.x.min;
        otherChart.options.scales.x.max = chart.options.scales.x.max;
        otherChart.update('none');
      };

      const zoomOptions = {
        pan: { enabled: true, mode: 'xy' },
        zoom: { wheel: { enabled: true }, pinch: { enabled: true }, mode: 'xy' },
        onZoom: ({chart}) => sync(chart),
        onPan: ({chart}) => sync(chart)
      };

      const envCtx = document.getElementById('envChart').getContext('2d');
      envChart = new Chart(envCtx, {
        type: 'line',
        data: {
          datasets: [
            { label: 'Temp (°C)', borderColor: '#f02849', data: [], yAxisID: 'yTemp', pointRadius: 0, borderWidth: 3, tension: 0.35 },
            { label: 'Hum (%)', borderColor: '#1877f2', data: [], yAxisID: 'yHum', pointRadius: 0, borderWidth: 3, tension: 0.35 }
          ]
        },
        options: {
          responsive: true, maintainAspectRatio: false,
          interaction: { mode: 'index', intersect: false },
          scales: {
            x: { type: 'linear', title: { display: false }, grid: { color: '#f0f0f0' } },
            yTemp: { type: 'linear', position: 'left', title: { display: true, text: 'Temp °C', font: { weight: 'bold' } }, min: 30, max: 45 },
            yHum: { type: 'linear', position: 'right', title: { display: true, text: 'Hum %', font: { weight: 'bold' } }, min: 0, max: 100, grid: { display: false } }
          },
          plugins: {
            legend: { position: 'top', labels: { usePointStyle: true, boxWidth: 8, font: { weight: '700' } } },
            zoom: zoomOptions
          }
        }
      });

      const actCtx = document.getElementById('actChart').getContext('2d');
      actChart = new Chart(actCtx, {
        type: 'line',
        data: {
          datasets: [
            { label: 'Heater', borderColor: '#f02849', backgroundColor: 'rgba(240, 40, 73, 0.1)', data: [], stepped: true, fill: true, pointRadius: 0 },
            { label: 'Atomizer', borderColor: '#1877f2', backgroundColor: 'rgba(24, 119, 242, 0.1)', data: [], stepped: true, fill: true, pointRadius: 0 },
            { label: 'Fan', borderColor: '#42b72a', backgroundColor: 'rgba(66, 183, 42, 0.1)', data: [], stepped: true, fill: true, pointRadius: 0 },
            { label: 'Turner', borderColor: '#925e0d', data: [], stepped: true, pointRadius: 0, borderWidth: 2 }
          ]
        },
        options: {
          responsive: true, maintainAspectRatio: false,
          interaction: { mode: 'index', intersect: false },
          scales: {
            x: { type: 'linear', title: { display: true, text: 'Time (seconds ago)', font: { weight: 'bold' } }, grid: { color: '#f0f0f0' } },
            y: { min: -1.2, max: 1.2, ticks: { callback: v => v===1?'ON':(v===-1?'REV':'OFF') } }
          },
          plugins: {
            legend: { display: false },
            zoom: zoomOptions,
            tooltip: {
              callbacks: {
                label: function(context) {
                  let val = context.parsed.y;
                  let label = context.dataset.label + ': ';
                  if (context.datasetIndex === 3) { // Turner
                    return label + (val === 1 ? 'FORW' : (val === -1 ? 'REV' : 'IDLE'));
                  }
                  return label + (val === 1 ? 'ON' : 'OFF');
                }
              }
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
        
        const hStat = document.getElementById('heaterStat'); hStat.textContent = d.heater ? 'ON' : 'OFF'; hStat.className = 'stat-value ' + (d.heater ? 'on' : 'off');
        const aStat = document.getElementById('atomizerStat'); aStat.textContent = d.atomizer ? 'ON' : 'OFF'; aStat.className = 'stat-value ' + (d.atomizer ? 'on' : 'off');
        const fStat = document.getElementById('fanStat'); fStat.textContent = d.fan ? 'ON' : 'OFF'; fStat.className = 'stat-value ' + (d.fan ? 'on' : 'off');
        const tStat = document.getElementById('turnerStat'); tStat.textContent = d.servo === 0 ? 'IDLE' : (d.servo === 1 ? 'FORW' : 'REV'); tStat.className = 'stat-value ' + (d.servo !== 0 ? 'on' : 'idle');
        
        const b = document.getElementById('stageBadge'); b.textContent = d.stageLockdown ? 'Lockdown Stage' : 'Incubation Stage'; b.className = 'badge ' + (d.stageLockdown ? 'badge-lockdown' : 'badge-incubation');

        const logs = decodeLogs(d.logs, d.logCount);
        if (logs.length > 0) {
          const now = logs[logs.length-1].t;
          const xData = logs.map(l => (l.t - now)/1000);
          envChart.data.datasets[0].data = logs.map((l,i) => ({ x: xData[i], y: l.temp }));
          envChart.data.datasets[1].data = logs.map((l,i) => ({ x: xData[i], y: l.hum }));
          envChart.update('none');

          actChart.data.datasets[0].data = logs.map((l,i) => ({ x: xData[i], y: l.h }));
          actChart.data.datasets[1].data = logs.map((l,i) => ({ x: xData[i], y: l.a }));
          actChart.data.datasets[2].data = logs.map((l,i) => ({ x: xData[i], y: l.f }));
          actChart.data.datasets[3].data = logs.map((l,i) => ({ x: xData[i], y: l.s }));
          actChart.update('none');
        }
      });
    }

    function updateRefreshInterval() {
      const rate = parseInt(document.getElementById('refreshRate').value);
      clearInterval(refreshTimer);
      refreshTimer = setInterval(update, rate);
      fetch(`/mock/api?logInterval=${rate}`);
    }

    function setStage() {
      const s = document.getElementById('stageSelect').value;
      fetch(`/mock/api?stageType=${s}`).then(() => update());
    }

    initCharts();
    update();
    fetch('/mock/api').then(r => r.json()).then(d => {
       document.getElementById('refreshRate').value = d.logInterval;
       updateRefreshInterval();
    });
  </script>
</body>
</html>
)rawliteral";

const char MOCK_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>EGGubator - Advanced</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: 'Segoe UI', sans-serif; padding: 20px; background: #f0f2f5; color: #1c1e21; }
    .card { background: white; padding: 28px; border-radius: 20px; max-width: 540px; margin: 0 auto 24px; box-shadow: 0 4px 15px rgba(0,0,0,0.06); }
    h3 { margin-top: 0; color: #1877f2; font-weight: 800; border-bottom: 2px solid #f0f2f5; padding-bottom: 12px; margin-bottom: 24px; }
    .row { margin-bottom: 18px; display: flex; justify-content: space-between; align-items: center; }
    input, select { padding: 12px; border-radius: 10px; border: 1px solid #e0e4e9; width: 180px; font-weight: 600; }
    button { padding: 14px; width: 100%; cursor: pointer; background: #1877f2; color: white; border: none; border-radius: 10px; font-weight: 800; margin-top: 10px; transition: all 0.2s; }
    button:hover { background: #166fe5; }
    .sys-grid { display: grid; grid-template-columns: 1fr; }
    .sys-item { display: flex; justify-content: space-between; padding: 12px 0; border-bottom: 1px solid #f3f5f8; font-size: 14px; }
    .sys-item span:first-child { color: #65676b; font-weight: 700; text-transform: uppercase; font-size: 11px; letter-spacing: 0.5px; }
    .sys-item span:last-child { color: #1c1e21; font-weight: 700; font-family: monospace; font-size: 15px; }
    a { display: block; text-align: center; margin-top: 24px; color: #1877f2; text-decoration: none; font-weight: 800; }
    label { font-weight: 700; color: #444; font-size: 14px; }
    .danger { background: #f02849 !important; margin-top: 30px; }
    .danger:hover { background: #d02040 !important; }
    .mock-controls { display: none; margin-top: 15px; border-top: 1px dashed #ddd; padding-top: 15px; }
  </style>
</head>
<body>
  <div class="card">
    <h3>System Telemetry</h3>
    <div class="sys-grid">
      <div class="sys-item"><span>IP Address</span><span id="ip">--</span></div>
      <div class="sys-item"><span>WiFi RSSI</span><span id="rssi">-- dBm</span></div>
      <div class="sys-item"><span>Free RAM</span><span id="heap">-- KB</span></div>
      <div class="sys-item"><span>Log Buffer RAM</span><span>7.0 KB (fixed)</span></div>
      <div class="sys-item"><span>Active Logs</span><span id="logCount">--</span></div>
      <div class="sys-item"><span>Firmware</span><span id="version">--</span></div>
      <div class="sys-item"><span>Uptime</span><span id="uptimeSys">--</span></div>
    </div>
  </div>

  <div class="card">
    <h3>Sensor Simulation</h3>
    <div class="row">
      <label>Use Mock Sensor</label>
      <input type="checkbox" id="mockEnable" style="width:24px; height:24px;" onchange="toggleMock()">
    </div>
    <div class="row">
      <label>Physics Simulation</label>
      <input type="checkbox" id="autoSim" style="width:24px; height:24px;" onchange="toggleSim()">
    </div>
    
    <div id="mockControls" class="mock-controls">
      <div class="row">
        <label>Mock Temp (°C)</label>
        <input type="number" id="mTemp" step="0.1" value="37.5">
      </div>
      <div class="row">
        <label>Mock Hum (%)</label>
        <input type="number" id="mHum" step="0.1" value="60">
      </div>
      <button onclick="setMockValues()">Apply Simulation Values</button>
    </div>
  </div>

  <div class="card">
    <h3>Maintenance</h3>
    <div class="row">
      <label>Egg Turn Interval</label>
      <select id="turnInterval">
        <option value="1800000">30 Minutes</option>
        <option value="3600000">1 Hour</option>
        <option value="5400000">1.5 Hours</option>
        <option value="7200000">2 Hours</option>
        <option value="9000000">2.5 Hours</option>
        <option value="10800000">3 Hours</option>
        <option value="12600000">3.5 Hours</option>
        <option value="14400000">4 Hours</option>
      </select>
    </div>
    <button onclick="saveTurnInterval()">Update Turner Interval</button>
    <button class="danger" onclick="reboot()">Restart Controller</button>
  </div>
  
  <a href="/">&larr; Return to Dashboard</a>

  <script>
    function load() {
      fetch('/data').then(r => r.json()).then(d => {
        document.getElementById('ip').textContent = d.ip;
        document.getElementById('rssi').textContent = d.rssi;
        document.getElementById('heap').textContent = Math.round(d.heapFree/1024);
        document.getElementById('logCount').textContent = d.logCount;
        document.getElementById('version').textContent = d.version;
        document.getElementById('uptimeSys').textContent = d.uptime;
        
        document.getElementById('mockEnable').checked = d.mock === 1;
        document.getElementById('autoSim').checked = d.autosim === 1;
        document.getElementById('mTemp').value = d.temperature.toFixed(1);
        document.getElementById('mHum').value = d.humidity.toFixed(1);
        
        document.getElementById('mockControls').style.display = d.mock === 1 ? 'block' : 'none';
      });
      fetch('/mock/api').then(r => r.json()).then(d => {
        document.getElementById('turnInterval').value = d.eggTurnInterval;
      });
    }

    function toggleMock() {
      const enable = document.getElementById('mockEnable').checked;
      if (enable) document.getElementById('autoSim').checked = false;
      fetch(`/mock/api?enable=${enable ? 1 : 0}`).then(load);
    }

    function toggleSim() {
      const enable = document.getElementById('autoSim').checked;
      if (enable) document.getElementById('mockEnable').checked = false;
      fetch(`/mock/api?autosim=${enable ? 1 : 0}`).then(load);
    }

    function setMockValues() {
      const t = document.getElementById('mTemp').value;
      const h = document.getElementById('mHum').value;
      fetch(`/mock/api?temp=${t}&hum=${h}`).then(load);
    }

    function saveTurnInterval() {
      const val = document.getElementById('turnInterval').value;
      fetch(`/mock/api?eggTurnInterval=${val}`).then(load);
    }

    function reboot() { if(confirm('Reboot device?')) fetch('/reboot'); }
    load();
    setInterval(load, 5000);
  </script>
</body>
</html>
)rawliteral";

#endif
