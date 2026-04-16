// ============================================
// EGG INCUBATOR CONTROLLER - Modular Version
// ============================================

// Include header files
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266HTTPClient.h>
#include <Servo.h>
#include <EEPROM.h>

#include "config.h"
#include "dht_sensor.h"
#include "wifi_manager.h"
#include "logging.h"
#include "updates.h"

extern bool useMockSensor;
extern float mockTemp;
extern float mockHum;
extern bool autoMode;

// Global variables
float currentTemp = 0;
float currentHumidity = 0;
bool heaterState = false;
bool atomizerState = false;
bool fanState = false;
bool servoEnabled = false;
bool autoMode = true;
unsigned long manualModeStart = 0;
String manualModeReason = "";
unsigned long lastReadTime = 0;
unsigned long lastOtaCheck = 0;
unsigned long lastServoTurn = 0;

// CPU monitoring
unsigned long lastCpuCheck = 0;
unsigned long cpuCyclesStart = 0;
unsigned long cpuCyclesEnd = 0;
unsigned long cpuUtil = 0;

// Control state variables
unsigned long atomizerPulseStart = 0;
bool atomizerPulsing = false;
bool atomizerInOffPhase = false;
unsigned long atomizerOffStart = 0;
unsigned long heaterLastChanged = 0;
bool heaterWasOn = false;
unsigned long atomizerLastChanged = 0;
bool atomizerWasOn = false;

// Log buffer (defined in logging.h)
LogEntry logBuffer[MAX_LOG_ENTRIES];
int logIndex = 0;
bool logFull = false;
unsigned long lastLogTime = 0;

// Web server
ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;
Servo eggServo;

// ============================================
// WEB INTERFACE HTML
// ============================================
const char INDEX_HTML[] PROGMEM = R"rawliteral(
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
    .header { display: flex; justify-content: center; align-items: center; gap: 15px; background: rgba(255,255,255,0.25); padding: 12px 20px; border-radius: 12px; margin-bottom: 15px; flex-wrap: wrap; }
    .mode-group { display: flex; align-items: center; gap: 8px; background: white; padding: 6px 12px; border-radius: 20px; }
    .mode-group label { font-weight: 600; color: #333; font-size: 13px; }
    .switch { position: relative; display: inline-block; width: 44px; height: 24px; }
    .switch input { opacity: 0; width: 0; height: 0; }
    .slider { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: #bbb; transition: .3s; border-radius: 24px; }
    .slider:before { position: absolute; content: ""; height: 18px; width: 18px; left: 3px; bottom: 3px; background: white; transition: .3s; border-radius: 50%; }
    input:checked + .slider { background-color: #2196F3; }
    input:checked + .slider:before { transform: translateX(20px); }
    .mode-text { font-size: 12px; color: #666; font-weight: 600; }
    .mode-text.active { color: #2196F3; }
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
    .btn:disabled:hover { opacity: 0.5; }
    .btn-on { background: #4CAF50; color: white; }
    .btn-off { background: #f44336; color: white; }
    .btn-auto { background: #2196F3; color: white; width: 100%; }
    .target-info { text-align: center; color: #555; font-size: 13px; }
    .chart-wrap { overflow-x: auto; -webkit-overflow-scrolling: touch; }
    canvas { display: block; background: #fafafa; border-radius: 8px; touch-action: pan-x pinch-x; }
    .legend { display: flex; flex-wrap: wrap; justify-content: center; gap: 12px; margin-top: 8px; font-size: 11px; }
    .legend-item { display: flex; align-items: center; gap: 4px; }
    .legend-dot { width: 10px; height: 10px; border-radius: 50%; }
    .time-label { text-align: center; font-size: 10px; color: #999; margin-top: 4px; }
    .alert { background: #ff9800; color: white; padding: 10px; border-radius: 8px; margin-bottom: 15px; display: none; text-align: center; }
    .alert.show { display: block; animation: fadeIn 0.3s; }
    @keyframes fadeIn { from { opacity: 0; transform: translateY(-10px); } to { opacity: 1; transform: translateY(0); } }
    .version { text-align: center; color: rgba(255,255,255,0.8); font-size: 11px; margin-top: 10px; }
    .uptime-tag { background: rgba(255,255,255,0.2); padding: 4px 10px; border-radius: 12px; color: white; font-size: 12px; }
    @media (max-width: 400px) { .device-grid { grid-template-columns: 1fr; } .info-grid { grid-template-columns: 1fr; } }
  </style>
</head>
<body>
  <div class="container">
    <h1><span>🥚</span> EGGubator</h1>
    <div class="header">
      <div class="mode-group">
        <span class="mode-text" id="modeText">AUTO</span>
        <label class="switch">
          <input type="checkbox" id="modeSwitch" onchange="toggleMode()">
          <span class="slider"></span>
        </label>
      </div>
      <div class="uptime-tag" id="uptime">--</div>
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
      <div class="target-info" id="targets">Target: 37.5°C | 60%</div>
    </div>
    <div class="card">
      <button class="btn btn-auto" onclick="checkOta()">Check Update</button>
      <div style="text-align:center;margin-top:8px;font-size:12px;color:#666;" id="otaStatus"></div>
    </div>
    <div class="card">
      <div class="label">Temperature</div>
      <div class="chart-wrap"><canvas id="tempChart" width="400" height="120"></canvas></div>
      <div class="time-label" id="tempTime">--</div>
    </div>
    <div class="card">
      <div class="label">Humidity</div>
      <div class="chart-wrap"><canvas id="humChart" width="400" height="120"></canvas></div>
      <div class="time-label" id="humTime">--</div>
    </div>
    <div class="card">
      <div class="label">Controls</div>
      <div class="chart-wrap"><canvas id="ctrlChart" width="400" height="120"></canvas></div>
      <div class="time-label" id="ctrlTime">--</div>
      <div class="legend" id="ctrlLegend"></div>
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
    let devicesInited = false;
    function initDevices() {
      if (devicesInited) return;
      devicesInited = true;
      const grid = document.getElementById('deviceGrid');
      devices.forEach(d => {
        grid.innerHTML += '<div class="device-card"><div class="device-name">'+d.name+'</div><div class="device-status" id="'+d.id+'">OFF</div><div class="btn-group"><button class="btn btn-on" id="'+d.id+'On" onclick="toggleDevice(\''+d.id+'\',\'on\')">ON</button><button class="btn btn-off" id="'+d.id+'Off" onclick="toggleDevice(\''+d.id+'\',\'off\')">OFF</button></div></div>';
      });
    }
    initDevices();
    let lastLogLen = 0;
    function updateData() {
      fetch('/data').then(r => r.json()).then(d => {
        document.getElementById('version').textContent = d.version;
        document.getElementById('temp').textContent = d.temperature.toFixed(1)+'°C';
        document.getElementById('hum').textContent = d.humidity.toFixed(1)+'%';
        document.getElementById('uptime').textContent = d.uptime;
        devices.forEach(dev => {
          const state = d[dev.id];
          const el = document.getElementById(dev.id);
          const onBtn = document.getElementById(dev.id+'On');
          const offBtn = document.getElementById(dev.id+'Off');
          el.textContent = state ? 'ON' : 'OFF';
          el.className = 'device-status ' + (state ? 'on' : 'off');
          onBtn.disabled = d.autoMode;
          offBtn.disabled = d.autoMode;
        });
        const modeText = document.getElementById('modeText');
        modeText.textContent = d.autoMode ? 'AUTO' : 'MANUAL';
        modeText.className = 'mode-text ' + (d.autoMode ? 'active' : '');
        document.getElementById('modeSwitch').checked = !d.autoMode;
        if (d.reason) { document.getElementById('alertBox').textContent = d.reason; document.getElementById('alertBox').classList.add('show'); setTimeout(() => document.getElementById('alertBox').classList.remove('show'), 5000); }
        if (d.log && d.log.length > 0) { console.log('Log entries:', d.log.length); drawTempChart(d.log); drawHumChart(d.log); drawCtrlChart(d.log); }
      }).catch(e => console.error('Data fetch error:', e));
    }
    function formatTime(ms) {
      if (ms < 60000) return (ms/1000).toFixed(0)+'s';
      if (ms < 3600000) return (ms/60000).toFixed(0)+'m';
      if (ms < 86400000) return (ms/3600000).toFixed(0)+'h';
      return (ms/86400000).toFixed(0)+'d';
    }
    function drawTempChart(logData) {
      const c = document.getElementById('tempChart');
      const ctx = c.getContext('2d');
      const w = c.width = c.offsetWidth;
      const h = c.height = 120;
      ctx.clearRect(0, 0, w, h);
      if (!logData || logData.length < 1) return;
      
      let minT = 50, maxT = 0;
      logData.forEach(p => { if (p.temp > maxT) maxT = p.temp; if (p.temp < minT) minT = p.temp; });
      if (maxT - minT < 2) { minT = 35; maxT = 40; }
      
      const times = logData.map(p => p.t);
      const minTime = times[0], maxTime = times[times.length-1], timeRange = maxTime - minTime || 1;
      
      ctx.beginPath(); ctx.strokeStyle = '#ff5722'; ctx.lineWidth = 2;
      logData.forEach((p, i) => {
        const x = (i / (logData.length - 1)) * w;
        const y = h - ((p.temp - minT) / (maxT - minT)) * h;
        i === 0 ? ctx.moveTo(x, y) : ctx.lineTo(x, y);
      });
      ctx.stroke();
      
      const targetY = h - ((37.5 - minT) / (maxT - minT)) * h;
      ctx.beginPath(); ctx.strokeStyle = '#4CAF50'; ctx.setLineDash([4, 4]); ctx.moveTo(0, targetY); ctx.lineTo(w, targetY); ctx.stroke(); ctx.setLineDash([]);
      
      document.getElementById('tempTime').textContent = formatTime(timeRange);
    }
    function drawHumChart(logData) {
      const c = document.getElementById('humChart');
      const ctx = c.getContext('2d');
      const w = c.width = c.offsetWidth;
      const h = c.height = 120;
      ctx.clearRect(0, 0, w, h);
      if (!logData || logData.length < 1) return;
      
      let minH = 100, maxH = 0;
      logData.forEach(p => { if (p.hum > maxH) maxH = p.hum; if (p.hum < minH) minH = p.hum; });
      if (maxH - minH < 5) { minH = 40; maxH = 80; }
      
      const times = logData.map(p => p.t);
      const minTime = times[0], maxTime = times[times.length-1], timeRange = maxTime - minTime || 1;
      
      ctx.beginPath(); ctx.strokeStyle = '#2196F3'; ctx.lineWidth = 2;
      logData.forEach((p, i) => {
        const x = (i / (logData.length - 1)) * w;
        const y = h - ((p.hum - minH) / (maxH - minH)) * h;
        i === 0 ? ctx.moveTo(x, y) : ctx.lineTo(x, y);
      });
      ctx.stroke();
      
      const targetY = h - ((60 - minH) / (maxH - minH)) * h;
      ctx.beginPath(); ctx.strokeStyle = '#4CAF50'; ctx.setLineDash([4, 4]); ctx.moveTo(0, targetY); ctx.lineTo(w, targetY); ctx.stroke(); ctx.setLineDash([]);
      
      document.getElementById('humTime').textContent = formatTime(timeRange);
    }
    const ctrlDevices = [
      { key: 'h', name: 'Heater', color: '#f44336' },
      { key: 'a', name: 'Atomizer', color: '#2196F3' },
      { key: 'f', name: 'Fan', color: '#4CAF50' },
      { key: 's', name: 'Turner', color: '#FF9800' }
    ];
    function drawCtrlChart(logData) {
      const c = document.getElementById('ctrlChart');
      const ctx = c.getContext('2d');
      const w = c.width = c.offsetWidth;
      const h = c.height = 120;
      ctx.clearRect(0, 0, w, h);
      if (!logData || logData.length < 1) return;
      
      const times = logData.map(p => p.t);
      const minTime = times[0], maxTime = times[times.length-1], timeRange = maxTime - minTime || 1;
      
      const legend = document.getElementById('ctrlLegend');
      legend.innerHTML = ctrlDevices.map(d => '<div class="legend-item"><div class="legend-dot" style="background:'+d.color+'"></div><span>'+d.name+'</span></div>').join('');
      
      const rowH = h / 4;
      ctrlDevices.forEach((dev, idx) => {
        ctx.beginPath(); ctx.strokeStyle = dev.color; ctx.lineWidth = 2;
        let started = false;
        logData.forEach((p, i) => {
          const x = (i / (logData.length - 1)) * w;
          const val = p[dev.key] ? 1 : 0;
          const y = h - (idx + 0.5) * rowH - (val * rowH * 0.35);
          started ? ctx.lineTo(x, y) : ctx.moveTo(x, y);
          started = true;
        });
        ctx.stroke();
      });
      document.getElementById('ctrlTime').textContent = formatTime(timeRange);
    }
    function toggleDevice(device, state) {
      const onBtn = document.getElementById(device+'On');
      if (onBtn.disabled) return;
      fetch('/control?device='+device+'&state='+state).then(() => updateData()).catch(e => console.error(e));
    }
    function checkOta() {
      document.getElementById('otaStatus').textContent = 'Checking...';
      fetch('/ota/check').then(r => r.json()).then(d => {
        document.getElementById('otaStatus').textContent = d.update ? 'Update: '+d.version : 'Up to date';
        if (d.update) fetch('/ota/update').then(r => r.json()).then(r => document.getElementById('otaStatus').textContent = r.status);
      }).catch(e => document.getElementById('otaStatus').textContent = 'Error');
    }
    function toggleMode() {
      const isManual = document.getElementById('modeSwitch').checked;
      fetch('/control?mode='+(isManual?'manual':'auto')).then(() => updateData()).catch(e => console.error(e));
    }
    window.addEventListener('resize', () => updateData());
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
  <title>Mock - EGGubator</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    * { box-sizing: border-box; }
    body { font-family: 'Segoe UI', Arial, sans-serif; margin: 0; padding: 0; background: linear-gradient(135deg, #9C27B0 0%, #673AB7 100%); min-height: 100vh; }
    .container { max-width: 500px; margin: 0 auto; padding: 15px; }
    .card { background: white; padding: 15px; border-radius: 12px; box-shadow: 0 4px 15px rgba(0,0,0,0.2); margin-bottom: 15px; }
    h1 { color: white; text-align: center; font-size: 24px; margin: 10px 0 15px 0; }
    h1 span { color: #ffd700; }
    .header { display: flex; justify-content: space-between; align-items: center; background: rgba(255,255,255,0.25); padding: 12px 20px; border-radius: 12px; margin-bottom: 15px; }
    .mode-group { display: flex; align-items: center; gap: 8px; background: white; padding: 6px 12px; border-radius: 20px; }
    .mode-group label { font-weight: 600; color: #333; font-size: 13px; }
    .switch { position: relative; display: inline-block; width: 44px; height: 24px; }
    .switch input { opacity: 0; width: 0; height: 0; }
    .slider { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: #bbb; transition: .3s; border-radius: 24px; }
    .slider:before { position: absolute; content: ""; height: 18px; width: 18px; left: 3px; bottom: 3px; background: white; transition: .3s; border-radius: 50%; }
    input:checked + .slider { background-color: #9C27B0; }
    input:checked + .slider:before { transform: translateX(20px); }
    .mode-text { font-size: 12px; color: #666; font-weight: 600; }
    .mode-text.active { color: #9C27B0; }
    .label { color: #777; font-size: 12px; margin-bottom: 8px; display: block; }
    .input-row { display: flex; align-items: center; gap: 10px; margin-bottom: 12px; }
    .input-row label { margin: 0; min-width: 45px; }
    input[type="number"] { padding: 8px 10px; border: 1px solid #ddd; border-radius: 6px; width: 90px; font-size: 14px; }
    .btn { padding: 10px 20px; margin: 5px 5px 0 0; border: none; border-radius: 6px; cursor: pointer; font-size: 14px; font-weight: 500; }
    .btn-auto { background: #9C27B0; color: white; width: 100%; }
    .btn-back { background: white; color: #333; }
    .status-row { display: flex; justify-content: space-between; align-items: center; padding: 10px 0; border-bottom: 1px solid #eee; }
    .status-row:last-child { border-bottom: none; }
    .stat { font-size: 20px; font-weight: bold; }
    .on { color: #4CAF50; }
    .off { color: #f44336; }
    .msg { text-align: center; font-size: 13px; color: #666; margin-top: 8px; min-height: 20px; }
    .back-link { color: white; text-decoration: none; font-size: 14px; }
    .sys-row { display: flex; justify-content: space-between; padding: 8px 0; border-bottom: 1px solid #eee; }
    .sys-row:last-child { border-bottom: none; }
    .sys-label { color: #666; font-size: 12px; }
    .sys-val { font-weight: 600; color: #333; font-size: 13px; }
  </style>
</head>
<body>
  <div class="container">
    <h1><span>🥚</span> Mock Settings</h1>
    <div class="header">
      <a href="/" class="back-link">← Back</a>
      <div class="mode-group">
        <span class="mode-text" id="mockText">OFF</span>
        <label class="switch">
          <input type="checkbox" id="mockSwitch" onchange="toggleMock()">
          <span class="slider"></span>
        </label>
      </div>
    </div>
    <div class="card">
      <span class="label">Mock Values</span>
      <div class="input-row">
        <label>Temp:</label>
        <input type="number" id="mockTemp" value="25" step="0.1" min="20" max="45">
        <span>°C</span>
      </div>
      <div class="input-row">
        <label>Hum:</label>
        <input type="number" id="mockHum" value="50" step="0.1" min="0" max="100">
        <span>%</span>
      </div>
      <button class="btn btn-auto" id="setValuesBtn" onclick="setMockValues()">Set Values</button>
      <div class="msg" id="mockStatus"></div>
    </div>
    <div class="card">
      <span class="label">Current Status</span>
      <div class="status-row"><span>Sensor</span><span class="stat" id="sensorStatus">Real</span></div>
      <div class="status-row"><span>Temperature</span><span class="stat" id="currentTemp">--°C</span></div>
      <div class="status-row"><span>Humidity</span><span class="stat" id="currentHum">--%</span></div>
    </div>
    <div class="card">
      <span class="label">System Info</span>
      <div class="sys-row"><span class="sys-label">Free RAM</span><span class="sys-val" id="sysHeap">--</span></div>
      <div class="sys-row"><span class="sys-label">CPU</span><span class="sys-val" id="sysCpu">--</span></div>
      <div class="sys-row"><span class="sys-label">Flash</span><span class="sys-val" id="sysFlash">--</span></div>
      <div class="sys-row"><span class="sys-label">Log Entries</span><span class="sys-val" id="sysLog">--</span></div>
      <div class="sys-row"><span class="sys-label">Uptime</span><span class="sys-val" id="sysUptime">--</span></div>
    </div>
  </div>
  <script>
    let userEditing = false;
    function updateData() {
      fetch('/data').then(r => r.json()).then(d => {
        const mockOn = d.mock;
        document.getElementById('mockText').textContent = mockOn ? 'ON' : 'OFF';
        document.getElementById('mockText').className = 'mode-text ' + (mockOn ? 'active' : '');
        document.getElementById('mockSwitch').checked = mockOn;
        document.getElementById('currentTemp').textContent = d.temperature.toFixed(1)+'°C';
        document.getElementById('currentHum').textContent = d.humidity.toFixed(1)+'%';
        document.getElementById('sensorStatus').textContent = mockOn ? 'Mock' : 'Real';
        document.getElementById('sensorStatus').className = 'stat ' + (mockOn ? 'on' : 'off');
        document.getElementById('setValuesBtn').disabled = !mockOn;
        if (d.sys) {
          const heapFree = d.sys.heapFree || 0;
          const heapTotal = d.sys.heapTotal || 81920;
          const flashSize = d.sys.flashSize || 0;
          const flashTotal = d.sys.flashTotal || 4194304;
          document.getElementById('sysHeap').textContent = heapFree + ' / ' + heapTotal + ' KB';
          document.getElementById('sysCpu').textContent = d.sys.cpu + '%';
          document.getElementById('sysFlash').textContent = (flashSize/1048576).toFixed(1) + ' / ' + (flashTotal/1048576) + ' MB';
          document.getElementById('sysLog').textContent = d.sys.logCnt + ' / 100';
          document.getElementById('sysUptime').textContent = d.uptime;
        }
      }).catch(e => console.error(e));
    }
    function loadMockValues() {
      fetch('/mock/api').then(r => r.json()).then(d => {
        document.getElementById('mockTemp').value = d.temp;
        document.getElementById('mockHum').value = d.hum;
      }).catch(e => console.error(e));
    }
    function onInputFocus() { userEditing = true; }
    function onInputBlur() { userEditing = false; }
    function toggleMock() {
      const enable = document.getElementById('mockSwitch').checked;
      fetch('/mock/api?enable=' + (enable ? '1' : '0')).then(r => r.text()).then(() => { userEditing = false; updateData(); loadMockValues(); }).catch(e => console.error(e));
    }
    function setMockValues() {
      const t = document.getElementById('mockTemp').value;
      const h = document.getElementById('mockHum').value;
      if (t < 20 || t > 45 || h < 0 || h > 100) { document.getElementById('mockStatus').textContent = 'Invalid: Temp 20-45°C, Hum 0-100%'; return; }
      fetch('/mock/api?temp=' + t + '&hum=' + h).then(r => r.text()).then(msg => { document.getElementById('mockStatus').textContent = msg; updateData(); }).catch(e => console.error(e));
    }
    document.getElementById('mockTemp').addEventListener('focus', onInputFocus);
    document.getElementById('mockHum').addEventListener('focus', onInputFocus);
    document.getElementById('mockTemp').addEventListener('blur', onInputBlur);
    document.getElementById('mockHum').addEventListener('blur', onInputBlur);
    setInterval(updateData, 2000);
    loadMockValues();
    updateData();
  </script>
</body>
</html>
)rawliteral";

// ============================================
// WEB SERVER HANDLERS
// ============================================
void handleRoot() {
  server.send(200, "text/html; charset=utf-8", INDEX_HTML);
}

void handleMockPage() {
  server.send(200, "text/html; charset=utf-8", MOCK_HTML);
}

void handleData() {
  unsigned long uptimeSec = millis() / 1000;
  int days = uptimeSec / 86400;
  int hours = (uptimeSec % 86400) / 3600;
  int mins = (uptimeSec % 3600) / 60;
  int secs = uptimeSec % 60;
  String uptimeStr = "";
  if (days > 0) uptimeStr += String(days) + "d ";
  uptimeStr += String(hours) + "h " + String(mins) + "m " + String(secs) + "s";
  
  String json = "{\"temperature\":" + String(currentTemp) +
               ",\"humidity\":" + String(currentHumidity) +
               ",\"heater\":" + String(heaterState ? "true" : "false") +
               ",\"atomizer\":" + String(atomizerState ? "true" : "false") +
               ",\"fan\":" + String(fanState ? "true" : "false") +
               ",\"servo\":" + String(servoEnabled ? "true" : "false") +
               ",\"version\":\"" + FIRMWARE_VERSION + "\"" +
               ",\"uptime\":\"" + uptimeStr + "\"" +
               ",\"mock\":" + String(useMockSensor ? "true" : "false") +
               ",\"autoMode\":" + String(autoMode ? "true" : "false") +
               ",\"reason\":\"" + manualModeReason + "\"" +
               ",\"sys\":{\"heapFree\":" + String(ESP.getFreeHeap()) +
               ",\"heapTotal\":81920" +
               ",\"cpu\":" + String(cpuUtil) +
               ",\"flashSize\":" + String(ESP.getFlashChipSize()) +
               ",\"flashTotal\":4194304" +
               ",\"logCnt\":" + String(logIndex) + "}";
  getLogDataForWeb(json);
  json += "}";
  server.send(200, "application/json", json);
}

void handleControl() {
  if (server.hasArg("mode")) {
    autoMode = (server.arg("mode") == "auto");
    manualModeStart = autoMode ? 0 : millis();
    manualModeReason = "";
    server.send(200, "text/plain", autoMode ? "Auto mode enabled" : "Manual mode enabled");
  } else if (server.hasArg("device") && server.hasArg("state")) {
    String device = server.arg("device");
    String state = server.arg("state");
    bool isOn = (state == "on");
    
    if (device == "heater") {
      heaterState = isOn;
      digitalWrite(RELAY_HEATER, heaterState ? HIGH : LOW);
    } else if (device == "atomizer") {
      atomizerState = isOn;
      digitalWrite(RELAY_ATOMIZER, atomizerState ? HIGH : LOW);
    } else if (device == "fan") {
      fanState = isOn;
      digitalWrite(RELAY_FAN, fanState ? HIGH : LOW);
    } else if (device == "servo") {
      servoEnabled = isOn;
    }
  }
  server.send(200, "text/plain", "OK");
}

void handleOtaCheck() {
  HTTPClient http;
  WiFiClient client;
  http.begin(client, VERSION_URL);
  int httpCode = http.GET();
  
  if (httpCode == 200) {
    String remoteVersion = http.getString();
    remoteVersion.trim();
    bool hasUpdate = (remoteVersion != FIRMWARE_VERSION);
    String json = "{\"update\":" + String(hasUpdate ? "true" : "false") + ",\"version\":\"" + remoteVersion + "\"}";
    server.send(200, "application/json", json);
  } else {
    server.send(200, "application/json", "{\"update\":false,\"version\":\"error\"}");
  }
  http.end();
}

void handleOtaUpdate() {
  performUpdate();
}

void handleMockSensor() {
  if (server.hasArg("enable")) {
    bool enable = (server.arg("enable") == "1");
    setMockSensor(enable);
    server.send(200, "text/plain", enable ? "Mock sensor enabled" : "Mock sensor disabled");
  } else if (server.hasArg("temp") && server.hasArg("hum")) {
    float t = server.arg("temp").toFloat();
    float h = server.arg("hum").toFloat();
    setMockValues(t, h);
    server.send(200, "text/plain", "Mock values set: " + String(t) + "C, " + String(h) + "%");
  } else {
    String json = "{\"enabled\":" + String(useMockSensor ? "true" : "false") + 
                  ",\"temp\":" + String(mockTemp) + 
                  ",\"hum\":" + String(mockHum) + "}";
    server.send(200, "application/json", json);
  }
}

// ============================================
// AUTO CONTROL LOGIC
// ============================================
void checkManualModeSafety() {
  if (!autoMode && !isnan(currentTemp) && !isnan(currentHumidity)) {
    if (currentTemp < TARGET_TEMP - TEMP_HYSTERESIS - 1.0 || 
        currentTemp > TARGET_TEMP + TEMP_HYSTERESIS + 1.0 ||
        currentHumidity < TARGET_HUMIDITY - HUMIDITY_HYSTERESIS - 5.0 ||
        currentHumidity > TARGET_HUMIDITY + HUMIDITY_HYSTERESIS + 5.0) {
      autoMode = true;
      manualModeReason = "Values out of range - switched to AUTO";
    }
  }
}

void autoControl() {
  if (!autoMode) {
    checkManualModeSafety();
    return;
  }
  if (!isnan(currentTemp) && !isnan(currentHumidity)) {
    unsigned long now = millis();
    
    // Temperature control
    if (currentTemp < TARGET_TEMP - TEMP_HYSTERESIS) {
      heaterState = true;
    } else if (currentTemp > TARGET_TEMP + TEMP_HYSTERESIS) {
      heaterState = false;
    }
    digitalWrite(RELAY_HEATER, heaterState ? HIGH : LOW);
    
    if (heaterState != heaterWasOn) {
      heaterLastChanged = now;
      heaterWasOn = heaterState;
    }
    
    // Pulsating humidity control (3s ON, 10s OFF)
    if (currentHumidity < TARGET_HUMIDITY) {
      if (!atomizerPulsing && !atomizerInOffPhase) {
        atomizerState = true;
        digitalWrite(RELAY_ATOMIZER, HIGH);
        atomizerPulseStart = millis();
        atomizerPulsing = true;
      } else if (atomizerPulsing && (millis() - atomizerPulseStart >= PULSE_ON_TIME)) {
        atomizerState = false;
        digitalWrite(RELAY_ATOMIZER, LOW);
        atomizerPulsing = false;
        atomizerInOffPhase = true;
        atomizerOffStart = millis();
      } else if (atomizerInOffPhase && (millis() - atomizerOffStart >= PULSE_OFF_TIME)) {
        atomizerInOffPhase = false;
      }
    } else {
      if (atomizerState) {
        atomizerState = false;
        digitalWrite(RELAY_ATOMIZER, LOW);
        atomizerPulsing = false;
        atomizerInOffPhase = false;
      }
    }
    
    if (atomizerState != atomizerWasOn) {
      atomizerLastChanged = now;
      atomizerWasOn = atomizerState;
    }
    
    // Determine stability
    bool tempStable = (currentTemp >= TARGET_TEMP - TEMP_HYSTERESIS && 
                     currentTemp <= TARGET_TEMP + TEMP_HYSTERESIS);
    bool humStable = (currentHumidity >= TARGET_HUMIDITY - HUMIDITY_HYSTERESIS && 
                    currentHumidity <= TARGET_HUMIDITY + HUMIDITY_HYSTERESIS);
    
    // Fan control
    bool withinHeaterWindow = (!heaterState && (now - heaterLastChanged < FAN_EXTEND_TIME));
    bool withinAtomizerWindow = (!atomizerState && (now - atomizerLastChanged < FAN_EXTEND_TIME));
    
    if (heaterState || withinHeaterWindow || atomizerState || withinAtomizerWindow || 
        currentTemp > MAX_SAFE_TEMP) {
      fanState = true;
    } else if (tempStable && humStable) {
      fanState = false;
    }
    digitalWrite(RELAY_FAN, fanState ? HIGH : LOW);
  }
}

// ============================================
// EGG TURNER
// ============================================
void rotateEggs() {
  static int servoPos = 0;
  static bool sweeping = false;
  
  if (servoEnabled && (millis() - lastServoTurn > 7200000)) {
    if (!sweeping) {
      for (servoPos = 0; servoPos <= 180; servoPos += 10) {
        eggServo.write(servoPos);
        delay(50);
      }
      sweeping = true;
    } else {
      for (servoPos = 180; servoPos >= 0; servoPos -= 10) {
        eggServo.write(servoPos);
        delay(50);
      }
      sweeping = false;
    }
    lastServoTurn = millis();
    Serial.println("Eggs rotated");
  }
}

// Rollback endpoints
void handleReboot() {
  server.send(200, "text/plain", "Rebooting...");
  delay(500);
  ESP.restart();
}

void handleRollback() {
  // Trigger rollback - restore previous firmware if available
  Serial.println("Rollback triggered - resetting boot counter");
  EEPROM.write(EEPROM_BOOT_COUNT, MAX_BOOT_FAILURES);
  EEPROM.commit();
  server.send(200, "text/plain", "Rollback: please reflash to recover");
}

void handleRecovery() {
  // Enter recovery mode - reset EEPROM and await reflash
  EEPROM.write(EEPROM_BOOT_OK, 0);
  EEPROM.write(EEPROM_BOOT_COUNT, 0);
  EEPROM.commit();
  server.send(200, "text/plain", "Recovery mode - please reflash");
}

void handleRecoveryReset() {
  // Reset recovery mode - allow normal boot
  EEPROM.write(EEPROM_BOOT_OK, BOOT_OK_MAGIC);
  EEPROM.write(EEPROM_BOOT_COUNT, 0);
  EEPROM.commit();
  server.send(200, "text/plain", "Recovery reset - normal boot enabled");
  delay(500);
  ESP.restart();
}

// ============================================
// LOGS PAGE HANDLER
// ============================================
void handleLogsPage() {
  server.send(200, "text/html; charset=utf-8", LOGS_HTML);
}

// ============================================
// MAIN SETUP
// ============================================
void setup() {
  Serial.begin(115200);
  
  pinMode(RELAY_HEATER, OUTPUT);
  pinMode(RELAY_ATOMIZER, OUTPUT);
  pinMode(RELAY_FAN, OUTPUT);
  digitalWrite(RELAY_HEATER, LOW);
  digitalWrite(RELAY_ATOMIZER, LOW);
  digitalWrite(RELAY_FAN, LOW);

  eggServo.attach(SERVO_PIN);

  initLogging();
  initRecovery();
  connectWiFi();
  markBootSuccess();

  Serial.print("Firmware: ");
  Serial.println(FIRMWARE_VERSION);

  // Setup web server
  server.on("/", handleRoot);
  server.on("/mock", handleMockPage);
  server.on("/logs", handleLogsPage);
  server.on("/data", handleData);
  server.on("/control", handleControl);
  server.on("/ota/check", handleOtaCheck);
  server.on("/ota/update", handleOtaUpdate);
  server.on("/mock/api", handleMockSensor);
  server.on("/reboot", handleReboot);
  server.on("/rollback", handleRollback);
  server.on("/recovery", handleRecovery);
  server.on("/recovery/reset", handleRecoveryReset);
  httpUpdater.setup(&server);
  server.begin();

  Serial.println("HTTP server started");
}

// ============================================
// MAIN LOOP
// ============================================
void loop() {
  server.handleClient();

  // CPU monitoring - measure cycles every second
  if (millis() - lastCpuCheck > 1000) {
    cpuCyclesEnd = ESP.getCycleCount();
    if (cpuCyclesStart > 0) {
      unsigned long cycles = cpuCyclesEnd - cpuCyclesStart;
      cpuUtil = (cycles / 8000); // 80MHz / 10000 = rough %, adjusted
      if (cpuUtil > 100) cpuUtil = 100;
    }
    cpuCyclesStart = cpuCyclesEnd;
    lastCpuCheck = millis();
  }

  if (millis() - lastReadTime > 2000) {
    float t = readDHT22();
    float h = readHumidity();

    if (t > 0 && h > 0) {
      currentTemp = t;
      currentHumidity = h;
      autoControl();
    }
    lastReadTime = millis();
  }

  if (millis() - lastLogTime >= LOG_INTERVAL) {
    logData(currentTemp, currentHumidity, heaterState, atomizerState, fanState, servoEnabled);
    lastLogTime = millis();
  }

  if (servoEnabled) {
    rotateEggs();
  }

  if (millis() - lastOtaCheck > 3600000) {
    checkAndUpdateAuto();
    lastOtaCheck = millis();
  }
}
