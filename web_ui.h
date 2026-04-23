#line 1 "/root/termux_home/eggubator/web_ui.h"
#ifndef WEB_UI_H
#define WEB_UI_H

#define DEXIE_ASSET_URL "https://cdn.jsdelivr.net/npm/dexie@3.2.7/dist/dexie.min.js"
#define CHARTJS_ASSET_URL "https://cdn.jsdelivr.net/npm/chart.js"

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
    .ticker { background: #2196F3; color: white; padding: 10px 15px; border-radius: 8px; margin-bottom: 15px; text-align: center; font-weight: 600; font-size: 14px; display: none; }
    .ticker.show { display: block; animation: tickerPulse 1s ease-in-out infinite; }
    @keyframes tickerPulse { 0%, 100% { opacity: 1; } 50% { opacity: 0.7; } }
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
          <span class="device-label">Egg Turner</span>
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
      <div class="ticker" id="syncTicker"></div>
      <div class="header" style="display:flex;justify-content:space-between;align-items:center;">
        <div class="uptime-tag" id="uptime">--</div>
        <div class="countdown-timer" id="countdownTimer">Next: --s</div>
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
      <button class="btn btn-auto" onclick="checkOta()">Check Update</button>
      <div style="text-align:center;margin-top:8px;font-size:12px;color:#666;" id="otaStatus"></div>
    </div>
    <div class="card">
      <div class="label">Temperature</div>
      <div class="chart-wrap"><canvas id="tempChart"></canvas></div>
      <div class="time-label" id="tempTime">--</div>
    </div>
    <div class="card">
      <div class="label">Humidity</div>
      <div class="chart-wrap"><canvas id="humChart"></canvas></div>
      <div class="time-label" id="humTime">--</div>
    </div>
    <div class="card">
      <div class="label">Controls</div>
      <div class="chart-wrap"><canvas id="ctrlChart"></canvas></div>
      <div class="time-label" id="ctrlTime">--</div>
      <div class="legend" id="ctrlLegend"></div>
    </div>
    <div class="version">v<span id="version">--</span></div>
  </div>
)webui"
"<script src=\"" DEXIE_ASSET_URL "\"></script>\n"
"<script src=\"" CHARTJS_ASSET_URL "\"></script>\n"
R"webui(  <script>
    const DB_NAME = 'EggubatorDB';
    const LOG_STORE_SCHEMA = '[bootId+t], timestamp';
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
    const zoomState = {
      temp: { scale: 1, offset: 0 },
      hum: { scale: 1, offset: 0 },
      ctrl: { scale: 1, offset: 0 }
    };

    let devicesInited = false;
    window.db = null;
    let logStorageReady = false;
    let isSyncing = false;
    let updateInFlight = false;
    let syncQueue = [];
    let syncBootId = null;
    let currentTargetTemp = 37.5;
    let currentTargetHumidity = 60.0;
    let lastLiveLogs = [];
    let logInterval = 10000;
    let countdownValue = 10;
    let countdownInterval = null;
    let syncStartTime = 0;
    let tempChart = null;
    let humChart = null;
    let ctrlChart = null;

    function initDevices() {
      if (devicesInited) return;
      devicesInited = true;
      const grid = document.getElementById('deviceGrid');
      let html = '';
      devices.forEach(function(device) {
        html += '<div class="device-card"><div class="device-name">' + device.name + '</div><div class="device-status" id="' + device.id + '">OFF</div></div>';
      });
      grid.innerHTML = html;
    }

    function safeGetStorage(key) {
      try {
        return localStorage.getItem(key);
      } catch (error) {
        return null;
      }
    }

    function safeSetStorage(key, value) {
      try {
        localStorage.setItem(key, value);
      } catch (error) {
        console.error('Storage write failed:', error);
      }
    }

    function safeRemoveStorage(key) {
      try {
        localStorage.removeItem(key);
      } catch (error) {
        console.error('Storage delete failed:', error);
      }
    }

    function showBanner(id, message, backgroundColor) {
      const banner = document.getElementById(id);
      if (!banner) return;
      banner.textContent = message;
      banner.style.display = 'block';
      if (backgroundColor) banner.style.background = backgroundColor;
      banner.className = 'alert show';
    }

    function hideBanner(id) {
      const banner = document.getElementById(id);
      if (!banner) return;
      banner.style.display = 'none';
      banner.classList.remove('show');
    }

    function showStorageAlert(message) {
      showBanner('storageAlert', message, '#f44336');
    }

    function coerceBool(value) {
      return value === true || value === 'true' || value === 1 || value === '1';
    }

    function normalizeLogRecord(point, bootId, absBaseTime) {
      return {
        bootId: bootId || 0,
        t: point.t,
        timestamp: absBaseTime + point.t,
        temp: point.temp,
        hum: point.hum,
        h: coerceBool(point.h),
        a: coerceBool(point.a),
        f: coerceBool(point.f),
        s: coerceBool(point.s)
      };
    }

    async function initLogStorage() {
      safeRemoveStorage('lastSyncedSector');
      if (!window.indexedDB) {
        showStorageAlert('IndexedDB unavailable. Showing live RAM logs only.');
        return;
      }
if (!window.Dexie) {
        showStorageAlert('Dexie failed to load from firmware. Showing live RAM logs only.');
        return;
      }
      
      try {
        window.db = new Dexie(DB_NAME);
        window.db.version(1).stores({
          logs: LOG_STORE_SCHEMA
        });
        await window.db.open();
        logStorageReady = true;
        hideBanner('storageAlert');
      } catch (error) {
        console.error('Dexie initialization error:', error);
        window.db = null;
        logStorageReady = false;
        showStorageAlert('Local log storage is unavailable. Showing live RAM logs only.');
      }
    }

    function getSyncKey(bootId) {
      return 'lastSyncedSector:' + String(bootId || 0);
    }

    async function cleanupOldLogs() {
      if (!logStorageReady || !window.db) return;
      const thirtyDaysAgo = Date.now() - (30 * 24 * 3600 * 1000);
      try {
        await window.db.logs.where('timestamp').below(thirtyDaysAgo).delete();
      } catch (error) {
        console.error('Cleanup error:', error);
      }
    }

    async function getPendingGap(sinceTimestamp) {
      if (!logStorageReady || !window.db) return 0;
      try {
        const response = await fetch('/data?since=' + sinceTimestamp + '&limit=201');
        const data = await response.json();
        return data.records ? data.records.length : 0;
      } catch (error) {
        console.error('Gap check error:', error);
        return 0;
      }
    }

    function showTicker(message) {
      const ticker = document.getElementById('syncTicker');
      if (ticker) {
        ticker.textContent = message;
        ticker.className = 'ticker show';
      }
    }

    function hideTicker() {
      const ticker = document.getElementById('syncTicker');
      if (ticker) {
        ticker.className = 'ticker';
        ticker.textContent = '';
      }
    }

    function startCountdown() {
      if (countdownInterval) clearInterval(countdownInterval);
      countdownValue = Math.ceil(logInterval / 1000);
      updateCountdownDisplay();
      countdownInterval = setInterval(function() {
        countdownValue--;
        if (countdownValue <= 0) countdownValue = Math.ceil(logInterval / 1000);
        updateCountdownDisplay();
      }, 1000);
    }

    function updateCountdownDisplay() {
      const timer = document.getElementById('countdownTimer');
      if (timer) {
        timer.textContent = 'Next: ' + countdownValue + 's';
      }
    }

    function stopCountdown() {
      if (countdownInterval) {
        clearInterval(countdownInterval);
        countdownInterval = null;
      }
      const timer = document.getElementById('countdownTimer');
      if (timer) timer.textContent = '';
    }

    async function syncLogs() {
      if (!logStorageReady || !window.db || isSyncing) return;

      isSyncing = true;
      syncStartTime = Date.now();
      let totalSynced = 0;
      let since = safeGetStorage('lastSyncedTimestamp') || 0;
      stopCountdown();

      try {
        while (true) {
          showTicker('Syncing ' + totalSynced + ' records...');

          const response = await fetch('/data?since=' + since + '&limit=100');
          const data = await response.json();

          if (data.records && data.records.length > 0) {
            const records = data.records.map(function(point) {
              return normalizeLogRecord(point, 0, 0);
            });
            await window.db.logs.bulkPut(records);
            totalSynced += data.records.length;
            since = data.lastTimestamp;
            safeSetStorage('lastSyncedTimestamp', since);
          }

          if (!data.hasMore) break;

          await new Promise(r => setTimeout(r, 100));
        }

        hideTicker();
        if (totalSynced > 0) {
          showTicker(totalSynced + ' records synced');
          setTimeout(hideTicker, 3000);
        }
        startCountdown();
      } catch (error) {
        console.error('Sync error:', error);
        showTicker('Sync failed');
        setTimeout(hideTicker, 3000);
        startCountdown();
      }

      isSyncing = false;
    }

    async function loadLogsForCharts() {
      if (!logStorageReady || !window.db) {
        return lastLiveLogs;
      }
      try {
        const logs = await window.db.logs.orderBy('timestamp').toArray();
        return logs.length > 0 ? logs : lastLiveLogs;
      } catch (error) {
        console.error('Read logs error:', error);
        return lastLiveLogs;
      }
    }

    function updateStatusFromData(data) {
      document.getElementById('version').textContent = data.version;
      document.getElementById('temp').textContent = data.temperature.toFixed(1) + '°C';
      document.getElementById('hum').textContent = data.humidity.toFixed(1) + '%';
      document.getElementById('uptime').textContent = data.uptime;
      document.getElementById('mainStageSelect').value = data.stageLockdown ? 'lockdown' : 'incubation';

      const eggTurnerStatus = document.getElementById('eggTurnerStatus');
      eggTurnerStatus.textContent = data.stageLockdown ? 'Egg Turner: OFF (during lockdown)' : '';

      devices.forEach(function(device) {
        const state = data[device.id];
        const element = document.getElementById(device.id);
        element.textContent = state ? 'ON' : 'OFF';
        element.className = 'device-status ' + (state ? 'on' : 'off');
      });

      ['heater', 'atomizer', 'fan', 'servo'].forEach(function(device) {
        const mode = data[device + 'Mode'];
        const modeText = document.getElementById(device + 'ModeText');
        const modeSwitch = document.getElementById(device + 'ModeSwitch');
        modeText.textContent = mode === 0 ? 'OFF' : 'AUTO';
        modeText.className = 'mode-text ' + (mode === 0 ? 'killed' : 'auto');
        modeSwitch.checked = mode === 0;
      });

      if (typeof data.targetTemp === 'number') {
        currentTargetTemp = data.targetTemp;
      }
      if (typeof data.targetHumidity === 'number') {
        currentTargetHumidity = data.targetHumidity;
      }
      document.getElementById('targets').textContent = 'Target: ' + currentTargetTemp.toFixed(1) + 'C | ' + currentTargetHumidity.toFixed(0) + '%';

      if (data.reason) {
        showBanner('alertBox', data.reason, '#ff9800');
        setTimeout(function() {
          hideBanner('alertBox');
        }, 5000);
      }
    }

    async function updateData() {
      if (updateInFlight) return;
      updateInFlight = true;
      try {
        const response = await fetch('/data');
        const data = await response.json();
        updateStatusFromData(data);

        const absBaseTime = Date.now() - (data.uptime_ms || 0);
        lastLiveLogs = (data.log || []).map(function(point) {
          return normalizeLogRecord(point, data.bootId || 0, absBaseTime);
        });

        if (logStorageReady && window.db && lastLiveLogs.length > 0) {
          await window.db.logs.bulkPut(lastLiveLogs);
        }

        let lastDexieTimestamp = safeGetStorage('lastSyncedTimestamp') || 0;
        if (lastDexieTimestamp === 0 && logStorageReady && window.db) {
          try {
            const latestLog = await window.db.logs.orderBy('timestamp').last();
            if (latestLog) lastDexieTimestamp = latestLog.timestamp;
          } catch (e) { console.error('Get latest log error:', e); }
        }

        const gap = await getPendingGap(lastDexieTimestamp);

        if (gap > 200) {
          stopCountdown();
          await syncLogs();
          await cleanupOldLogs();
        } else {
          if (gap > 200 && data.bootId !== undefined && data.currentSector !== undefined) {
            await syncLogs();
            await cleanupOldLogs();
          }
          if (!isSyncing && countdownInterval === null) {
            startCountdown();
          }
        }

        const logs = await loadLogsForCharts();
        if (logs.length > 0) {
          drawTempChart(logs);
          drawHumChart(logs);
          drawCtrlChart(logs);
        }
      } catch (error) {
        console.error('Data fetch error:', error);
      } finally {
        updateInFlight = false;
      }
    }

    function saveMainStage() {
      const stage = document.getElementById('mainStageSelect').value;
      fetch('/mock/api?stageType=' + stage).then(function() {
        updateData();
      }).catch(function(error) {
        console.error(error);
      });
    }

    function formatTime(ms) {
      if (ms < 60000) return (ms / 1000).toFixed(0) + 's';
      if (ms < 3600000) return (ms / 60000).toFixed(0) + 'm';
      if (ms < 86400000) return (ms / 3600000).toFixed(0) + 'h';
      return (ms / 86400000).toFixed(0) + 'd';
    }

    function formatTimeLabel(ms) {
      if (ms < 60000) return Math.round(ms / 1000) + 's';
      if (ms < 3600000) return Math.round(ms / 60000) + 'm';
      if (ms < 86400000) return Math.round(ms / 3600000) + 'h';
      return Math.round(ms / 86400000) + 'd';
    }

    function initCharts() {
      var commonOpts = { responsive: true, maintainAspectRatio: false, animation: false, plugins: { legend: { display: false }, tooltip: { intersect: false, mode: 'index' } }, scales: { x: { type: 'linear', display: true, grid: { color: '#eee' }, ticks: { color: '#999', font: { size: 9 }, maxTicksLimit: 5, callback: function(v) { return formatTimeLabel(v); } } }, y: { display: true, grid: { color: '#eee' }, ticks: { color: '#666', font: { size: 9 } } } }, elements: { point: { radius: 0, hitRadius: 10 }, line: { borderWidth: 2, tension: 0.1 } } };
      tempChart = new Chart(document.getElementById('tempChart'), { type: 'line', data: { datasets: [{ label: 'Temperature', borderColor: '#ff5722', backgroundColor: 'rgba(255,87,34,0.1)', fill: true, data: [] }] }, options: Object.assign({}, commonOpts, { scales: Object.assign({}, commonOpts.scales, { y: Object.assign({}, commonOpts.scales.y, { min: 35, max: 40 }) }) }) });
      humChart = new Chart(document.getElementById('humChart'), { type: 'line', data: { datasets: [{ label: 'Humidity', borderColor: '#2196F3', backgroundColor: 'rgba(33,150,243,0.1)', fill: true, data: [] }] }, options: Object.assign({}, commonOpts, { scales: Object.assign({}, commonOpts.scales, { y: Object.assign({}, commonOpts.scales.y, { min: 40, max: 80 }) }) }) });
      document.getElementById('ctrlLegend').innerHTML = ctrlDevices.map(function(d) { return '<div class="legend-item"><div class="legend-dot" style="background:' + d.color + '"></div><span>' + d.name + '</span></div>'; }).join('');
      ctrlChart = new Chart(document.getElementById('ctrlChart'), { type: 'line', data: { datasets: ctrlDevices.map(function(d) { return { label: d.name, borderColor: d.color, backgroundColor: d.color, stepped: true, data: [] }; }) }, options: Object.assign({}, commonOpts, { scales: Object.assign({}, commonOpts.scales, { y: Object.assign({}, commonOpts.scales.y, { min: 0, max: 1, ticks: { stepSize: 1 } }) }) }) });
    }

    function downsampleData(data, maxPoints) {
      if (!data || data.length <= maxPoints) return data;
      var step = Math.ceil(data.length / maxPoints);
      var result = [];
      for (var i = 0; i < data.length; i += step) {
        var chunk = data.slice(i, Math.min(i + step, data.length));
        var avgT = 0, avgH = 0, cnt = 0;
        for (var j = 0; j < chunk.length; j++) { avgT += chunk[j].temp; avgH += chunk[j].hum; cnt++; }
        var mid = chunk[Math.floor(chunk.length / 2)];
        result.push({ t: mid.t, temp: avgT / cnt, hum: avgH / cnt, h: mid.h, a: mid.a, f: mid.f, s: mid.s });
      }
      return result;
    }

    function drawTempChart(logData) {
      if (!tempChart || !logData || logData.length < 1) return;
      var displayData = downsampleData(logData, 200);
      var minT = displayData[0].t, maxT = displayData[displayData.length - 1].t;
      tempChart.data.datasets[0].data = displayData.map(function(p) { return { x: p.t, y: p.temp }; });
      if (tempChart.data.datasets.length === 1) tempChart.data.datasets.push({ label: 'Target', borderColor: '#4CAF50', borderDash: [4, 4], borderWidth: 1, pointRadius: 0, data: [] });
      tempChart.data.datasets[1].data = [{ x: minT, y: currentTargetTemp }, { x: maxT, y: currentTargetTemp }];
      tempChart.options.scales.x.min = minT;
      tempChart.options.scales.x.max = maxT;
      tempChart.update('none');
      document.getElementById('tempTime').textContent = formatTimeLabel(maxT - minT) + (displayData.length < logData.length ? ' (' + logData.length + ' pts)' : '');
    }

    function drawHumChart(logData) {
      if (!humChart || !logData || logData.length < 1) return;
      var displayData = downsampleData(logData, 200);
      var minT = displayData[0].t, maxT = displayData[displayData.length - 1].t;
      humChart.data.datasets[0].data = displayData.map(function(p) { return { x: p.t, y: p.hum }; });
      if (humChart.data.datasets.length === 1) humChart.data.datasets.push({ label: 'Target', borderColor: '#4CAF50', borderDash: [4, 4], borderWidth: 1, pointRadius: 0, data: [] });
      humChart.data.datasets[1].data = [{ x: minT, y: currentTargetHumidity }, { x: maxT, y: currentTargetHumidity }];
      humChart.options.scales.x.min = minT;
      humChart.options.scales.x.max = maxT;
      humChart.update('none');
      document.getElementById('humTime').textContent = formatTimeLabel(maxT - minT) + (displayData.length < logData.length ? ' (' + logData.length + ' pts)' : '');
    }

    function drawCtrlChart(logData) {
      if (!ctrlChart || !logData || logData.length < 1) return;
      var displayData = downsampleData(logData, 200);
      var minT = displayData[0].t, maxT = displayData[displayData.length - 1].t;
      ctrlDevices.forEach(function(d, i) { ctrlChart.data.datasets[i].data = displayData.map(function(p) { return { x: p.t, y: p[d.key] ? 1 : 0 }; }); });
      ctrlChart.options.scales.x.min = minT;
      ctrlChart.options.scales.x.max = maxT;
      ctrlChart.update('none');
      document.getElementById('ctrlTime').textContent = formatTimeLabel(maxT - minT);
    }

    function toggleDeviceMode(device) {
      const modeSwitch = document.getElementById(device + 'ModeSwitch');
      const mode = modeSwitch.checked ? 'off' : 'auto';
      fetch('/control?device=' + device + '&mode=' + mode).then(function() {
        updateData();
      }).catch(function(error) {
        console.error(error);
      });
    }

    function checkOta() {
      document.getElementById('otaStatus').textContent = 'Checking...';
      fetch('/ota/check').then(function(response) {
        return response.json();
      }).then(function(data) {
        document.getElementById('otaStatus').textContent = data.update ? 'Update: ' + data.version : 'Up to date';
        if (data.update) {
          fetch('/ota/update').then(function(response) {
            return response.json();
          }).then(function(result) {
            document.getElementById('otaStatus').textContent = result.status;
          });
        }
      }).catch(function() {
        document.getElementById('otaStatus').textContent = 'Error';
      });
    }

    function loadLogInterval() {
      fetch('/mock/api').then(function(r) { return r.json(); }).then(function(d) {
        if (d.logInterval) {
          logInterval = parseInt(d.logInterval, 10);
          startCountdown();
          if (window.updateDataInterval) clearInterval(window.updateDataInterval);
          window.updateDataInterval = setInterval(updateData, logInterval);
        }
      }).catch(function(e) { console.error('Load log interval error:', e); });
    }

    async function bootApp() {
      initDevices();
      initCharts();
      window.addEventListener('resize', function() {
        updateData();
      });
      await initLogStorage();
      await updateData();
      setTimeout(function() {
        if (document.getElementById('temp').textContent === '--°C') {
          updateData();
        }
      }, 2000);
      loadLogInterval();
    }

    bootApp();
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
  <script src="https://cdn.jsdelivr.net/npm/dexie@3.2.7/dist/dexie.min.js"></script>
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
        <label>Stage:</label>
        <select id="incubationStage" onchange="saveIncubationStage()">
          <option value="incubation">Incubation (Days 1-18)</option>
          <option value="lockdown">Lockdown (Days 19-21)</option>
        </select>
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
      <div class="sys-row"><span class="sys-label">Uptime</span><span class="sys-val" id="sysUptime">--</span></div>
      <div class="sys-row"><span class="sys-label">RAM Records</span><span class="sys-val" id="sysRamLogCnt">--</span></div>
      <div class="sys-row"><span class="sys-label">RAM Storage</span><span class="sys-val" id="sysRamLogBytes">--</span></div>
      <div class="sys-row"><span class="sys-label">Flash Sector</span><span class="sys-val" id="sysFlashSector">--</span></div>
      <div class="sys-row"><span class="sys-label">Flash Records</span><span class="sys-val" id="sysSectorsUsed">--</span></div>
      <div class="sys-row"><span class="sys-label">Local Dexie</span><span class="sys-val" id="sysDexieCnt">--</span></div>
    </div>
    <div class="footer">EGGubator v<span id="version">--</span></div>
  </div>
   <script>
     let userEditing = false;
     window.db = null;
     let logStorageReady = false;

     function initMockLogStorage() {
         if (!window.indexedDB) {
             return;
         }
         if (!window.Dexie) {
             return;
         }
         
         try {
             window.db = new Dexie('EggubatorDB');
             window.db.version(1).stores({
                 logs: '[bootId+t], timestamp'
             });
             return window.db.open();
         } catch (error) {
             console.error('Dexie initialization error:', error);
             window.db = null;
         }
     }
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
        
        document.getElementById('sysUptime').textContent = d.uptime || '--';
        document.getElementById('sysRamLogCnt').textContent = (d.ramLogCnt || 0) + ' records';
        document.getElementById('sysRamLogBytes').textContent = (d.ramLogBytes || 0) + ' bytes';
        document.getElementById('sysFlashSector').textContent = d.flashSector || 0;
        document.getElementById('sysSectorsUsed').textContent = (d.sectorsUsed || 0) + ' sectors';
        
        if (window.db) {
          window.db.logs.count().then(cnt => {
            document.getElementById('sysDexieCnt').textContent = cnt + ' records';
          }).catch(() => {
            document.getElementById('sysDexieCnt').textContent = '--';
          });
        }
        
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
    initMockLogStorage().then(() => {
      setInterval(updateData, 3000);
      loadMockValues();
      updateData();
    }).catch(err => {
      console.error('Failed to init storage:', err);
      // Continue anyway - we can still show live data
      setInterval(updateData, 3000);
      loadMockValues();
      updateData();
    });
  </script>
</body>
</html>
)webui";

#endif
