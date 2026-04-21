#ifndef WEB_UI_H
#define WEB_UI_H

#include <Arduino.h>

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>EGGubator</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    * { box-sizing: border-box; }
    body { font-family: 'Segoe UI', Arial, sans-serif; margin: 0; padding: 0; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); min-height: 100vh; }
    .container { max-width: 640px; margin: 0 auto; padding: 15px; }
    .card { background: white; padding: 15px; border-radius: 12px; box-shadow: 0 4px 15px rgba(0,0,0,0.2); margin-bottom: 15px; }
    h1 { color: white; text-align: center; font-size: 28px; margin: 10px 0 15px 0; }
    h1 span { color: #ffd700; }
    .header { display: grid; grid-template-columns: 1fr 1fr; gap: 8px; background: rgba(255,255,255,0.25); padding: 12px 15px; border-radius: 12px; margin-bottom: 15px; }
    .header .uptime-tag { grid-column: span 2; text-align: center; margin-top: 5px; }
    .mode-group { display: flex; align-items: center; gap: 6px; background: white; padding: 8px 12px; border-radius: 8px; box-shadow: 0 1px 3px rgba(0,0,0,0.1); }
    .device-label { font-weight: 600; color: #555; font-size: 11px; min-width: 68px; }
    .switch { position: relative; display: inline-block; width: 52px; height: 26px; }
    .switch input { opacity: 0; width: 0; height: 0; }
    .slider { position: absolute; cursor: pointer; inset: 0; background-color: #e0e0e0; transition: .3s; border-radius: 26px; }
    .slider:before { position: absolute; content: ""; height: 22px; width: 22px; left: 2px; bottom: 2px; background: white; transition: .3s; border-radius: 50%; box-shadow: 0 1px 3px rgba(0,0,0,0.2); }
    input:checked + .slider { background-color: #f44336; }
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
    .device-status { font-size: 18px; font-weight: bold; margin-bottom: 4px; }
    .device-detail { font-size: 11px; color: #777; min-height: 14px; }
    .btn { display: inline-block; text-align: center; text-decoration: none; padding: 10px 14px; border: none; border-radius: 8px; cursor: pointer; font-size: 13px; font-weight: 600; transition: opacity 0.2s; width: 100%; }
    .btn:disabled { opacity: 0.5; cursor: not-allowed; }
    .btn-primary { background: #2196F3; color: white; }
    .btn-secondary { background: #6c757d; color: white; }
    .target-info { text-align: center; color: #555; font-size: 13px; }
    .action-row { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; }
    .chart-wrap { overflow-x: auto; -webkit-overflow-scrolling: touch; }
    canvas { display: block; width: 100%; background: #fafafa; border-radius: 8px; touch-action: pan-x pinch-x; }
    .legend { display: flex; flex-wrap: wrap; justify-content: center; gap: 12px; margin-top: 8px; font-size: 11px; }
    .legend-item { display: flex; align-items: center; gap: 4px; }
    .legend-dot { width: 10px; height: 10px; border-radius: 50%; }
    .time-label { text-align: center; font-size: 10px; color: #999; margin-top: 4px; }
    .alert { background: #2196F3; color: white; padding: 10px; border-radius: 8px; margin-bottom: 15px; display: none; text-align: center; }
    .alert.show { display: block; }
    .version { text-align: center; color: rgba(255,255,255,0.8); font-size: 11px; margin-top: 10px; }
    .uptime-tag { background: rgba(255,255,255,0.3); padding: 6px 15px; border-radius: 20px; color: #333; font-size: 12px; font-weight: 600; }
    .stage-row { display: flex; align-items: center; justify-content: center; gap: 10px; margin-bottom: 12px; }
    .stage-row select { padding: 6px 10px; border: 2px solid #667eea; border-radius: 6px; font-size: 12px; font-weight: 600; color: #333; background: white; min-width: 220px; }
    .stage-note { text-align: center; font-size: 11px; color: #f44336; font-weight: 600; min-height: 14px; margin-bottom: 10px; }
    @media (max-width: 460px) {
      .header, .action-row, .device-grid, .info-grid { grid-template-columns: 1fr; }
      .header .uptime-tag { grid-column: span 1; }
    }
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
        <span class="device-label">Atomizer</span>
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
    <div class="stage-note" id="eggTurnerStatus"></div>
    <div class="alert" id="syncStatus"></div>
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
      <div class="target-info" id="targets">Target: 37.5°C | 55%</div>
    </div>
    <div class="card action-row">
      <a class="btn btn-secondary" href="/mock">Settings &amp; Mock</a>
      <button class="btn btn-primary" onclick="checkOta()">Check Update</button>
    </div>
    <div class="card">
      <div style="text-align:center;font-size:12px;color:#666;" id="otaStatus">Ready</div>
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
    var devices = [
      { id: 'heater', name: 'Heater' },
      { id: 'atomizer', name: 'Atomizer' },
      { id: 'fan', name: 'Fan' },
      { id: 'servo', name: 'Egg Turner' }
    ];
    var ctrlDevices = [
      { key: 'h', name: 'Heater', color: '#f44336' },
      { key: 'a', name: 'Atomizer', color: '#2196F3' },
      { key: 'f', name: 'Fan', color: '#4CAF50' },
      { key: 's', name: 'Turner', color: '#FF9800' }
    ];
    var zoomState = {
      temp: { scale: 1 },
      hum: { scale: 1 },
      ctrl: { scale: 1 }
    };
    var chartLogs = [];
    var currentTargets = { temp: 37.5, hum: 55 };
    var refreshInFlight = false;
    var syncInProgress = false;
    var memoryStore = { logs: {}, meta: {} };

    var store = {
      db: null,
      ready: null,
      useMemory: !window.indexedDB,
      open: function() {
        var self = this;
        if (self.useMemory) {
          return Promise.resolve(null);
        }
        if (self.ready) {
          return self.ready;
        }
        self.ready = new Promise(function(resolve, reject) {
          var request = indexedDB.open('EggubatorDB', 1);
          request.onupgradeneeded = function(event) {
            var db = event.target.result;
            if (!db.objectStoreNames.contains('logs')) {
              var logs = db.createObjectStore('logs', { keyPath: 'id' });
              logs.createIndex('byTimestamp', 'timestamp', { unique: false });
            }
            if (!db.objectStoreNames.contains('meta')) {
              db.createObjectStore('meta', { keyPath: 'key' });
            }
          };
          request.onsuccess = function() {
            self.db = request.result;
            resolve(self.db);
          };
          request.onerror = function() {
            reject(request.error || new Error('IndexedDB open failed'));
          };
        });
        return self.ready;
      },
      putLogs: function(records) {
        if (!records || !records.length) {
          return Promise.resolve();
        }
        if (this.useMemory) {
          for (var i = 0; i < records.length; i++) {
            memoryStore.logs[records[i].id] = records[i];
          }
          return Promise.resolve();
        }
        return this.open().then(function(db) {
          return new Promise(function(resolve, reject) {
            var tx = db.transaction(['logs'], 'readwrite');
            var logs = tx.objectStore('logs');
            for (var i = 0; i < records.length; i++) {
              logs.put(records[i]);
            }
            tx.oncomplete = function() { resolve(); };
            tx.onerror = function() { reject(tx.error || new Error('Log write failed')); };
            tx.onabort = function() { reject(tx.error || new Error('Log write aborted')); };
          });
        });
      },
      getAllLogs: function() {
        if (this.useMemory) {
          var values = [];
          for (var key in memoryStore.logs) {
            if (memoryStore.logs.hasOwnProperty(key)) {
              values.push(memoryStore.logs[key]);
            }
          }
          values.sort(function(a, b) { return a.timestamp - b.timestamp; });
          return Promise.resolve(values);
        }
        return this.open().then(function(db) {
          return new Promise(function(resolve, reject) {
            var values = [];
            var request = db.transaction(['logs'], 'readonly').objectStore('logs').index('byTimestamp').openCursor();
            request.onsuccess = function(event) {
              var cursor = event.target.result;
              if (cursor) {
                values.push(cursor.value);
                cursor.continue();
              } else {
                resolve(values);
              }
            };
            request.onerror = function() {
              reject(request.error || new Error('Log read failed'));
            };
          });
        });
      },
      clearLogs: function() {
        if (this.useMemory) {
          memoryStore.logs = {};
          return Promise.resolve();
        }
        return this.open().then(function(db) {
          return new Promise(function(resolve, reject) {
            var request = db.transaction(['logs'], 'readwrite').objectStore('logs').clear();
            request.onsuccess = function() { resolve(); };
            request.onerror = function() { reject(request.error || new Error('Log clear failed')); };
          });
        });
      },
      getMeta: function(key) {
        if (this.useMemory) {
          return Promise.resolve(memoryStore.meta[key] || null);
        }
        return this.open().then(function(db) {
          return new Promise(function(resolve, reject) {
            var request = db.transaction(['meta'], 'readonly').objectStore('meta').get(key);
            request.onsuccess = function() {
              resolve(request.result ? request.result.value : null);
            };
            request.onerror = function() {
              reject(request.error || new Error('Meta read failed'));
            };
          });
        });
      },
      setMeta: function(key, value) {
        if (this.useMemory) {
          memoryStore.meta[key] = value;
          return Promise.resolve();
        }
        return this.open().then(function(db) {
          return new Promise(function(resolve, reject) {
            var request = db.transaction(['meta'], 'readwrite').objectStore('meta').put({ key: key, value: value });
            request.onsuccess = function() { resolve(); };
            request.onerror = function() { reject(request.error || new Error('Meta write failed')); };
          });
        });
      },
      pruneLogs: function(currentBootId, cutoff) {
        if (this.useMemory) {
          for (var key in memoryStore.logs) {
            if (memoryStore.logs.hasOwnProperty(key)) {
              var record = memoryStore.logs[key];
              if (record.bootId !== currentBootId || record.timestamp < cutoff) {
                delete memoryStore.logs[key];
              }
            }
          }
          return Promise.resolve();
        }
        return this.open().then(function(db) {
          return new Promise(function(resolve, reject) {
            var tx = db.transaction(['logs'], 'readwrite');
            var request = tx.objectStore('logs').openCursor();
            request.onsuccess = function(event) {
              var cursor = event.target.result;
              if (cursor) {
                var record = cursor.value;
                if (record.bootId !== currentBootId || record.timestamp < cutoff) {
                  cursor.delete();
                }
                cursor.continue();
              }
            };
            request.onerror = function() {
              reject(request.error || new Error('Log prune failed'));
            };
            tx.oncomplete = function() { resolve(); };
            tx.onerror = function() { reject(tx.error || new Error('Log prune transaction failed')); };
          });
        });
      }
    };

    function toBool(value) {
      return value === true || value === 'true' || value === 1 || value === '1';
    }

    function toServoPos(value) {
      var parsed = parseInt(value, 10);
      if (isNaN(parsed)) return 0;
      if (parsed > 1) return 1;
      if (parsed < -1) return -1;
      return parsed;
    }

    function servoPositionLabel(pos) {
      if (pos > 0) return 'RIGHT';
      if (pos < 0) return 'LEFT';
      return 'CENTER';
    }

    function formatTime(ms) {
      if (ms < 60000) return Math.round(ms / 1000) + 's';
      if (ms < 3600000) return Math.round(ms / 60000) + 'm';
      if (ms < 86400000) return Math.round(ms / 3600000) + 'h';
      return Math.round(ms / 86400000) + 'd';
    }

    function formatTimeLabel(ms) {
      if (ms < 60000) return Math.round(ms / 1000) + 's';
      if (ms < 3600000) return Math.round(ms / 60000) + 'm';
      if (ms < 86400000) return Math.round(ms / 3600000) + 'h';
      return Math.round(ms / 86400000) + 'd';
    }

    function buildLogRecord(bootId, baseTime, entry) {
      return {
        id: String(bootId) + ':' + String(entry.t),
        bootId: bootId,
        t: Number(entry.t),
        timestamp: baseTime + Number(entry.t),
        temp: Number(entry.temp),
        hum: Number(entry.hum),
        h: toBool(entry.h),
        a: toBool(entry.a),
        f: toBool(entry.f),
        s: toServoPos(entry.s)
      };
    }

    function ensureSessionState(data) {
      return store.getMeta('session').then(function(session) {
        if (!session || session.bootId !== data.bootId) {
          return store.clearLogs().then(function() {
            var nextSession = {
              bootId: data.bootId || 0,
              baseTime: Date.now() - (data.uptime_ms || 0),
              lastSyncedSector: null,
              bootStartSector: data.bootStartSector,
              lastCleanupAt: 0
            };
            return store.setMeta('session', nextSession).then(function() {
              return nextSession;
            });
          });
        }
        if (typeof session.baseTime !== 'number') {
          session.baseTime = Date.now() - (data.uptime_ms || 0);
        }
        session.bootStartSector = data.bootStartSector;
        return store.setMeta('session', session).then(function() {
          return session;
        });
      });
    }

    function ingestRamLogs(data, session) {
      if (!data.log || !data.log.length) {
        return Promise.resolve();
      }
      var records = [];
      for (var i = 0; i < data.log.length; i++) {
        records.push(buildLogRecord(data.bootId || 0, session.baseTime, data.log[i]));
      }
      return store.putLogs(records);
    }

    function cleanupLogs(session) {
      var now = Date.now();
      if (session.lastCleanupAt && (now - session.lastCleanupAt) < 3600000) {
        return Promise.resolve();
      }
      return store.pruneLogs(session.bootId, now - (7 * 24 * 3600 * 1000)).then(function() {
        session.lastCleanupAt = now;
        return store.setMeta('session', session);
      });
    }

    function buildSectorQueue(session, currentSector) {
      var queue = [];
      var startSector = session.lastSyncedSector === null || session.lastSyncedSector === undefined
        ? session.bootStartSector
        : (session.lastSyncedSector + 1) % 256;
      var sector = startSector;
      while (sector !== currentSector && queue.length < 256) {
        queue.push(sector);
        sector = (sector + 1) % 256;
      }
      return queue;
    }

    function setSyncStatus(message, visible) {
      var el = document.getElementById('syncStatus');
      el.textContent = message || '';
      el.className = visible ? 'alert show' : 'alert';
      el.style.display = visible ? 'block' : 'none';
    }

    function syncFlashLogs(data, session) {
      var queue = buildSectorQueue(session, data.currentSector);
      if (syncInProgress || !queue.length) {
        if (!queue.length) {
          setSyncStatus('', false);
        }
        return Promise.resolve();
      }
      syncInProgress = true;
      setSyncStatus('Syncing log history...', true);
      var chain = Promise.resolve();
      queue.forEach(function(sector, index) {
        chain = chain.then(function() {
          setSyncStatus('Syncing log history... ' + (index + 1) + '/' + queue.length, true);
          return fetch('/data?sector=' + sector).then(function(response) {
            return response.json();
          }).then(function(entries) {
            if (!entries || !entries.length) {
              return null;
            }
            var records = [];
            for (var i = 0; i < entries.length; i++) {
              records.push(buildLogRecord(data.bootId || 0, session.baseTime, entries[i]));
            }
            return store.putLogs(records);
          }).then(function() {
            session.lastSyncedSector = sector;
            return store.setMeta('session', session);
          });
        });
      });
      return chain.catch(function(error) {
        console.error('Flash sync error:', error);
      }).then(function() {
        syncInProgress = false;
        setSyncStatus('', false);
      });
    }

    function initDevices() {
      var grid = document.getElementById('deviceGrid');
      var html = '';
      for (var i = 0; i < devices.length; i++) {
        var device = devices[i];
        html += '<div class="device-card">' +
          '<div class="device-name">' + device.name + '</div>' +
          '<div class="device-status" id="' + device.id + '">--</div>' +
          '<div class="device-detail" id="' + device.id + 'Detail"></div>' +
          '</div>';
      }
      grid.innerHTML = html;
      var legend = [];
      for (var j = 0; j < ctrlDevices.length; j++) {
        legend.push('<div class="legend-item"><div class="legend-dot" style="background:' + ctrlDevices[j].color + '"></div><span>' + ctrlDevices[j].name + '</span></div>');
      }
      document.getElementById('ctrlLegend').innerHTML = legend.join('');
    }

    function updateModeControl(device, mode) {
      var text = document.getElementById(device + 'ModeText');
      var toggle = document.getElementById(device + 'ModeSwitch');
      var isOff = Number(mode) === 0;
      text.textContent = isOff ? 'OFF' : 'AUTO';
      text.className = 'mode-text ' + (isOff ? 'killed' : 'auto');
      toggle.checked = isOff;
    }

    function setDeviceCard(id, status, isOn, detail) {
      var statusEl = document.getElementById(id);
      var detailEl = document.getElementById(id + 'Detail');
      statusEl.textContent = status;
      statusEl.className = 'device-status ' + (isOn ? 'on' : 'off');
      detailEl.textContent = detail || '';
    }

    function updateLiveUi(data) {
      currentTargets.temp = Number(data.targetTemp || currentTargets.temp);
      currentTargets.hum = Number(data.targetHumidity || currentTargets.hum);
      document.getElementById('version').textContent = data.version || '--';
      document.getElementById('temp').textContent = Number(data.temperature || 0).toFixed(1) + '°C';
      document.getElementById('hum').textContent = Number(data.humidity || 0).toFixed(1) + '%';
      document.getElementById('uptime').textContent = data.uptime || '--';
      document.getElementById('targets').textContent = 'Target: ' + currentTargets.temp.toFixed(1) + '°C | ' + currentTargets.hum.toFixed(0) + '%';
      document.getElementById('mainStageSelect').value = data.stageLockdown ? 'lockdown' : 'incubation';

      updateModeControl('heater', data.heaterMode);
      updateModeControl('atomizer', data.atomizerMode);
      updateModeControl('fan', data.fanMode);
      updateModeControl('servo', data.servoMode);

      setDeviceCard('heater', data.heater ? 'ON' : 'OFF', toBool(data.heater), Number(data.heaterMode) === 0 ? 'Manual OFF' : (data.heater ? 'Heating' : 'Idle'));
      setDeviceCard('atomizer', data.atomizer ? 'ON' : 'OFF', toBool(data.atomizer), Number(data.atomizerMode) === 0 ? 'Manual OFF' : (data.atomizer ? 'Pulsing humidity' : 'Idle'));
      setDeviceCard('fan', data.fan ? 'ON' : 'OFF', toBool(data.fan), Number(data.fanMode) === 0 ? 'Manual OFF' : (data.fan ? 'Ventilating' : 'Idle'));

      if (data.stageLockdown) {
        setDeviceCard('servo', 'LOCKDOWN', false, 'Disabled during lockdown');
        document.getElementById('eggTurnerStatus').textContent = 'Egg turner is disabled during lockdown.';
      } else if (Number(data.servoMode) === 0) {
        setDeviceCard('servo', 'OFF', false, 'Centered');
        document.getElementById('eggTurnerStatus').textContent = 'Egg turner is manually OFF.';
      } else {
        var servoDetail = data.servoTurning ? 'Moving • ' + servoPositionLabel(data.servoPosition) : 'Resting • ' + servoPositionLabel(data.servoPosition);
        setDeviceCard('servo', data.servoTurning ? 'TURNING' : 'AUTO', true, servoDetail);
        document.getElementById('eggTurnerStatus').textContent = 'Egg turner position: ' + servoPositionLabel(data.servoPosition) + '.';
      }
    }

    function clearChart(canvasId, timeLabelId) {
      var canvas = document.getElementById(canvasId);
      var ctx = canvas.getContext('2d');
      canvas.width = canvas.offsetWidth || 400;
      canvas.height = 120;
      ctx.clearRect(0, 0, canvas.width, canvas.height);
      document.getElementById(timeLabelId).textContent = '--';
    }

    function drawTempChart(logData) {
      var canvas = document.getElementById('tempChart');
      var ctx = canvas.getContext('2d');
      var width = canvas.width = canvas.offsetWidth || 400;
      var height = canvas.height = 120;
      var padL = 35, padR = 10, padT = 10, padB = 25;
      var plotW = width - padL - padR;
      var plotH = height - padT - padB;
      ctx.clearRect(0, 0, width, height);
      if (!logData || !logData.length) {
        document.getElementById('tempTime').textContent = '--';
        return;
      }

      var minT = currentTargets.temp - 2;
      var maxT = currentTargets.temp + 2;
      for (var i = 0; i < logData.length; i++) {
        if (logData[i].temp < minT) minT = logData[i].temp;
        if (logData[i].temp > maxT) maxT = logData[i].temp;
      }
      minT = Math.floor(minT - 1);
      maxT = Math.ceil(maxT + 1);
      if (maxT <= minT) maxT = minT + 2;

      var minTime = logData[0].t;
      var maxTime = logData[logData.length - 1].t;
      var timeRange = maxTime - minTime || 1;
      var scale = zoomState.temp.scale;
      var viewW = plotW / scale;
      var offset = plotW - viewW;

      ctx.fillStyle = '#666';
      ctx.font = '9px Arial';
      ctx.textAlign = 'right';
      for (var step = 0; step <= 4; step++) {
        var val = minT + (maxT - minT) * (4 - step) / 4;
        var y = padT + (step / 4) * plotH;
        ctx.fillText(val.toFixed(1), padL - 4, y + 3);
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
      for (var p = 0; p < logData.length; p++) {
        var x = padL + ((logData[p].t - minTime) / timeRange) * viewW + offset;
        var yPos = padT + ((maxT - logData[p].temp) / (maxT - minT)) * plotH;
        if (p === 0) ctx.moveTo(x, yPos); else ctx.lineTo(x, yPos);
      }
      ctx.stroke();

      ctx.strokeStyle = '#4CAF50';
      ctx.setLineDash([4, 4]);
      var targetY = padT + ((maxT - currentTargets.temp) / (maxT - minT)) * plotH;
      ctx.beginPath();
      ctx.moveTo(padL, targetY);
      ctx.lineTo(padL + plotW, targetY);
      ctx.stroke();
      ctx.setLineDash([]);

      ctx.fillStyle = '#999';
      ctx.textAlign = 'center';
      for (var label = 0; label <= 5; label++) {
        var t = minTime + (timeRange * label / 5);
        var xLabel = padL + (label / 5) * plotW;
        ctx.fillText(formatTimeLabel(t - minTime), xLabel, height - 5);
      }
      document.getElementById('tempTime').textContent = formatTime(timeRange);
    }

    function drawHumChart(logData) {
      var canvas = document.getElementById('humChart');
      var ctx = canvas.getContext('2d');
      var width = canvas.width = canvas.offsetWidth || 400;
      var height = canvas.height = 120;
      var padL = 35, padR = 10, padT = 10, padB = 25;
      var plotW = width - padL - padR;
      var plotH = height - padT - padB;
      ctx.clearRect(0, 0, width, height);
      if (!logData || !logData.length) {
        document.getElementById('humTime').textContent = '--';
        return;
      }

      var minH = currentTargets.hum - 10;
      var maxH = currentTargets.hum + 10;
      for (var i = 0; i < logData.length; i++) {
        if (logData[i].hum < minH) minH = logData[i].hum;
        if (logData[i].hum > maxH) maxH = logData[i].hum;
      }
      minH = Math.max(0, Math.floor(minH - 5));
      maxH = Math.min(100, Math.ceil(maxH + 5));
      if (maxH <= minH) maxH = minH + 10;

      var minTime = logData[0].t;
      var maxTime = logData[logData.length - 1].t;
      var timeRange = maxTime - minTime || 1;
      var scale = zoomState.hum.scale;
      var viewW = plotW / scale;
      var offset = plotW - viewW;

      ctx.fillStyle = '#666';
      ctx.font = '9px Arial';
      ctx.textAlign = 'right';
      for (var step = 0; step <= 4; step++) {
        var val = minH + (maxH - minH) * (4 - step) / 4;
        var y = padT + (step / 4) * plotH;
        ctx.fillText(val.toFixed(0), padL - 4, y + 3);
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
      for (var p = 0; p < logData.length; p++) {
        var x = padL + ((logData[p].t - minTime) / timeRange) * viewW + offset;
        var yPos = padT + ((maxH - logData[p].hum) / (maxH - minH)) * plotH;
        if (p === 0) ctx.moveTo(x, yPos); else ctx.lineTo(x, yPos);
      }
      ctx.stroke();

      ctx.strokeStyle = '#4CAF50';
      ctx.setLineDash([4, 4]);
      var targetY = padT + ((maxH - currentTargets.hum) / (maxH - minH)) * plotH;
      ctx.beginPath();
      ctx.moveTo(padL, targetY);
      ctx.lineTo(padL + plotW, targetY);
      ctx.stroke();
      ctx.setLineDash([]);

      ctx.fillStyle = '#999';
      ctx.textAlign = 'center';
      for (var label = 0; label <= 5; label++) {
        var t = minTime + (timeRange * label / 5);
        var xLabel = padL + (label / 5) * plotW;
        ctx.fillText(formatTimeLabel(t - minTime), xLabel, height - 5);
      }
      document.getElementById('humTime').textContent = formatTime(timeRange);
    }

    function drawCtrlChart(logData) {
      var canvas = document.getElementById('ctrlChart');
      var ctx = canvas.getContext('2d');
      var width = canvas.width = canvas.offsetWidth || 400;
      var height = canvas.height = 120;
      var padL = 35, padR = 10, padT = 10, padB = 25;
      var plotW = width - padL - padR;
      var plotH = height - padT - padB;
      ctx.clearRect(0, 0, width, height);
      if (!logData || !logData.length) {
        document.getElementById('ctrlTime').textContent = '--';
        return;
      }

      var minTime = logData[0].t;
      var maxTime = logData[logData.length - 1].t;
      var timeRange = maxTime - minTime || 1;
      var scale = zoomState.ctrl.scale;
      var viewW = plotW / scale;
      var offset = plotW - viewW;
      var rowH = plotH / ctrlDevices.length;

      for (var i = 0; i < ctrlDevices.length; i++) {
        var device = ctrlDevices[i];
        var rowCenter = padT + (i + 0.5) * rowH;
        ctx.beginPath();
        ctx.strokeStyle = device.color;
        ctx.lineWidth = 2;
        for (var j = 0; j < logData.length; j++) {
          var x = padL + ((logData[j].t - minTime) / timeRange) * viewW + offset;
          var y;
          if (device.key === 's') {
            y = rowCenter - (toServoPos(logData[j].s) * rowH * 0.28);
          } else {
            var value = logData[j][device.key] ? 1 : 0;
            y = rowCenter + (value ? -1 : 1) * rowH * 0.28;
          }
          if (j === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
        }
        ctx.stroke();
      }

      ctx.fillStyle = '#999';
      ctx.font = '9px Arial';
      ctx.textAlign = 'center';
      for (var label = 0; label <= 5; label++) {
        var t = minTime + (timeRange * label / 5);
        var xLabel = padL + (label / 5) * plotW;
        ctx.fillText(formatTimeLabel(t - minTime), xLabel, height - 5);
      }
      document.getElementById('ctrlTime').textContent = formatTime(timeRange);
    }

    function renderCharts(logs) {
      chartLogs = logs || [];
      if (!chartLogs.length) {
        clearChart('tempChart', 'tempTime');
        clearChart('humChart', 'humTime');
        clearChart('ctrlChart', 'ctrlTime');
        return;
      }
      drawTempChart(chartLogs);
      drawHumChart(chartLogs);
      drawCtrlChart(chartLogs);
    }

    function redrawCharts() {
      return store.getAllLogs().then(function(logs) {
        renderCharts(logs);
      });
    }

    function updateData() {
      if (refreshInFlight) {
        return;
      }
      refreshInFlight = true;
      fetch('/data').then(function(response) {
        return response.json();
      }).then(function(data) {
        updateLiveUi(data);
        return ensureSessionState(data).then(function(session) {
          return ingestRamLogs(data, session).then(function() {
            return cleanupLogs(session).then(function() {
              return syncFlashLogs(data, session);
            });
          });
        });
      }).then(function() {
        return redrawCharts();
      }).catch(function(error) {
        console.error('Data fetch error:', error);
      }).then(function() {
        refreshInFlight = false;
      });
    }

    function saveMainStage() {
      var stage = document.getElementById('mainStageSelect').value;
      fetch('/mock/api?stageType=' + stage).then(function() {
        updateData();
      }).catch(function(error) {
        console.error(error);
      });
    }

    function toggleDeviceMode(device) {
      var modeSwitch = document.getElementById(device + 'ModeSwitch');
      var mode = modeSwitch.checked ? 'off' : 'auto';
      fetch('/control?device=' + device + '&mode=' + mode).then(function() {
        updateData();
      }).catch(function(error) {
        console.error(error);
      });
    }

    function checkOta() {
      var status = document.getElementById('otaStatus');
      status.textContent = 'Checking for updates...';
      fetch('/ota/check').then(function(response) {
        return response.json();
      }).then(function(data) {
        if (!data.update) {
          status.textContent = 'Firmware is up to date.';
          return null;
        }
        status.textContent = 'Updating to ' + data.version + '...';
        return fetch('/ota/update').then(function(response) {
          return response.json();
        }).then(function(updateResponse) {
          status.textContent = updateResponse.status || 'Update started.';
        });
      }).catch(function() {
        status.textContent = 'Unable to check for updates.';
      });
    }

    function pinchDistance(touchA, touchB) {
      var dx = touchA.clientX - touchB.clientX;
      var dy = touchA.clientY - touchB.clientY;
      return Math.sqrt((dx * dx) + (dy * dy));
    }

    function setupPinchZoom(canvasId, key) {
      var canvas = document.getElementById(canvasId);
      var startDist = 0;
      var startScale = 1;
      canvas.addEventListener('touchstart', function(event) {
        if (event.touches.length === 2) {
          startDist = pinchDistance(event.touches[0], event.touches[1]);
          startScale = zoomState[key].scale;
        }
      }, { passive: true });
      canvas.addEventListener('touchmove', function(event) {
        if (event.touches.length === 2 && startDist > 0) {
          var dist = pinchDistance(event.touches[0], event.touches[1]);
          zoomState[key].scale = Math.max(1, Math.min(8, startScale * (dist / startDist)));
          renderCharts(chartLogs);
        }
      }, { passive: true });
    }

    initDevices();
    setupPinchZoom('tempChart', 'temp');
    setupPinchZoom('humChart', 'hum');
    setupPinchZoom('ctrlChart', 'ctrl');
    window.addEventListener('resize', function() { renderCharts(chartLogs); });
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
    .container { max-width: 440px; margin: 0 auto; padding: 12px; }
    .card { background: rgba(255,255,255,0.96); padding: 16px; border-radius: 16px; box-shadow: 0 8px 32px rgba(0,0,0,0.3); margin-bottom: 12px; }
    .card-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 12px; padding-bottom: 8px; border-bottom: 2px solid #e94560; }
    .card-title { color: #1a1a2e; font-size: 14px; font-weight: 700; text-transform: uppercase; letter-spacing: 1px; }
    h1 { color: white; text-align: center; font-size: 28px; margin: 8px 0 12px 0; text-shadow: 0 2px 4px rgba(0,0,0,0.3); }
    h1 span { color: #ffd700; }
    .header { display: flex; justify-content: space-between; align-items: center; background: rgba(255,255,255,0.15); padding: 10px 16px; border-radius: 12px; margin-bottom: 12px; }
    .back-link { color: white; text-decoration: none; font-size: 14px; font-weight: 600; }
    .input-row { display: flex; align-items: center; justify-content: space-between; margin-bottom: 10px; gap: 10px; }
    .input-row label { color: #333; font-size: 13px; font-weight: 600; }
    select { padding: 8px 12px; border: 2px solid #e94560; border-radius: 8px; width: 170px; font-size: 13px; font-weight: 600; color: #1a1a2e; background: white; }
    .btn { padding: 10px 20px; border: none; border-radius: 8px; cursor: pointer; font-size: 14px; font-weight: 600; width: 100%; }
    .btn-primary { background: linear-gradient(135deg, #e94560 0%, #c53b5a 100%); color: white; box-shadow: 0 4px 15px rgba(233,69,96,0.4); }
    .input-wrapper { display: flex; align-items: center; gap: 6px; }
    .input-wrapper span { color: #666; font-weight: 600; }
    input[type="number"] { padding: 8px 10px; border: 2px solid #ddd; border-radius: 8px; font-size: 14px; font-weight: 600; width: 90px; }
    .toggle-row { display: flex; justify-content: space-between; align-items: center; padding: 10px 0; }
    .toggle-label { font-size: 14px; font-weight: 600; color: #333; }
    .switch { position: relative; display: inline-block; width: 52px; height: 28px; }
    .switch input { opacity: 0; width: 0; height: 0; }
    .slider { position: absolute; cursor: pointer; inset: 0; background-color: #ccc; transition: .3s; border-radius: 28px; }
    .slider:before { position: absolute; content: ""; height: 22px; width: 22px; left: 3px; bottom: 3px; background: white; transition: .3s; border-radius: 50%; box-shadow: 0 2px 4px rgba(0,0,0,0.2); }
    input:checked + .slider { background: linear-gradient(135deg, #e94560 0%, #c53b5a 100%); }
    input:checked + .slider:before { transform: translateX(24px); }
    .status-row { display: flex; justify-content: space-between; padding: 10px 0; border-bottom: 1px solid #eee; }
    .status-row:last-child, .sys-row:last-child { border-bottom: none; }
    .stat { font-size: 18px; font-weight: bold; }
    .on { color: #4CAF50; }
    .off { color: #f44336; }
    .sys-row { display: flex; justify-content: space-between; padding: 8px 0; border-bottom: 1px solid #eee; font-size: 12px; }
    .sys-label { color: #666; }
    .sys-val { font-weight: 700; color: #333; }
    .stage-badge { display: inline-block; padding: 4px 12px; border-radius: 20px; font-size: 12px; font-weight: 700; }
    .stage-incubation { background: #4CAF50; color: white; }
    .stage-lockdown { background: #e94560; color: white; }
    .footer { text-align: center; margin-top: 15px; padding: 10px; color: rgba(255,255,255,0.6); font-size: 11px; }
  </style>
</head>
<body>
  <div class="container">
    <h1><span>🥚</span> EGGubator</h1>
    <div class="header">
      <a href="/" class="back-link">← Dashboard</a>
      <span id="stageBadge" class="stage-badge stage-incubation">INCUBATION</span>
    </div>
    <div class="card">
      <div class="card-header"><span class="card-title">Timing Settings</span></div>
      <div class="input-row">
        <label>Stage</label>
        <select id="incubationStage" onchange="saveIncubationStage()">
          <option value="incubation">Incubation (Days 1-18)</option>
          <option value="lockdown">Lockdown (Days 19-21)</option>
        </select>
      </div>
      <div class="input-row">
        <label>Log Interval</label>
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
        <label>Save Flash</label>
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
        <label>Atomizer Pulse</label>
        <select id="atomizerPulse" onchange="saveAtomizerPulse()">
          <option value="2000">2 sec</option>
          <option value="3000">3 sec</option>
          <option value="4000">4 sec</option>
          <option value="5000">5 sec</option>
        </select>
      </div>
      <div class="input-row">
        <label>Egg Turner</label>
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
      <div class="card-header"><span class="card-title">Simulation</span></div>
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
      <div class="card-header"><span class="card-title">Mock Values</span></div>
      <div class="input-row">
        <label>Temperature</label>
        <div class="input-wrapper"><input type="number" id="mockTemp" value="25" step="0.1" min="20" max="45"><span>°C</span></div>
      </div>
      <div class="input-row">
        <label>Humidity</label>
        <div class="input-wrapper"><input type="number" id="mockHum" value="50" step="0.1" min="0" max="100"><span>%</span></div>
      </div>
      <button class="btn btn-primary" id="setValuesBtn" onclick="setMockValues()">Apply Values</button>
    </div>
    <div class="card">
      <div class="card-header"><span class="card-title">Current Status</span></div>
      <div class="status-row"><span>Sensor</span><span class="stat" id="sensorStatus">Real</span></div>
      <div class="status-row"><span>Temperature</span><span class="stat" id="currentTemp">--°C</span></div>
      <div class="status-row"><span>Humidity</span><span class="stat" id="currentHum">--%</span></div>
      <div class="status-row"><span>Target</span><span class="stat" id="targetStatus">37.5°C / 55%</span></div>
    </div>
    <div class="card">
      <div class="card-header"><span class="card-title">System Info</span></div>
      <div class="sys-row"><span class="sys-label">RAM</span><span class="sys-val" id="sysHeap">--</span></div>
      <div class="sys-row"><span class="sys-label">CPU</span><span class="sys-val" id="sysCpu">--</span></div>
      <div class="sys-row"><span class="sys-label">Uptime</span><span class="sys-val" id="sysUptime">--</span></div>
      <div class="sys-row"><span class="sys-label">Unsaved Log Records</span><span class="sys-val" id="sysLogCnt">--</span></div>
      <div class="sys-row"><span class="sys-label">RAM Log Storage</span><span class="sys-val" id="sysLogStorage">--</span></div>
    </div>
    <div class="footer">EGGubator v<span id="version">--</span></div>
  </div>
  <script>
    function updateStageBadge(lockdown) {
      var badge = document.getElementById('stageBadge');
      if (lockdown) {
        badge.textContent = 'LOCKDOWN';
        badge.className = 'stage-badge stage-lockdown';
      } else {
        badge.textContent = 'INCUBATION';
        badge.className = 'stage-badge stage-incubation';
      }
    }

    function loadSettings() {
      fetch('/mock/api').then(function(response) {
        return response.json();
      }).then(function(data) {
        document.getElementById('mockTemp').value = data.temp;
        document.getElementById('mockHum').value = data.hum;
        document.getElementById('logInterval').value = String(data.logInterval);
        document.getElementById('saveFlashInterval').value = String(data.saveFlashInterval);
        document.getElementById('eggTurnInterval').value = String(data.eggTurnInterval);
        document.getElementById('atomizerPulse').value = String(data.pulseOnTime);
        document.getElementById('incubationStage').value = data.stageType || (data.stageLockdown ? 'lockdown' : 'incubation');
      }).catch(function(error) {
        console.error(error);
      });
    }

    function updateData() {
      fetch('/data').then(function(response) {
        return response.json();
      }).then(function(data) {
        document.getElementById('version').textContent = data.version || '--';
        document.getElementById('currentTemp').textContent = Number(data.temperature || 0).toFixed(1) + '°C';
        document.getElementById('currentHum').textContent = Number(data.humidity || 0).toFixed(1) + '%';
        document.getElementById('targetStatus').textContent = Number(data.targetTemp || 37.5).toFixed(1) + '°C / ' + Number(data.targetHumidity || 55).toFixed(0) + '%';
        document.getElementById('mockSwitch').checked = !!data.mock;
        document.getElementById('autoSimSwitch').checked = !!data.autosim;
        document.getElementById('sensorStatus').textContent = data.mock ? 'Mock' : (data.autosim ? 'Auto-Sim' : 'Real');
        document.getElementById('sensorStatus').className = 'stat ' + ((data.mock || data.autosim) ? 'on' : 'off');
        document.getElementById('setValuesBtn').disabled = !data.mock;
        document.getElementById('mockValuesCard').style.display = data.mock ? 'block' : 'none';
        updateStageBadge(!!data.stageLockdown);
        document.getElementById('incubationStage').value = data.stageLockdown ? 'lockdown' : 'incubation';
        document.getElementById('sysHeap').textContent = data.sys && data.sys.heapFree ? (data.sys.heapFree / 1024).toFixed(0) + ' KB free' : '--';
        document.getElementById('sysCpu').textContent = data.sys && data.sys.cpu !== undefined ? data.sys.cpu + '%' : '--';
        document.getElementById('sysUptime').textContent = data.uptime || '--';
        document.getElementById('sysLogCnt').textContent = data.logCnt + ' records';
        document.getElementById('sysLogStorage').textContent = data.logStorage + ' KB';
      }).catch(function(error) {
        console.error(error);
      });
    }

    function toggleMock() {
      var enable = document.getElementById('mockSwitch').checked;
      fetch('/mock/api?enable=' + (enable ? '1' : '0')).then(function() {
        updateData();
        loadSettings();
      }).catch(function(error) {
        console.error(error);
      });
    }

    function toggleAutoSim() {
      var enable = document.getElementById('autoSimSwitch').checked;
      fetch('/mock/api?autosim=' + (enable ? '1' : '0')).then(function() {
        updateData();
      }).catch(function(error) {
        console.error(error);
      });
    }

    function saveLogInterval() {
      var value = document.getElementById('logInterval').value;
      fetch('/mock/api?logInterval=' + value).then(function() {
        loadSettings();
      }).catch(function(error) {
        console.error(error);
      });
    }

    function saveFlashInterval() {
      var value = document.getElementById('saveFlashInterval').value;
      fetch('/mock/api?saveFlashInterval=' + value).then(function() {
        loadSettings();
      }).catch(function(error) {
        console.error(error);
      });
    }

    function saveEggTurnInterval() {
      var value = document.getElementById('eggTurnInterval').value;
      fetch('/mock/api?eggTurnInterval=' + value).then(function() {
        loadSettings();
      }).catch(function(error) {
        console.error(error);
      });
    }

    function saveAtomizerPulse() {
      var value = document.getElementById('atomizerPulse').value;
      fetch('/mock/api?pulseOnTime=' + value).then(function() {
        loadSettings();
      }).catch(function(error) {
        console.error(error);
      });
    }

    function saveIncubationStage() {
      var stage = document.getElementById('incubationStage').value;
      fetch('/mock/api?stageType=' + stage).then(function() {
        updateData();
        loadSettings();
      }).catch(function(error) {
        console.error(error);
      });
    }

    function setMockValues() {
      var temp = document.getElementById('mockTemp').value;
      var hum = document.getElementById('mockHum').value;
      fetch('/mock/api?temp=' + temp + '&hum=' + hum).then(function() {
        updateData();
      }).catch(function(error) {
        console.error(error);
      });
    }

    setInterval(updateData, 3000);
    loadSettings();
    updateData();
  </script>
</body>
</html>
)rawliteral";

#endif
