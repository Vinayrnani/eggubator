#ifndef WEB_UI_H
#define WEB_UI_H

#ifndef DEXIE_ASSET_URL
#define DEXIE_ASSET_URL "/vendor/dexie-3.2.7.min.js"
#endif

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
      <div class="alert" id="syncStatus" style="display:none; text-align:center; background:#2196F3;">Syncing logs...</div>
      <div class="alert" id="storageAlert" style="display:none; text-align:center; background:#f44336;"></div>
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
)webui"
"<script src=\"" DEXIE_ASSET_URL "\"></script>\n"
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
    let db = null;
    let logStorageReady = false;
    let isSyncing = false;
    let updateInFlight = false;
    let syncQueue = [];
    let syncBootId = null;
    let currentTargetTemp = 37.5;
    let currentTargetHumidity = 60.0;
    let lastLiveLogs = [];

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
        db = new Dexie(DB_NAME);
        db.version(1).stores({
          logs: LOG_STORE_SCHEMA
        });
        await db.open();
        logStorageReady = true;
        hideBanner('storageAlert');
      } catch (error) {
        console.error('Dexie initialization error:', error);
        db = null;
        logStorageReady = false;
        showStorageAlert('Local log storage is unavailable. Showing live RAM logs only.');
      }
    }

    function getSyncKey(bootId) {
      return 'lastSyncedSector:' + String(bootId || 0);
    }

    async function cleanupOldLogs() {
      if (!logStorageReady || !db) return;
      const thirtyDaysAgo = Date.now() - (30 * 24 * 3600 * 1000);
      try {
        await db.logs.where('timestamp').below(thirtyDaysAgo).delete();
      } catch (error) {
        console.error('Cleanup error:', error);
      }
    }

    async function syncLogs(bootId, currentSector, absBaseTime) {
      if (!logStorageReady || !db || isSyncing) return;

      if (syncBootId !== bootId) {
        syncBootId = bootId;
        syncQueue = [];
      }

      const syncKey = getSyncKey(bootId);
      let lastSynced = safeGetStorage(syncKey);
      if (lastSynced === null) {
        lastSynced = currentSector;
      } else {
        lastSynced = parseInt(lastSynced, 10);
        if (isNaN(lastSynced)) {
          lastSynced = currentSector;
        }
      }

      if (syncQueue.length === 0) {
        let sector = (lastSynced + 1) % 256;
        while (sector !== currentSector) {
          syncQueue.push(sector);
          sector = (sector + 1) % 256;
        }
      }

      if (syncQueue.length === 0) {
        hideBanner('syncStatus');
        return;
      }

      isSyncing = true;
      showBanner('syncStatus', 'Syncing logs...', '#2196F3');

      while (syncQueue.length > 0) {
        const sectorToSync = syncQueue[0];
        showBanner('syncStatus', 'Syncing... (' + syncQueue.length + ' chunks left)', '#2196F3');
        try {
          const response = await fetch('/data?sector=' + sectorToSync);
          const data = await response.json();
          if (data && data.length > 0) {
            const records = data.map(function(point) {
              return normalizeLogRecord(point, bootId, absBaseTime);
            });
            await db.logs.bulkPut(records);
          }
          safeSetStorage(syncKey, String(sectorToSync));
          syncQueue.shift();
        } catch (error) {
          console.error('Sync error for sector', sectorToSync, error);
          break;
        }
      }

      isSyncing = false;
      if (syncQueue.length === 0) {
        hideBanner('syncStatus');
      } else {
        showBanner('syncStatus', 'Sync paused. Retrying...', '#2196F3');
      }
    }

    async function loadLogsForCharts() {
      if (!logStorageReady || !db) {
        return lastLiveLogs;
      }
      try {
        const logs = await db.logs.orderBy('timestamp').toArray();
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

        if (logStorageReady && db && lastLiveLogs.length > 0) {
          await db.logs.bulkPut(lastLiveLogs);
        }

        if (data.bootId !== undefined && data.currentSector !== undefined) {
          await syncLogs(data.bootId, data.currentSector, absBaseTime);
          await cleanupOldLogs();
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

    function drawTempChart(logData) {
      const canvas = document.getElementById('tempChart');
      const ctx = canvas.getContext('2d');
      const width = canvas.width = canvas.offsetWidth;
      const height = canvas.height = 120;
      const padL = 35, padR = 10, padT = 10, padB = 25;
      const plotW = width - padL - padR;
      const plotH = height - padT - padB;
      ctx.clearRect(0, 0, width, height);
      if (!logData || logData.length < 1) return;

      let minT = 50;
      let maxT = 0;
      logData.forEach(function(point) {
        if (point.temp > maxT) maxT = point.temp;
        if (point.temp < minT) minT = point.temp;
      });
      if (maxT - minT < 2) {
        minT = 35;
        maxT = 40;
      }
      minT = Math.floor(minT - 1);
      maxT = Math.ceil(maxT + 1);

      const times = logData.map(function(point) { return point.t; });
      const minTime = times[0];
      const maxTime = times[times.length - 1];
      const timeRange = maxTime - minTime || 1;

      ctx.fillStyle = '#666';
      ctx.font = '9px Arial';
      ctx.textAlign = 'right';
      const ySteps = 4;
      for (let i = 0; i <= ySteps; i++) {
        const value = minT + (maxT - minT) * (ySteps - i) / ySteps;
        const y = padT + (i / ySteps) * plotH;
        ctx.fillText(value.toFixed(1), padL - 4, y + 3);
        ctx.beginPath();
        ctx.strokeStyle = '#eee';
        ctx.lineWidth = 1;
        ctx.moveTo(padL, y);
        ctx.lineTo(padL + plotW, y);
        ctx.stroke();
      }

      ctx.strokeStyle = '#ff5722';
      ctx.lineWidth = 2;
      ctx.beginPath();
      logData.forEach(function(point, index) {
        const x = padL + ((point.t - minTime) / timeRange) * plotW;
        const y = padT + ((maxT - point.temp) / (maxT - minT)) * plotH;
        if (index === 0) {
          ctx.moveTo(x, y);
        } else {
          ctx.lineTo(x, y);
        }
      });
      ctx.stroke();

      ctx.strokeStyle = '#4CAF50';
      ctx.setLineDash([4, 4]);
      const targetY = padT + ((maxT - currentTargetTemp) / (maxT - minT)) * plotH;
      ctx.beginPath();
      ctx.moveTo(padL, targetY);
      ctx.lineTo(padL + plotW, targetY);
      ctx.stroke();
      ctx.setLineDash([]);

      ctx.fillStyle = '#999';
      ctx.font = '9px Arial';
      ctx.textAlign = 'center';
      const xLabels = 5;
      for (let i = 0; i <= xLabels; i++) {
        const t = minTime + (timeRange * i / xLabels);
        const x = padL + (i / xLabels) * plotW;
        ctx.fillText(formatTimeLabel(t - minTime), x, height - 5);
      }

      document.getElementById('tempTime').textContent = formatTime(timeRange);
    }

    function drawHumChart(logData) {
      const canvas = document.getElementById('humChart');
      const ctx = canvas.getContext('2d');
      const width = canvas.width = canvas.offsetWidth;
      const height = canvas.height = 120;
      const padL = 35, padR = 10, padT = 10, padB = 25;
      const plotW = width - padL - padR;
      const plotH = height - padT - padB;
      ctx.clearRect(0, 0, width, height);
      if (!logData || logData.length < 1) return;

      let minH = 100;
      let maxH = 0;
      logData.forEach(function(point) {
        if (point.hum > maxH) maxH = point.hum;
        if (point.hum < minH) minH = point.hum;
      });
      if (maxH - minH < 5) {
        minH = 40;
        maxH = 80;
      }
      minH = Math.floor(minH - 5);
      maxH = Math.ceil(maxH + 5);

      const times = logData.map(function(point) { return point.t; });
      const minTime = times[0];
      const maxTime = times[times.length - 1];
      const timeRange = maxTime - minTime || 1;

      ctx.fillStyle = '#666';
      ctx.font = '9px Arial';
      ctx.textAlign = 'right';
      const ySteps = 4;
      for (let i = 0; i <= ySteps; i++) {
        const value = minH + (maxH - minH) * (ySteps - i) / ySteps;
        const y = padT + (i / ySteps) * plotH;
        ctx.fillText(value.toFixed(0), padL - 4, y + 3);
        ctx.beginPath();
        ctx.strokeStyle = '#eee';
        ctx.lineWidth = 1;
        ctx.moveTo(padL, y);
        ctx.lineTo(padL + plotW, y);
        ctx.stroke();
      }

      ctx.strokeStyle = '#2196F3';
      ctx.lineWidth = 2;
      ctx.beginPath();
      logData.forEach(function(point, index) {
        const x = padL + ((point.t - minTime) / timeRange) * plotW;
        const y = padT + ((maxH - point.hum) / (maxH - minH)) * plotH;
        if (index === 0) {
          ctx.moveTo(x, y);
        } else {
          ctx.lineTo(x, y);
        }
      });
      ctx.stroke();

      ctx.strokeStyle = '#4CAF50';
      ctx.setLineDash([4, 4]);
      const targetY = padT + ((maxH - currentTargetHumidity) / (maxH - minH)) * plotH;
      ctx.beginPath();
      ctx.moveTo(padL, targetY);
      ctx.lineTo(padL + plotW, targetY);
      ctx.stroke();
      ctx.setLineDash([]);

      ctx.fillStyle = '#999';
      ctx.font = '9px Arial';
      ctx.textAlign = 'center';
      const xLabels = 5;
      for (let i = 0; i <= xLabels; i++) {
        const t = minTime + (timeRange * i / xLabels);
        const x = padL + (i / xLabels) * plotW;
        ctx.fillText(formatTimeLabel(t - minTime), x, height - 5);
      }

      document.getElementById('humTime').textContent = formatTime(timeRange);
    }

    function setupPinchZoom(canvasId, key) {
      const canvas = document.getElementById(canvasId);
      let startDist = 0;
      let startScale = 1;
      canvas.addEventListener('touchstart', function(event) {
        event.stopPropagation();
        if (event.touches.length === 2) {
          startDist = Math.hypot(
            event.touches[0].clientX - event.touches[1].clientX,
            event.touches[0].clientY - event.touches[1].clientY
          );
          startScale = zoomState[key].scale;
        }
      }, { passive: true });
      canvas.addEventListener('touchmove', function(event) {
        event.stopPropagation();
        if (event.touches.length === 2) {
          const dist = Math.hypot(
            event.touches[0].clientX - event.touches[1].clientX,
            event.touches[0].clientY - event.touches[1].clientY
          );
          zoomState[key].scale = Math.max(1, Math.min(10, startScale * (dist / startDist)));
          updateData();
        }
      }, { passive: true });
    }

    function drawCtrlChart(logData) {
      const canvas = document.getElementById('ctrlChart');
      const ctx = canvas.getContext('2d');
      const width = canvas.width = canvas.offsetWidth;
      const height = canvas.height = 120;
      const padL = 35, padR = 10, padT = 10, padB = 25;
      const plotW = width - padL - padR;
      const plotH = height - padT - padB;
      ctx.clearRect(0, 0, width, height);
      if (!logData || logData.length < 1) return;

      const times = logData.map(function(point) { return point.t; });
      const minTime = times[0];
      const maxTime = times[times.length - 1];
      const timeRange = maxTime - minTime || 1;
      const scale = zoomState.ctrl.scale;
      const viewW = plotW / scale;
      const offset = zoomState.ctrl.offset * (plotW - viewW);

      const legend = document.getElementById('ctrlLegend');
      legend.innerHTML = ctrlDevices.map(function(device) {
        return '<div class="legend-item"><div class="legend-dot" style="background:' + device.color + '"></div><span>' + device.name + '</span></div>';
      }).join('');

      const rowH = plotH / 4;
      ctrlDevices.forEach(function(device, index) {
        ctx.beginPath();
        ctx.strokeStyle = device.color;
        ctx.lineWidth = 2;
        let started = false;
        logData.forEach(function(point) {
          const x = padL + ((point.t - minTime) / timeRange) * viewW + offset;
          if (x < padL || x > padL + plotW) return;
          const value = point[device.key] ? 1 : 0;
          const y = padT + (index + 0.5) * rowH + (1 - value) * rowH * 0.4;
          if (started) {
            ctx.lineTo(x, y);
          } else {
            ctx.moveTo(x, y);
            started = true;
          }
        });
        ctx.stroke();
      });

      ctx.fillStyle = '#999';
      ctx.font = '9px Arial';
      ctx.textAlign = 'center';
      const xLabels = 5;
      for (let i = 0; i <= xLabels; i++) {
        const t = minTime + (timeRange * i / xLabels);
        const x = padL + (i / xLabels) * plotW;
        ctx.fillText(formatTimeLabel(t - minTime), x, height - 5);
      }

      document.getElementById('ctrlTime').textContent = formatTime(timeRange);
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

    async function bootApp() {
      initDevices();
      setupPinchZoom('tempChart', 'temp');
      setupPinchZoom('humChart', 'hum');
      setupPinchZoom('ctrlChart', 'ctrl');
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
      setInterval(updateData, 2000);
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
)webui";

#endif
