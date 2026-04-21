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
extern bool autoSimMode;
extern float mockTemp;
extern float mockHum;
extern float simTemp;
extern float simHum;
extern void updateAutoSim(bool heater, bool atomizer, bool fan);

#define KILL_OFF 0
#define AUTO 1

// Configurable timing (can be changed via web)
unsigned long LOG_INTERVAL = 10000;
unsigned long SAVE_FLASH_INTERVAL = 7200000;
unsigned long EGG_TURN_INTERVAL = 7200000;
unsigned long PULSE_ON_TIME = 2000;  // Atomizer pulse ON time (2 sec default)

// Target temperature and humidity (can be changed via web/stage selection)
float TARGET_TEMP = 37.5;    // Default 37.5°C
float TARGET_HUMIDITY = 60.0; // Default 60.0%

// Global variables
float currentTemp = 0;
float currentHumidity = 0;
bool heaterState = false;
bool atomizerState = false;
bool fanState = false;
bool servoEnabled = false;
int servoPosition = 0; // -1 = -45deg, 0 = center, 1 = +45deg
int heaterMode = AUTO;
bool stageLockdown = false;  // false = incubation (1-18), true = lockdown (19-21)
int atomizerMode = AUTO;
int fanMode = AUTO;
int servoMode = AUTO;
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

// Web server
ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;
Servo eggServo;

// ============================================
// WEB INTERFACE HTML (from web_ui.h)
// ============================================

// ============================================
// WEB SERVER HANDLERS
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
    .uptime-tag { background: rgba(255,255,255,0.3); padding: 6px 15px; border-radius: 20px; color: #333; font-size: 12px; font-weight: 600; }
    .stage-row { display: flex; align-items: center; justify-content: center; gap: 10px; margin-top: 8px; }
    .stage-row select { padding: 6px 10px; border: 2px solid #667eea; border-radius: 6px; font-size: 12px; font-weight: 600; color: #333; background: white; min-width: 200px; }
    .egg-turner-status { text-align: center; font-size: 11px; color: #f44336; font-weight: 600; margin-top: 4px; }
    .stage-badge { display: inline-block; padding: 2px 8px; border-radius: 10px; font-size: 10px; font-weight: 700; }
    .stage-incubation { background: #4CAF50; color: white; }
    .stage-lockdown { background: #f44336; color: white; }
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
      <div class="target-info" id="targets">Target: --°C | --%</div>
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
        grid.innerHTML += '<div class="device-card"><div class="device-name">'+d.name+'</div><div class="device-status" id="'+d.id+'">OFF</div></div>';
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
        document.getElementById('mainStageSelect').value = d.stageLockdown ? 'lockdown' : 'incubation';
        const eggTurnerStatus = document.getElementById('eggTurnerStatus');
        if (d.stageLockdown) {
          eggTurnerStatus.textContent = 'Egg Turner: OFF (during lockdown)';
        } else {
          eggTurnerStatus.textContent = '';
        }
        devices.forEach(dev => {
          const state = d[dev.id];
          const el = document.getElementById(dev.id);
          el.textContent = state ? 'ON' : 'OFF';
          el.className = 'device-status ' + (state ? 'on' : 'off');
        });
        ['heater', 'atomizer', 'fan', 'servo'].forEach(dev => {
          const mode = d[dev + 'Mode'];
          const modeText = document.getElementById(dev + 'ModeText');
          const modeSwitch = document.getElementById(dev + 'ModeSwitch');
          modeText.textContent = mode === 0 ? 'OFF' : 'AUTO';
          modeText.className = 'mode-text ' + (mode === 0 ? 'killed' : 'auto');
          modeSwitch.checked = (mode === 0);
        });
        if (d.targetTemp) document.getElementById('targets').textContent = 'Target: ' + d.targetTemp.toFixed(1) + 'C | ' + d.targetHumidity.toFixed(0) + '%';
        if (d.log && d.log.length > 0) { console.log('Log entries:', d.log.length); drawTempChart(d.log); drawHumChart(d.log); drawCtrlChart(d.log); }
      }).catch(e => console.error('Data fetch error:', e));
    }
    // Fix: Call updateData on load and as failsafe after 2s
    updateData();
    setTimeout(function() {
      if (document.getElementById('temp').textContent === '--°C') updateData();
    }, 2000);
    function saveMainStage() {
      const stage = document.getElementById('mainStageSelect').value;
      fetch('/mock/api?stageType=' + stage).then(r => r.text()).then(msg => { updateData(); }).catch(e => console.error(e));
    }
    function formatTime(ms) {
      if (ms < 60000) return (ms/1000).toFixed(0)+'s';
      if (ms < 3600000) return (ms/60000).toFixed(0)+'m';
      if (ms < 86400000) return (ms/3600000).toFixed(0)+'h';
      return (ms/86400000).toFixed(0)+'d';
    }
    function formatTimeLabel(ms) {
      if (ms < 60000) return Math.round(ms/1000)+'s';
      if (ms < 3600000) return Math.round(ms/60000)+'m';
      if (ms < 86400000) return Math.round(ms/3600000)+'h';
      return Math.round(ms/86400000)+'d';
    }
    function drawTempChart(logData) {
      const c = document.getElementById('tempChart');
      const ctx = c.getContext('2d');
      const w = c.width = c.offsetWidth;
      const h = c.height = 120;
      const padL = 35, padR = 10, padT = 10, padB = 25;
      const plotW = w - padL - padR;
      const plotH = h - padT - padB;
      ctx.clearRect(0, 0, w, h);
      if (!logData || logData.length < 1) return;
      
      let minT = 50, maxT = 0;
      logData.forEach(p => { if (p.temp > maxT) maxT = p.temp; if (p.temp < minT) minT = p.temp; });
      if (maxT - minT < 2) { minT = 35; maxT = 40; }
      minT = Math.floor(minT - 1); maxT = Math.ceil(maxT + 1);
      
      const times = logData.map(p => p.t);
      const minTime = times[0], maxTime = times[times.length-1], timeRange = maxTime - minTime || 1;
      
      ctx.fillStyle = '#666'; ctx.font = '9px Arial'; ctx.textAlign = 'right';
      const ySteps = 4;
      for (let i = 0; i <= ySteps; i++) {
        const val = minT + (maxT - minT) * (ySteps - i) / ySteps;
        const y = padT + (i / ySteps) * plotH;
        ctx.fillText(val.toFixed(1), padL - 4, y + 3);
        ctx.beginPath(); ctx.strokeStyle = '#eee'; ctx.lineWidth = 1;
        ctx.moveTo(padL, y); ctx.lineTo(padL + plotW, y); ctx.stroke();
      }
      
      ctx.strokeStyle = '#ff5722'; ctx.lineWidth = 2; ctx.beginPath();
      logData.forEach((p, i) => {
        const x = padL + ((p.t - minTime) / timeRange) * plotW;
        const y = padT + ((maxT - p.temp) / (maxT - minT)) * plotH;
        i === 0 ? ctx.moveTo(x, y) : ctx.lineTo(x, y);
      });
      ctx.stroke();
      
      ctx.strokeStyle = '#4CAF50'; ctx.setLineDash([4, 4]);
      const targetY = padT + ((maxT - 37.5) / (maxT - minT)) * plotH;
      ctx.beginPath(); ctx.moveTo(padL, targetY); ctx.lineTo(padL + plotW, targetY); ctx.stroke(); ctx.setLineDash([]);
      
      ctx.fillStyle = '#999'; ctx.font = '9px Arial'; ctx.textAlign = 'center';
      const xLabels = 5;
      for (let i = 0; i <= xLabels; i++) {
        const t = minTime + (timeRange * i / xLabels);
        const x = padL + (i / xLabels) * plotW;
        ctx.fillText(formatTimeLabel(t - minTime), x, h - 5);
      }
      
      document.getElementById('tempTime').textContent = formatTime(timeRange);
    }
    function drawHumChart(logData) {
      const c = document.getElementById('humChart');
      const ctx = c.getContext('2d');
      const w = c.width = c.offsetWidth;
      const h = c.height = 120;
      const padL = 35, padR = 10, padT = 10, padB = 25;
      const plotW = w - padL - padR;
      const plotH = h - padT - padB;
      ctx.clearRect(0, 0, w, h);
      if (!logData || logData.length < 1) return;
      
      let minH = 100, maxH = 0;
      logData.forEach(p => { if (p.hum > maxH) maxH = p.hum; if (p.hum < minH) minH = p.hum; });
      if (maxH - minH < 5) { minH = 40; maxH = 80; }
      minH = Math.floor(minH - 5); maxH = Math.ceil(maxH + 5);
      
      const times = logData.map(p => p.t);
      const minTime = times[0], maxTime = times[times.length-1], timeRange = maxTime - minTime || 1;
      
      ctx.fillStyle = '#666'; ctx.font = '9px Arial'; ctx.textAlign = 'right';
      const ySteps = 4;
      for (let i = 0; i <= ySteps; i++) {
        const val = minH + (maxH - minH) * (ySteps - i) / ySteps;
        const y = padT + (i / ySteps) * plotH;
        ctx.fillText(val.toFixed(0), padL - 4, y + 3);
        ctx.beginPath(); ctx.strokeStyle = '#eee'; ctx.lineWidth = 1;
        ctx.moveTo(padL, y); ctx.lineTo(padL + plotW, y); ctx.stroke();
      }
      
      ctx.strokeStyle = '#2196F3'; ctx.lineWidth = 2; ctx.beginPath();
      logData.forEach((p, i) => {
        const x = padL + ((p.t - minTime) / timeRange) * plotW;
        const y = padT + ((maxH - p.hum) / (maxH - minH)) * plotH;
        i === 0 ? ctx.moveTo(x, y) : ctx.lineTo(x, y);
      });
      ctx.stroke();
      
      ctx.strokeStyle = '#4CAF50'; ctx.setLineDash([4, 4]);
      const targetY = padT + ((maxH - 60) / (maxH - minH)) * plotH;
      ctx.beginPath(); ctx.moveTo(padL, targetY); ctx.lineTo(padL + plotW, targetY); ctx.stroke(); ctx.setLineDash([]);
      
      ctx.fillStyle = '#999'; ctx.font = '9px Arial'; ctx.textAlign = 'center';
      const xLabels = 5;
      for (let i = 0; i <= xLabels; i++) {
        const t = minTime + (timeRange * i / xLabels);
        const x = padL + (i / xLabels) * plotW;
        ctx.fillText(formatTimeLabel(t - minTime), x, h - 5);
      }
      
      document.getElementById('humTime').textContent = formatTime(timeRange);
    }
    const ctrlDevices = [
      { key: 'h', name: 'Heater', color: '#f44336' },
      { key: 'a', name: 'Atomizer', color: '#2196F3' },
      { key: 'f', name: 'Fan', color: '#4CAF50' },
      { key: 's', name: 'Turner', color: '#FF9800' }
    ];
    const zoomState = { temp: { scale: 1, offset: 0 }, hum: { scale: 1, offset: 0 }, ctrl: { scale: 1, offset: 0 } };
    function setupPinchZoom(canvasId, key) {
      const c = document.getElementById(canvasId);
      let startDist = 0, startScale = 1;
      c.addEventListener('touchstart', e => { 
        e.stopPropagation();
        if (e.touches.length === 2) { 
          startDist = Math.hypot(e.touches[0].clientX-e.touches[1].clientX, e.touches[0].clientY-e.touches[1].clientY); 
          startScale = zoomState[key].scale; 
        } 
      }, { passive: true });
      c.addEventListener('touchmove', e => { 
        e.stopPropagation();
        if (e.touches.length === 2) { 
          const dist = Math.hypot(e.touches[0].clientX-e.touches[1].clientX, e.touches[0].clientY-e.touches[1].clientY); 
          zoomState[key].scale = Math.max(1, Math.min(10, startScale * (dist/startDist))); 
          updateData(); 
        } 
      }, { passive: true });
    }
    function drawCtrlChart(logData) {
      const c = document.getElementById('ctrlChart');
      const ctx = c.getContext('2d');
      const w = c.width = c.offsetWidth;
      const h = c.height = 120;
      const padL = 35, padR = 10, padT = 10, padB = 25;
      const plotW = w - padL - padR;
      const plotH = h - padT - padB;
      ctx.clearRect(0, 0, w, h);
      if (!logData || logData.length < 1) return;
      
      const times = logData.map(p => p.t);
      const minTime = times[0], maxTime = times[times.length-1], timeRange = maxTime - minTime || 1;
      const scale = zoomState.ctrl.scale;
      const viewW = plotW / scale;
      const offset = zoomState.ctrl.offset * (plotW - viewW);
      
      const legend = document.getElementById('ctrlLegend');
      legend.innerHTML = ctrlDevices.map(d => '<div class="legend-item"><div class="legend-dot" style="background:'+d.color+'"></div><span>'+d.name+'</span></div>').join('');
      
      const rowH = plotH / 4;
      ctrlDevices.forEach((dev, idx) => {
        ctx.beginPath(); ctx.strokeStyle = dev.color; ctx.lineWidth = 2;
        let started = false;
        logData.forEach((p, i) => {
          const x = padL + ((p.t - minTime) / timeRange) * viewW + offset;
          if (x < padL || x > padL + plotW) return;
          const val = p[dev.key] ? 1 : 0;
          const y = padT + (idx + 0.5) * rowH + (1 - val) * rowH * 0.4;
          started ? ctx.lineTo(x, y) : ctx.moveTo(x, y);
          started = true;
        });
        ctx.stroke();
      });
      
      ctx.fillStyle = '#999'; ctx.font = '9px Arial'; ctx.textAlign = 'center';
      const xLabels = 5;
      for (let i = 0; i <= xLabels; i++) {
        const t = minTime + (timeRange * i / xLabels);
        const x = padL + (i / xLabels) * plotW;
        ctx.fillText(formatTimeLabel(t - minTime), x, h - 5);
      }
      
      document.getElementById('ctrlTime').textContent = formatTime(timeRange);
    }
    function toggleDeviceMode(device) {
      const modeSwitch = document.getElementById(device + 'ModeSwitch');
      const mode = modeSwitch.checked ? 'off' : 'auto';
      fetch('/control?device='+device+'&mode='+mode).then(() => updateData()).catch(e => console.error(e));
    }
    function checkOta() {
      document.getElementById('otaStatus').textContent = 'Checking...';
      fetch('/ota/check').then(r => r.json()).then(d => {
        document.getElementById('otaStatus').textContent = d.update ? 'Update: '+d.version : 'Up to date';
        if (d.update) fetch('/ota/update').then(r => r.json()).then(r => document.getElementById('otaStatus').textContent = r.status);
      }).catch(e => document.getElementById('otaStatus').textContent = 'Error');
    }
    window.addEventListener('resize', () => updateData());
    setupPinchZoom('tempChart', 'temp');
    setupPinchZoom('humChart', 'hum');
    setupPinchZoom('ctrlChart', 'ctrl');
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
    h1 span { color: #ffd700; }
    .header { display: flex; justify-content: space-between; align-items: center; background: rgba(255,255,255,0.15); padding: 10px 16px; border-radius: 12px; margin-bottom: 12px; backdrop-filter: blur(10px); }
    .back-link { color: white; text-decoration: none; font-size: 14px; font-weight: 600; }
    .input-row { display: flex; align-items: center; justify-content: space-between; margin-bottom: 10px; }
    .input-row label { color: #333; font-size: 13px; font-weight: 600; }
    select { padding: 8px 12px; border: 2px solid #e94560; border-radius: 8px; width: 140px; font-size: 13px; font-weight: 600; color: #1a1a2e; background: white; cursor: pointer; }
    select:focus { outline: none; border-color: #0f3460; }
    .btn { padding: 10px 20px; border: none; border-radius: 8px; cursor: pointer; font-size: 14px; font-weight: 600; width: 100%; }
    .btn-primary { background: linear-gradient(135deg, #e94560 0%, #c53b5a 100%); color: white; box-shadow: 0 4px 15px rgba(233,69,96,0.4); }
    .btn-primary:hover { transform: translateY(-2px); box-shadow: 0 6px 20px rgba(233,69,96,0.5); }
    .input-group { display: flex; gap: 10px; }
    .input-group input { width: 80px; }
    .input-wrapper { display: flex; align-items: center; gap: 6px; }
    .input-wrapper span { color: #666; font-weight: 600; }
    input[type="number"] { padding: 8px 10px; border: 2px solid #ddd; border-radius: 8px; font-size: 14px; font-weight: 600; width: 100%; }
    input[type="number"]:focus { outline: none; border-color: #e94560; }
    .toggle-row { display: flex; justify-content: space-between; align-items: center; padding: 10px 0; }
    .toggle-label { font-size: 14px; font-weight: 600; color: #333; }
    .switch { position: relative; display: inline-block; width: 52px; height: 28px; }
    .switch input { opacity: 0; width: 0; height: 0; }
    .slider { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; transition: .3s; border-radius: 28px; }
    .slider:before { position: absolute; content: ""; height: 22px; width: 22px; left: 3px; bottom: 3px; background: white; transition: .3s; border-radius: 50%; box-shadow: 0 2px 4px rgba(0,0,0,0.2); }
    input:checked + .slider { background: linear-gradient(135deg, #e94560 0%, #c53b5a 100%); }
    input:checked + .slider:before { transform: translateX(24px); }
    .mode-text { font-size: 12px; color: #666; font-weight: 700; text-transform: uppercase; }
    .status-row { display: flex; justify-content: space-between; padding: 10px 0; border-bottom: 1px solid #eee; }
    .status-row:last-child { border-bottom: none; }
    .stat { font-size: 18px; font-weight: bold; }
    .on { color: #4CAF50; }
    .off { color: #f44336; }
    .sys-row { display: flex; justify-content: space-between; padding: 8px 0; border-bottom: 1px solid #eee; font-size: 12px; }
    .sys-row:last-child { border-bottom: none; }
    .sys-label { color: #666; }
    .sys-val { font-weight: 700; color: #333; }
    .stage-badge { display: inline-block; padding: 4px 12px; border-radius: 20px; font-size: 12px; font-weight: 700; }
    .stage-incubation { background: #4CAF50; color: white; }
    .stage-lockdown { background: #e94560; color: white; }
    .footer { text-align: center; margin-top: 15px; padding: 10px; color: rgba(255,255,255,0.6); font-size: 11px; }
    footer a { color: #ffd700; }
  </style>
</head>
<body>
  <div class="container">
    <h1><span>🥚</span> EGGubator</h1>
    <div class="header">
      <a href="/" class="back-link">← Back</a>
      <span style="color:white;font-weight:600;font-size:13px;" id="stageBadge">INCUBATION</span>
    </div>
    <div class="card">
      <div class="card-header">
        <span class="card-title">Timing Settings</span>
      </div>
      <div class="input-row">
        <label>Log Interval:</label>
        <select id="logInterval" onchange="saveLogInterval()">
          <option value="5000">5 sec</option>
          <option value="10000">10 sec</option>
          <option value="15000">15 sec</option>
          <option value="20000">20 sec</option>
          <option value="25000">25 sec</option>
          <option value="30000">30 sec</option>
        </select>
      </div>
      <div class="input-row">
        <label>Save Flash:</label>
        <select id="saveFlashInterval" onchange="saveFlashInterval()">
          <option value="3600000">60 min</option>
          <option value="5400000">90 min</option>
          <option value="7200000">2 hrs</option>
          <option value="9000000">150 min</option>
          <option value="10800000">3 hrs</option>
          <option value="12600000">210 min</option>
          <option value="14400000">4 hrs</option>
        </select>
      </div>
      <div class="input-row">
        <label>Atomizer:</label>
        <select id="atomizerPulse" onchange="saveAtomizerPulse()">
          <option value="2000">2 sec</option>
          <option value="3000">3 sec</option>
          <option value="4000">4 sec</option>
          <option value="5000">5 sec</option>
        </select>
      </div>
      <div class="input-row">
        <label>Egg Turner:</label>
        <select id="eggTurnInterval" onchange="saveEggTurnInterval()">
          <option value="900000">15 min</option>
          <option value="1800000">30 min</option>
          <option value="3600000">1 hour</option>
          <option value="7200000">2 hours</option>
          <option value="10800000">3 hours</option>
          <option value="14400000">4 hours</option>
          <option value="21600000">6 hours</option>
        </select>
      </div>
    </div>
    <div class="card">
      <div class="card-header">
        <span class="card-title">Simulation</span>
      </div>
      <div class="toggle-row">
        <span class="toggle-label">Mock Sensor</span>
        <label class="switch">
          <input type="checkbox" id="mockSwitch" onchange="toggleMock()">
          <span class="slider"></span>
        </label>
      </div>
      <div class="toggle-row">
        <span class="toggle-label">Auto Simulation</span>
        <label class="switch">
          <input type="checkbox" id="autoSimSwitch" onchange="toggleAutoSim()">
          <span class="slider"></span>
        </label>
      </div>
    </div>
    <div class="card" id="mockValuesCard" style="display:none;">
      <div class="card-header">
        <span class="card-title">Mock Values</span>
      </div>
      <div class="input-row">
        <label>Temperature:</label>
        <div class="input-wrapper">
          <input type="number" id="mockTemp" value="25" step="0.1" min="20" max="45">
          <span>°C</span>
        </div>
      </div>
      <div class="input-row">
        <label>Humidity:</label>
        <div class="input-wrapper">
          <input type="number" id="mockHum" value="50" step="0.1" min="0" max="100">
          <span>%</span>
        </div>
      </div>
      <button class="btn btn-primary" id="setValuesBtn" onclick="setMockValues()">Apply Values</button>
    </div>
    <div class="card">
      <div class="card-header">
        <span class="card-title">Current Status</span>
      </div>
      <div class="status-row"><span>Sensor</span><span class="stat" id="sensorStatus">Real</span></div>
      <div class="status-row"><span>Temperature</span><span class="stat" id="currentTemp">--°C</span></div>
      <div class="status-row"><span>Humidity</span><span class="stat" id="currentHum">--%</span></div>
    </div>
    <div class="card">
      <div class="card-header">
        <span class="card-title">System Info</span>
      </div>
      <div class="sys-row"><span class="sys-label">RAM</span><span class="sys-val" id="sysHeap">--</span></div>
      <div class="sys-row"><span class="sys-label">CPU</span><span class="sys-val" id="sysCpu">--</span></div>
      <div class="sys-row"><span class="sys-label">Uptime</span><span class="sys-val" id="sysUptime">--</span></div>
      <div class="sys-row"><span class="sys-label">Log Records</span><span class="sys-val" id="sysLogCnt">--</span></div>
      <div class="sys-row"><span class="sys-label">Log Storage</span><span class="sys-val" id="sysLogStorage">--</span></div>
    </div>
    <div class="footer">EGGubator v<span id="version">--</span></div>
  </div>
  <script>
    let userEditing = false;
    function updateData() {
      fetch('/data').then(r => r.json()).then(d => {
        document.getElementById('version').textContent = d.version;
        document.getElementById('currentTemp').textContent = d.temperature.toFixed(1)+'°C';
        document.getElementById('currentHum').textContent = d.humidity.toFixed(1)+'%';
        const mockOn = d.mock;
        const autoSimOn = d.autosim;
        document.getElementById('mockSwitch').checked = mockOn;
        document.getElementById('autoSimSwitch').checked = autoSimOn;
        document.getElementById('sensorStatus').textContent = mockOn ? 'Mock' : (autoSimOn ? 'Auto-Sim' : 'Real');
        document.getElementById('sensorStatus').className = 'stat ' + ((mockOn || autoSimOn) ? 'on' : 'off');
        document.getElementById('setValuesBtn').disabled = !mockOn;
        document.getElementById('mockValuesCard').style.display = mockOn ? 'block' : 'none';
        const stage = d.stageLockdown;
        const badge = document.getElementById('stageBadge');
        if (stage) {
          badge.textContent = 'LOCKDOWN';
          badge.className = 'stage-badge stage-lockdown';
        } else {
          badge.textContent = 'INCUBATION';
          badge.className = 'stage-badge stage-incubation';
        }
        document.getElementById('incubationStage').value = stage ? 'lockdown' : 'incubation';
        
        // System info - show always
        if (d.sys && d.sys.heapFree) {
          document.getElementById('sysHeap').textContent = (d.sys.heapFree/1024).toFixed(0) + ' KB free';
        } else {
          document.getElementById('sysHeap').textContent = '--';
        }
        
        if (d.sys && d.sys.cpu) {
          document.getElementById('sysCpu').textContent = d.sys.cpu + '%';
        } else {
          document.getElementById('sysCpu').textContent = '--';
        }
        
        document.getElementById('sysUptime').textContent = d.uptime || '--';
        document.getElementById('sysLogCnt').textContent = (d.logCnt || 0) + ' records';
        document.getElementById('sysLogStorage').textContent = (d.logStorage || 0) + ' KB';
        
      });
    }
    function loadMockValues() {
      fetch('/mock/api').then(r => r.json()).then(d => {
        document.getElementById('mockTemp').value = d.temp;
        document.getElementById('mockHum').value = d.hum;
        document.getElementById('logInterval').value = d.logInterval;
        document.getElementById('saveFlashInterval').value = d.saveFlashInterval;
        document.getElementById('eggTurnInterval').value = d.eggTurnInterval;
      }).catch(e => console.error(e));
    }
    function toggleMock() {
      const enable = document.getElementById('mockSwitch').checked;
      const mockValuesCard = document.getElementById('mockValuesCard');
      mockValuesCard.style.display = enable ? 'block' : 'none';
      fetch('/mock/api?enable=' + (enable ? '1' : '0')).then(r => r.text()).then(() => { updateData(); loadMockValues(); }).catch(e => console.error(e));
    }
    function toggleAutoSim() {
      const enable = document.getElementById('autoSimSwitch').checked;
      fetch('/mock/api?autosim=' + (enable ? '1' : '0')).then(r => r.text()).then(() => { updateData(); }).catch(e => console.error(e));
    }
    function saveLogInterval() {
      const val = document.getElementById('logInterval').value;
      fetch('/mock/api?logInterval=' + val).then(r => r.text()).then(msg => console.log(msg)).catch(e => console.error(e));
    }
    function saveFlashInterval() {
      const val = document.getElementById('saveFlashInterval').value;
      fetch('/mock/api?saveFlashInterval=' + val).then(r => r.text()).then(msg => console.log(msg)).catch(e => console.error(e));
    }
    function saveEggTurnInterval() {
      const val = document.getElementById('eggTurnInterval').value;
      fetch('/mock/api?eggTurnInterval=' + val).then(r => r.text()).then(msg => console.log(msg)).catch(e => console.error(e));
    }
    function saveAtomizerPulse() {
      const val = document.getElementById('atomizerPulse').value;
      fetch('/mock/api?pulseOnTime=' + val).then(r => r.text()).then(msg => console.log(msg)).catch(e => console.error(e));
    }
    function saveIncubationStage() {
      const stage = document.getElementById('incubationStage').value;
      fetch('/mock/api?stageType=' + stage).then(r => r.text()).then(msg => { updateData(); }).catch(e => console.error(e));
    }
    function setMockValues() {
      const t = document.getElementById('mockTemp').value;
      const h = document.getElementById('mockHum').value;
      fetch('/mock/api?temp=' + t + '&hum=' + h).then(r => r.text()).then(msg => { updateData(); }).catch(e => console.error(e));
    }
    setInterval(updateData, 3000);
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
                 ",\"autosim\":" + String(autoSimMode ? "true" : "false") +
                ",\"stageLockdown\":" + String(stageLockdown ? "true" : "false") +
                ",\"heaterMode\":" + String(heaterMode) +
                ",\"atomizerMode\":" + String(atomizerMode) +
                ",\"fanMode\":" + String(fanMode) +
                ",\"servoMode\":" + String(servoMode) +
                ",\"targetTemp\":" + String(TARGET_TEMP) +
                ",\"targetHumidity\":" + String(TARGET_HUMIDITY) +
                ",\"logCnt\":" + String(logIndex) +
                ",\"logStorage\":" + String((logIndex * sizeof(LogEntry)) / 1024) +
                ",\"sys\":{\"heapFree\":" + String(ESP.getFreeHeap()) +
                ",\"heapTotal\":81920" +
                ",\"cpu\":" + String(cpuUtil) +
                ",\"flashSize\":" + String(ESP.getFlashChipSize()) +
                ",\"flashTotal\":4194304" +
                "}";
  getLogDataForWeb(json);
  json += "}";
  server.send(200, "application/json", json);
}

void handleControl() {
  if (server.hasArg("device") && server.hasArg("mode")) {
    String device = server.arg("device");
    String mode = server.arg("mode");
    bool isKillOff = (mode == "off");
    
    if (device == "heater") {
      heaterMode = isKillOff ? KILL_OFF : AUTO;
      if (isKillOff) {
        heaterState = false;
        digitalWrite(RELAY_HEATER, LOW);
      }
    } else if (device == "atomizer") {
      atomizerMode = isKillOff ? KILL_OFF : AUTO;
      if (isKillOff) {
        atomizerState = false;
        digitalWrite(RELAY_ATOMIZER, LOW);
        atomizerPulsing = false;
        atomizerInOffPhase = false;
      }
    } else if (device == "fan") {
      fanMode = isKillOff ? KILL_OFF : AUTO;
      if (isKillOff) {
        fanState = false;
        digitalWrite(RELAY_FAN, LOW);
      }
    } else if (device == "servo") {
      servoMode = isKillOff ? KILL_OFF : AUTO;
      if (isKillOff) {
        servoEnabled = false;
        servoPosition = 0;
        eggServo.write(SERVO_CENTER);
      }
    }
    server.send(200, "text/plain", device + " mode set to " + (isKillOff ? "OFF" : "AUTO"));
  } else {
    server.send(200, "text/plain", "Invalid request");
  }
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
  if (server.hasArg("autosim")) {
    bool enable = (server.arg("autosim") == "1");
    setAutoSim(enable);
    server.send(200, "text/plain", enable ? "Auto simulation enabled" : "Auto simulation disabled");
  } else if (server.hasArg("enable")) {
    bool enable = (server.arg("enable") == "1");
    if (enable) setAutoSim(false);
    setMockSensor(enable);
    server.send(200, "text/plain", enable ? "Mock sensor enabled" : "Mock sensor disabled");
  } else if (server.hasArg("temp") && server.hasArg("hum")) {
    float t = server.arg("temp").toFloat();
    float h = server.arg("hum").toFloat();
    setAutoSim(false);
    setMockSensor(true);
    setMockValues(t, h);
    server.send(200, "text/plain", "Mock values set: " + String(t) + "C, " + String(h) + "%");
  } else if (server.hasArg("logInterval")) {
    unsigned long val = server.arg("logInterval").toInt();
    LOG_INTERVAL = val;
    server.send(200, "text/plain", "Log interval set to " + String(val/1000) + "s");
  } else if (server.hasArg("saveFlashInterval")) {
    unsigned long val = server.arg("saveFlashInterval").toInt();
    SAVE_FLASH_INTERVAL = val;
    server.send(200, "text/plain", "Save to Flash interval set to " + String(val/60000) + "min");
  } else if (server.hasArg("eggTurnInterval")) {
    unsigned long val = server.arg("eggTurnInterval").toInt();
    EGG_TURN_INTERVAL = val;
    server.send(200, "text/plain", "Egg turner interval set to " + String(val/3600000) + " hours");
  } else if (server.hasArg("pulseOnTime")) {
    unsigned long val = server.arg("pulseOnTime").toInt();
    PULSE_ON_TIME = val;
    server.send(200, "text/plain", "Atomizer pulse set to " + String(val/1000) + "s");
  } else if (server.hasArg("stageType")) {
    String type = server.arg("stageType");
    if (type == "lockdown") {
      stageLockdown = true;
      TARGET_TEMP = 37.5;
      TARGET_HUMIDITY = 65.0;
    } else if (type == "incubation") {
      stageLockdown = false;
      TARGET_TEMP = 37.5;
      TARGET_HUMIDITY = 55.0;
    }
    server.send(200, "text/plain", "Stage set to " + type);
  } else {
    String json = "{\"enabled\":" + String(useMockSensor ? "true" : "false") + 
                  ",\"autosim\":" + String(autoSimMode ? "true" : "false") +
                  ",\"temp\":" + String(mockTemp) + 
                  ",\"hum\":" + String(mockHum) +
                  ",\"logInterval\":" + String(LOG_INTERVAL) +
                  ",\"saveFlashInterval\":" + String(SAVE_FLASH_INTERVAL) +
                  ",\"eggTurnInterval\":" + String(EGG_TURN_INTERVAL) +
                  ",\"stageLockdown\":" + String(stageLockdown ? "true" : "false") + "}";
    server.send(200, "application/json", json);
  }
}

// ============================================
// AUTO CONTROL LOGIC
// ============================================
void autoControl() {
  if (!isnan(currentTemp) && !isnan(currentHumidity)) {
    unsigned long now = millis();
    
    if (heaterMode == AUTO) {
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
    } else {
      if (heaterState) {
        heaterState = false;
        digitalWrite(RELAY_HEATER, LOW);
        heaterWasOn = false;
      }
    }
    
    if (atomizerMode == AUTO) {
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
    } else {
      if (atomizerState) {
        atomizerState = false;
        digitalWrite(RELAY_ATOMIZER, LOW);
        atomizerWasOn = false;
        atomizerPulsing = false;
        atomizerInOffPhase = false;
      }
    }
    
    bool tempStable = (currentTemp >= TARGET_TEMP - TEMP_HYSTERESIS && 
                     currentTemp <= TARGET_TEMP + TEMP_HYSTERESIS);
    bool humStable = (currentHumidity >= TARGET_HUMIDITY - HUMIDITY_HYSTERESIS && 
                     currentHumidity <= TARGET_HUMIDITY + HUMIDITY_HYSTERESIS);
    
    if (fanMode == AUTO) {
      bool withinHeaterWindow = (!heaterState && (now - heaterLastChanged < FAN_EXTEND_TIME));
      bool withinAtomizerWindow = (!atomizerState && (now - atomizerLastChanged < FAN_EXTEND_TIME));
      
      if (heaterState || withinHeaterWindow || atomizerState || withinAtomizerWindow || 
          currentTemp > MAX_SAFE_TEMP) {
        fanState = true;
      } else if (tempStable && humStable) {
        fanState = false;
      }
      digitalWrite(RELAY_FAN, fanState ? HIGH : LOW);
    } else {
      if (fanState) {
        fanState = false;
        digitalWrite(RELAY_FAN, LOW);
      }
    }
  }
}

// ============================================
// EGG TURNER
// ============================================
void rotateEggs() {
  static bool turning = false;
  static unsigned long turnStartTime = 0;
  static int currentAngle = 0;
  static bool direction = true;
  
  // Disable egg turner during lockdown stage
  if (stageLockdown) {
    servoEnabled = false;
    servoPosition = 0;
    eggServo.write(SERVO_CENTER);
    return;
  }
  
  if (servoEnabled && servoMode == AUTO && !turning && (millis() - lastServoTurn > EGG_TURN_INTERVAL)) {
    turning = true;
    turnStartTime = millis();
    direction = true;
    currentAngle = SERVO_CENTER - SERVO_ANGLE;
    eggServo.write(currentAngle);
    servoPosition = -1;
    Serial.println("Egg turn started");
  }
  
  if (turning && (millis() - turnStartTime < EGG_TURN_DURATION)) {
    unsigned long elapsed = millis() - turnStartTime;
    int targetAngle;
    
    if (elapsed < EGG_TURN_DURATION / 2) {
      targetAngle = map(elapsed, 0, EGG_TURN_DURATION/2, SERVO_CENTER - SERVO_ANGLE, SERVO_CENTER + SERVO_ANGLE);
      servoPosition = 1;
    } else {
      targetAngle = map(elapsed, EGG_TURN_DURATION/2, EGG_TURN_DURATION, SERVO_CENTER + SERVO_ANGLE, SERVO_CENTER - SERVO_ANGLE);
      servoPosition = -1;
    }
    
    if (targetAngle != currentAngle) {
      currentAngle = targetAngle;
      eggServo.write(targetAngle);
    }
  }
  
  if (turning && (millis() - turnStartTime >= EGG_TURN_DURATION)) {
    turning = false;
    eggServo.write(SERVO_CENTER - SERVO_ANGLE);
    servoPosition = -1;
    lastServoTurn = millis();
    Serial.println("Egg turn completed");
  }
  
  if (!servoEnabled || servoMode == KILL_OFF) {
    servoPosition = 0;
    eggServo.write(SERVO_CENTER);
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

  initDHT();
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
    if (autoSimMode) {
      updateAutoSim(heaterState, atomizerState, fanState);
    }
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
    logData(currentTemp, currentHumidity, heaterState, atomizerState, fanState, servoPosition);
    lastLogTime = millis();
  }

  if (shouldSaveToFlash()) {
    saveLogsToFlash();
  }

  if (servoEnabled) {
    rotateEggs();
  }

  if (millis() - lastOtaCheck > 3600000) {
    checkAndUpdateAuto();
    lastOtaCheck = millis();
  }
}
