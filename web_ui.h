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
  <link rel="icon" href="data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 100 100'%3E%3Ctext y='.9em' font-size='90'%3E🥚%3C/text%3E%3C/svg%3E">
  <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.5.1/css/all.min.css">
  <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.2/dist/css/bootstrap.min.css" rel="stylesheet">
  <script src="https://cdn.jsdelivr.net/npm/chart.js@4.4.1/dist/chart.umd.min.js"></script>
  <script src="https://cdn.jsdelivr.net/npm/hammerjs@2.0.8"></script>
  <script src="https://cdn.jsdelivr.net/npm/chartjs-plugin-zoom@2.0.1/dist/chartjs-plugin-zoom.min.js"></script>
  <script src="https://cdn.jsdelivr.net/npm/dexie@3.2.4/dist/dexie.min.js"></script>
  <style>
    :root { --primary: #1877f2; --primary-dark: #166fe5; --bg: #fdfaf6; --on: #42b72a; --off: #f02849; --idle: #8a8d91; }
    * { box-sizing: border-box; }
    body { font-family: system-ui, sans-serif; margin: 0; background: var(--bg); color: #1c1e21; line-height: 1.5; }
    .container { max-width: 1100px; margin: 0 auto; padding: 10px; }
    .header { background: linear-gradient(135deg, var(--primary), var(--primary-dark)); padding: 20px; border-radius: 16px; margin-bottom: 16px; color: white; text-align: center; }
    .header h1 { margin: 0; font-size: 24px; font-weight: 800; }
    .card { background: #fff; border-radius: 12px; padding: 12px; margin-bottom: 12px; box-shadow: 0 1px 4px rgba(0,0,0,0.05); }
    .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(100px, 1fr)); gap: 8px; }
    .stat-card { background: #fff; padding: 8px; border-radius: 10px; text-align: center; border: 1px solid #edf0f5; }
    .stat-label { font-size: 9px; color: #65676b; font-weight: 700; text-transform: uppercase; letter-spacing: 0.5px; }
    .stat-value { font-size: 18px; font-weight: 800; margin-top: 4px; }
    .target-val { font-size: 10px; color: #adb5bd; margin-top: 4px; font-weight: 600; }
    .on { color: var(--on); }
    .off { color: var(--off); }
    .idle { color: var(--idle); }
    .badge { padding: 6px 12px; border-radius: 20px; font-size: 11px; font-weight: 700; text-transform: uppercase; }
    .badge-incubation { background: #e7f3ff; color: var(--primary); }
    .badge-lockdown { background: #fff4e5; color: #d97706; }
    .refresh-control { display: flex; align-items: center; gap: 8px; font-size: 12px; color: rgba(255,255,255,0.9); font-weight: 600; }
    .chart-box { height: 350px; width: 100%; margin-top: 8px; }
    .footer { text-align: center; font-size: 12px; color: #65676b; margin-top: 30px; padding: 20px 0; border-top: 1px solid #e0e4e9; }
    h3 { margin: 0 0 8px 0; font-size: 16px; font-weight: 700; color: #333; }
    .icon { font-size: 22px; color: #8a8d91; }
    .heater-active { color: #f39c12 !important; animation: bulb-glow 1.5s infinite alternate; }
    .fan-active { color: #42b72a !important; }
    .atomizer-active { color: var(--primary) !important; animation: spray-puff 0.5s infinite; }
    .atomizer-idle { filter: grayscale(100%); opacity: 0.5; }
    @keyframes bulb-glow { 0% { filter: drop-shadow(0 0 2px #f39c12); } 100% { filter: drop-shadow(0 0 12px #f39c12); } }
    @keyframes spray-puff { 0% { transform: scale(1); } 50% { transform: scale(1.15); } 100% { transform: scale(1); } }
    #loadingOverlay { position: fixed; inset: 0; background: rgba(253, 250, 246, 0.95); z-index: 9999; display: flex; flex-direction: column; justify-content: center; align-items: center; }
    .spinner { width: 40px; height: 40px; border: 4px solid #e7f3ff; border-top: 4px solid var(--primary); border-radius: 50%; animation: spin 1s linear infinite; }
    @keyframes spin { to { transform: rotate(360deg); } }
  </style>
</head>
<body>
  <div id="loadingOverlay">
    <div class="spinner"></div>
    <div style="font-weight: 800; color: var(--primary);">Synchronizing Records...</div>
    <div id="loadingProgress" style="font-size: 12px; color: var(--text-muted); margin-top: 5px;">Connecting to EGGubator</div>
  </div>
  <div class="container">
    <div class="header">
      <h1>🥚 EGGubator 🐣</h1>
    </div>

    <div class="card">
        <div style="display:flex; justify-content: space-between; align-items: center; margin-bottom: 24px;">
        <div style="display:flex; flex-direction:column;">
          <div id="smartBadge" class="badge badge-incubation" style="font-size:12px; font-weight:800; padding:6px 16px;">Day -- : Loading Stage...</div>
          <div style="font-size: 10px; color: var(--text-muted); font-weight: 600; margin-top: 4px; text-align: center;">STARTED: <span id="startDate">--</span></div>
        </div>
        <div style="font-size: 14px; color: var(--text-muted); font-weight: 600;">UPTIME: <span id="uptime" style="color:var(--text)">--</span></div>
      </div>
      <div class="grid" style="grid-template-columns: repeat(2, 1fr); margin-bottom: 10px;">
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
      </div>
      <div class="grid" style="grid-template-columns: repeat(4, 1fr);">
        <div class="stat-card">
          <div class="stat-label">Heater</div>
          <div class="stat-value" id="heaterStat"><i class="icon fa-solid fa-lightbulb"></i></div>
        </div>
        <div class="stat-card">
          <div class="stat-label">Spray</div>
          <div class="stat-value" id="atomizerStat"><i class="icon fa-solid fa-spray-can"></i></div>
        </div>
        <div class="stat-card">
          <div class="stat-label">Fan</div>
          <div class="stat-value" id="fanStat"><i class="icon fa-solid fa-fan"></i></div>
        </div>
        <div class="stat-card">
          <div class="stat-label">Turner</div>
          <div class="stat-value" id="turnerStat"><i class="icon fa-solid fa-arrow-up" id="turnerIcon"></i></div>
        </div>
      </div>
    </div>

    <div class="card">
      <h3>Historical Averages</h3>
      <div class="grid" style="grid-template-columns: repeat(3, 1fr);">
        <div class="stat-card">
          <div class="stat-label" style="margin-bottom: 6px;">1 Hour</div>
          <div style="display:flex; justify-content: space-evenly; align-items: flex-start;">
             <div style="text-align:center;">
               <div class="stat-label" style="font-size:7px; margin-bottom: 0;">Temp</div>
               <div class="stat-value" style="font-size:12px; margin-top: 0;" id="avgTemp1h">--</div>
               <div style="font-size:8px; color:#8a8d91; line-height:1;" id="rngTemp1h">--</div>
             </div>
             <div style="width:1px; height:35px; background:#e0e4e9;"></div>
             <div style="text-align:center;">
               <div class="stat-label" style="font-size:7px; margin-bottom: 0;">Hum</div>
               <div class="stat-value" style="font-size:12px; margin-top: 0;" id="avgHum1h">--</div>
               <div style="font-size:8px; color:#8a8d91; line-height:1;" id="rngHum1h">--</div>
             </div>
          </div>
        </div>
        <div class="stat-card">
          <div class="stat-label" style="margin-bottom: 6px;">8 Hours</div>
          <div style="display:flex; justify-content: space-evenly; align-items: flex-start;">
             <div style="text-align:center;">
               <div class="stat-label" style="font-size:7px; margin-bottom: 0;">Temp</div>
               <div class="stat-value" style="font-size:12px; margin-top: 0;" id="avgTemp8h">--</div>
               <div style="font-size:8px; color:#8a8d91; line-height:1;" id="rngTemp8h">--</div>
             </div>
             <div style="width:1px; height:35px; background:#e0e4e9;"></div>
             <div style="text-align:center;">
               <div class="stat-label" style="font-size:7px; margin-bottom: 0;">Hum</div>
               <div class="stat-value" style="font-size:12px; margin-top: 0;" id="avgHum8h">--</div>
               <div style="font-size:8px; color:#8a8d91; line-height:1;" id="rngHum8h">--</div>
             </div>
          </div>
        </div>
        <div class="stat-card">
          <div class="stat-label" style="margin-bottom: 6px;">12 Hours</div>
          <div style="display:flex; justify-content: space-evenly; align-items: flex-start;">
             <div style="text-align:center;">
               <div class="stat-label" style="font-size:7px; margin-bottom: 0;">Temp</div>
               <div class="stat-value" style="font-size:12px; margin-top: 0;" id="avgTemp12h">--</div>
               <div style="font-size:8px; color:#8a8d91; line-height:1;" id="rngTemp12h">--</div>
             </div>
             <div style="width:1px; height:35px; background:#e0e4e9;"></div>
             <div style="text-align:center;">
               <div class="stat-label" style="font-size:7px; margin-bottom: 0;">Hum</div>
               <div class="stat-value" style="font-size:12px; margin-top: 0;" id="avgHum12h">--</div>
               <div style="font-size:8px; color:#8a8d91; line-height:1;" id="rngHum12h">--</div>
             </div>
          </div>
        </div>
      </div>
    </div>

    <div class="card">
      <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 5px;">
        <h3 style="margin: 0;">History & Device Activity</h3>
        <select id="refreshRate" onchange="updateRefreshInterval()" style="padding: 4px 8px; border-radius: 8px; background: var(--bg); border: 1px solid #e0e4e9; color: var(--text); font-size: 11px; font-weight:700;">
          <option value="1000">1s</option>
          <option value="2000">2s</option>
          <option value="5000">5s</option>
          <option value="10000">10s</option>
          <option value="30000">30s</option>
        </select>
      </div>
      <div class="chart-box"><canvas id="mainChart"></canvas></div>
    </div>

    <div class="card">
      <h3 style="margin-bottom:10px;">Environment Stability Analysis</h3>
      <div style="font-size:11px; font-weight:bold; color:var(--text-muted); margin-bottom:4px;">LAST 24 HOURS</div>
      <div class="grid" style="grid-template-columns: repeat(2, 1fr); margin-bottom: 10px;">
        <div class="stat-card" style="text-align:left; padding:8px;">
          <div class="stat-label">Temperature (±0.3°C)</div>
          <div style="font-size:11px; line-height:1.4;">
            <div><span style="color:#f02849;font-weight:bold;">Under:</span> <span id="tUnder24">--</span></div>
            <div style="color:#8a8d91;font-size:9px;" id="tUnderMax24">Longest: --</div>
            <div style="margin-top:4px;"><span style="color:#f02849;font-weight:bold;">Over:</span> <span id="tOver24">--</span></div>
            <div style="color:#8a8d91;font-size:9px;" id="tOverMax24">Longest: --</div>
          </div>
        </div>
        <div class="stat-card" style="text-align:left; padding:8px;">
          <div class="stat-label">Humidity (±5.0%)</div>
          <div style="font-size:11px; line-height:1.4;">
            <div><span style="color:#1877f2;font-weight:bold;">Under:</span> <span id="hUnder24">--</span></div>
            <div style="color:#8a8d91;font-size:9px;" id="hUnderMax24">Longest: --</div>
            <div style="margin-top:4px;"><span style="color:#1877f2;font-weight:bold;">Over:</span> <span id="hOver24">--</span></div>
            <div style="color:#8a8d91;font-size:9px;" id="hOverMax24">Longest: --</div>
          </div>
        </div>
      </div>
      
      <div style="font-size:11px; font-weight:bold; color:var(--text-muted); margin-bottom:4px;">ENTIRE INCUBATION</div>
      <div class="grid" style="grid-template-columns: repeat(2, 1fr);">
        <div class="stat-card" style="text-align:left; padding:8px;">
          <div class="stat-label">Temperature (±0.3°C)</div>
          <div style="font-size:11px; line-height:1.4;">
            <div><span style="color:#f02849;font-weight:bold;">Under:</span> <span id="tUnderAll">--</span></div>
            <div style="color:#8a8d91;font-size:9px;" id="tUnderMaxAll">Longest: --</div>
            <div style="margin-top:4px;"><span style="color:#f02849;font-weight:bold;">Over:</span> <span id="tOverAll">--</span></div>
            <div style="color:#8a8d91;font-size:9px;" id="tOverMaxAll">Longest: --</div>
          </div>
        </div>
        <div class="stat-card" style="text-align:left; padding:8px;">
          <div class="stat-label">Humidity (±5.0%)</div>
          <div style="font-size:11px; line-height:1.4;">
            <div><span style="color:#1877f2;font-weight:bold;">Under:</span> <span id="hUnderAll">--</span></div>
            <div style="color:#8a8d91;font-size:9px;" id="hUnderMaxAll">Longest: --</div>
            <div style="margin-top:4px;"><span style="color:#1877f2;font-weight:bold;">Over:</span> <span id="hOverAll">--</span></div>
            <div style="color:#8a8d91;font-size:9px;" id="hOverMaxAll">Longest: --</div>
          </div>
        </div>
      </div>
    </div>

    <div class="footer">
      <strong>EGGubator System</strong> &copy; 2026 | <a href="/settings">Device Status & Settings &rarr;</a>
    </div>
  </div>

  <script>
    let mainChart;
    let refreshRate = 5000;
    let latestBootId = 0;
    let latestTimeSec = 0;
    let currentUptimeSec = 0;
    let currentBootId = 0;
    let initialLoadDone = false;
    let totalLogsLoaded = 0;
    let serverBootStartUnix = 0;

    const db = new Dexie('EggubatorDB');
    db.version(1).stores({ logs: 't, timeSec, bootId, temp, hum, h, a, f, s' });
    db.version(2).stores({ logs: 't, timeSec, bootId, temp, hum, h, a, f, s' });
    db.version(3).stores({
      logs: 't, timeSec, bootId, temp, hum, h, a, f, s',
      bootTimestamps: 'bootId, startUnix, duration'
    });
    db.version(4).stores({
      logs: 't, timeSec, bootId, temp, hum, h, a, f, s',
      bootTimestamps: 'bootId, startUnix, duration'
    }).upgrade(async () => { await db.logs.clear(); });

    const bootStartCache = {};

    async function loadBootStartCache() {
      const all = await db.bootTimestamps.toArray();
      all.forEach(bt => { bootStartCache[bt.bootId] = bt.startUnix; });
    }

    function decodeLogs(hex, logCount) {
      if (!hex) return [];
      const match = hex.match(/.{1,2}/g);
      if (!match) return [];
      const bytes = new Uint8Array(match.map(byte => parseInt(byte, 16)));
      const entries = [];
      const now = Date.now();
      const currentBootAvail = bootStartCache[currentBootId] != null;
      const bootStartEstimate = currentBootAvail
        ? bootStartCache[currentBootId] * 1000
        : (now - (currentUptimeSec * 1000));

      for (let i = 0; i < logCount; i++) {
        const offset = i * 8;
        if (offset + 8 > bytes.length) break;
        const timeSec = bytes[offset] | (bytes[offset+1] << 8) | (bytes[offset+2] << 16) | (bytes[offset+3] << 24);
        const temp = bytes[offset+4] / 10 + 20;
        const hum = bytes[offset+5];
        const states = bytes[offset+6];
        const bootId = bytes[offset+7];

        let absoluteT;
        if (bootStartCache[bootId] != null) {
          absoluteT = (bootStartCache[bootId] + timeSec) * 1000;
        } else if (bootId === currentBootId) {
          absoluteT = bootStartEstimate + (timeSec * 1000);
        } else {
          const diff = (currentBootId - bootId + 256) % 256;
          absoluteT = bootStartEstimate - (diff * 20 * 1000) + (timeSec * 1000);
        }

        entries.push({ t: absoluteT, timeSec: timeSec, bootId: bootId, temp, hum, h: states & 1, a: (states >> 1) & 1, f: (states >> 2) & 1, s: ((states >> 3) & 0x1F) });
      }
      return entries;
    }

    async function cleanupDB() {
      // 1. Hard purge everything older than 30 days
      const thirtyDaysAgo = Date.now() - (30 * 24 * 60 * 60 * 1000);
      const purged = await db.logs.where('t').below(thirtyDaysAgo).delete();
      if (purged > 0) console.log(`Purged ${purged} logs older than 30 days.`);

      // 2. Downsample logs between 48 hours and 30 days old to 15-minute intervals
      const twoDaysAgo = Date.now() - (48 * 60 * 60 * 1000);
      const oldLogs = await db.logs.where('t').below(twoDaysAgo).toArray();
      
      if (oldLogs.length === 0) return;
      
      const seenBuckets = new Set();
      const deleteKeys = [];
      const BUCKET_SIZE = 2 * 60 * 1000; // 2 minutes
      
      oldLogs.forEach(log => {
        const bucketId = Math.floor(log.t / BUCKET_SIZE);
        if (!seenBuckets.has(bucketId)) {
          seenBuckets.add(bucketId);
        } else {
          deleteKeys.push(log.t);
        }
      });
      
      if (deleteKeys.length > 0) {
        // chunk deletion to avoid blocking main thread or hitting limits
        const chunkSize = 1000;
        for (let i = 0; i < deleteKeys.length; i += chunkSize) {
          const chunk = deleteKeys.slice(i, i + chunkSize);
          await db.logs.bulkDelete(chunk);
        }
        console.log(`Cleaned up ${deleteKeys.length} redundant logs older than 48h.`);
      }
    }

    function initCharts() {
      const ctx = document.getElementById('mainChart').getContext('2d');
      mainChart = new Chart(ctx, {
        type: 'line',
        data: {
          datasets: [
            { label: 'Temp (°C)', borderColor: '#f02849', data: [], yAxisID: 'yTemp', pointRadius: 0, borderWidth: 3, tension: 0.35 },
            { label: 'Hum (%)', borderColor: '#1877f2', data: [], yAxisID: 'yHum', pointRadius: 0, borderWidth: 3, tension: 0.35 },
            { label: 'Heater', borderColor: '#f02849', data: [], yAxisID: 'yControls', stepped: true, pointRadius: 0, borderWidth: 2 },
            { label: 'Atomizer', borderColor: '#1877f2', data: [], yAxisID: 'yControls', stepped: true, pointRadius: 0, borderWidth: 2 },
            { label: 'Fan', borderColor: '#42b72a', data: [], yAxisID: 'yControls', stepped: true, pointRadius: 0, borderWidth: 2 },
            { label: 'Turner', borderColor: 'rgba(146, 94, 13, 0.5)', data: [], yAxisID: 'yTurner', stepped: true, pointRadius: 0, borderWidth: 2 }
          ]
        },
        options: {
          responsive: true, maintainAspectRatio: false,
          interaction: { mode: 'index', intersect: false },
          scales: {
              x: { 
              type: 'linear', 
              min: -300, 
              max: 0,
              title: { display: false }, 
              grid: { color: '#f0f0f0' },
              ticks: {
                maxRotation: 0,
                autoSkipPadding: 15,
                callback: function(value) {
                   const now = Date.now();
                   const date = new Date(now + value * 1000);
                   return date.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', hour12: true });
                }
              }
            },
            yTemp: { type: 'linear', position: 'left', afterDataLimits: (s) => { let r = s.max - s.min; if(r===0)r=1; s.min -= r*0.35; s.max += r*0.05; }, title: { display: false }, ticks: { padding: 2 } },
            yHum: { type: 'linear', position: 'right', afterDataLimits: (s) => { let r = s.max - s.min; if(r===0)r=1; s.min -= r*0.35; s.max += r*0.05; }, title: { display: false }, grid: { display: false }, ticks: { padding: 2 } },
            yControls: { type: 'linear', position: 'right', min: 0, max: 40, display: false },
            yTurner: { type: 'linear', position: 'right', min: 0, max: 180, display: false, grid: { drawOnChartArea: false } }
          },
          plugins: {
            decimation: { enabled: true, algorithm: 'min-max' },
            legend: { position: 'top', labels: { usePointStyle: true, boxWidth: 8, font: { size: 11, weight: '700' } } },
            tooltip: {
              callbacks: {
                title: function(context) {
                  const now = Date.now();
                  const relSeconds = context[0].parsed.x;
                  const absTime = new Date(now + relSeconds * 1000);
                  return absTime.toLocaleString([], { month: 'short', day: 'numeric', hour: '2-digit', minute: '2-digit', second: '2-digit', hour12: true });
                },
                label: function(context) {
                  const idx = context.datasetIndex;
                  const val = context.raw.y;
                  const isStepped = context.chart.data.datasets[idx].stepped;
                  if (idx === 2) return 'Heater: ' + (isStepped ? (val > 0 ? 'ON' : 'OFF') : Math.round(val * 100) + '%');
                  if (idx === 3) return 'Atomizer: ' + (isStepped ? (val > 2 ? 'ON' : 'OFF') : Math.round((val - 2) * 100) + '%');
                  if (idx === 4) return 'Fan: ' + (isStepped ? (val > 4 ? 'ON' : 'OFF') : Math.round((val - 4) * 100) + '%');
                  if (idx === 5) return 'Turner: ' + Math.round(context.raw.y) + '°';
                  return null;
                },
                filter: function(context) {
                  return context.datasetIndex >= 2;
                }
              }
            },
            zoom: { 
              pan: { enabled: true, mode: 'x', onPanComplete: function() { updateChart(); } }, 
              zoom: { wheel: { enabled: true }, pinch: { enabled: true }, mode: 'x', onZoomComplete: function() { updateChart(); } }
            }
          }
        }
      });
    }

    function updateLiveData(d) {
      document.getElementById('temp').textContent = d.temperature.toFixed(1) + '°C';
      document.getElementById('hum').textContent = d.humidity.toFixed(1) + '%';
      document.getElementById('targetTemp').textContent = d.targetTemp.toFixed(1);
      document.getElementById('targetHum').textContent = d.targetHum.toFixed(1);
      document.getElementById('uptime').textContent = d.uptime;
      
      document.getElementById('heaterStat').firstElementChild.className = 'icon fa-solid fa-lightbulb ' + (d.heater ? 'heater-active' : '');
      document.getElementById('atomizerStat').firstElementChild.className = 'icon fa-solid fa-spray-can ' + (d.atomizer ? 'atomizer-active' : 'atomizer-idle');
      document.getElementById('fanStat').firstElementChild.className = 'icon fa-solid fa-fan ' + (d.fan ? 'fan-active fa-spin' : '');
      
      const servoAngle = Math.round(d.servo * 6);
      document.getElementById('turnerStat').innerHTML = d.servo > 0 ? servoAngle + '°' : '<i class="icon fa-solid fa-arrow-up" id="turnerIcon" style="color: var(--idle);"></i>';

      const badge = document.getElementById('smartBadge');
      badge.textContent = 'Day ' + (d.currentDay + 1) + (d.stageLockdown ? ' - Lockdown Stage' : ' - Incubation Stage');
      badge.className = 'badge ' + (d.stageLockdown ? 'badge-lockdown' : 'badge-incubation');
      
      const sd = document.getElementById('startDate');
      if (d.startTimestamp > 0) {
         sd.textContent = new Date(d.startTimestamp * 1000).toLocaleDateString();
      } else {
         sd.textContent = 'Unknown';
      }
      
      calculateAverages();
      calculateStability();
    }

    async function calculateAverages() {
      const now = Date.now();
      const windows = [3600, 28800, 43200]; // 1h, 8h, 12h in seconds
      const ids = [
        {tAvg:'avgTemp1h', hAvg:'avgHum1h', tRng:'rngTemp1h', hRng:'rngHum1h'},
        {tAvg:'avgTemp8h', hAvg:'avgHum8h', tRng:'rngTemp8h', hRng:'rngHum8h'},
        {tAvg:'avgTemp12h', hAvg:'avgHum12h', tRng:'rngTemp12h', hRng:'rngHum12h'}
      ];
      
      for (let i = 0; i < windows.length; i++) {
        const win = windows[i];
        const logs = await db.logs.where('t').above(now - win * 1000).toArray();
        if (logs.length > 0) {
          let sumT = 0, sumH = 0;
          let minT = 999, maxT = -999, minH = 999, maxH = -999;
          
          for(let j=0; j<logs.length; j++) {
            const l = logs[j];
            sumT += l.temp;
            sumH += l.hum;
            if(l.temp < minT) minT = l.temp;
            if(l.temp > maxT) maxT = l.temp;
            if(l.hum < minH) minH = l.hum;
            if(l.hum > maxH) maxH = l.hum;
          }
          
          const avgTemp = sumT / logs.length;
          const avgHum = sumH / logs.length;
          
          document.getElementById(ids[i].tAvg).textContent = avgTemp.toFixed(1) + '°C';
          document.getElementById(ids[i].hAvg).textContent = avgHum.toFixed(1) + '%';
          
          document.getElementById(ids[i].tRng).textContent = minT.toFixed(1) + ' - ' + maxT.toFixed(1);
          document.getElementById(ids[i].hRng).textContent = minH.toFixed(1) + ' - ' + maxH.toFixed(1);
        }
      }
    }

    function formatDuration(ms) {
      if (ms === 0) return 'None';
      const totalMins = Math.floor(ms / 60000);
      if (totalMins === 0) return '< 1m';
      const h = Math.floor(totalMins / 60);
      const m = totalMins % 60;
      return (h > 0 ? h + 'h ' : '') + m + 'm';
    }

    function formatDateRange(start, end) {
      if (!start) return '--';
      const s = new Date(start);
      const e = new Date(end);
      const sTime = s.toLocaleTimeString([], {hour: '2-digit', minute:'2-digit', hour12: true});
      const eTime = e.toLocaleTimeString([], {hour: '2-digit', minute:'2-digit', hour12: true});
      return s.toLocaleDateString([], {month: 'short', day: 'numeric'}) + ' ' + sTime + '-' + eTime;
    }

    async function calculateStability() {
      const now = Date.now();
      const targetT = parseFloat(document.getElementById('targetTemp').textContent) || 37.5;
      const targetH = parseFloat(document.getElementById('targetHum').textContent) || 55.0;
      const tempUnderThreshold = targetT - 0.3;
      const tempOverThreshold = targetT + 0.3;
      const humUnderThreshold = targetH - 5.0;
      const humOverThreshold = targetH + 5.0;

      const logsAll = await db.logs.orderBy('t').toArray();
      if (logsAll.length === 0) return;

      const logs24h = logsAll.filter(l => l.t >= now - 24 * 3600 * 1000);

      function analyze(logs, prefix) {
        if (!logs || logs.length === 0) return;
        let tUnderTime = 0, tOverTime = 0;
        let hUnderTime = 0, hOverTime = 0;

        let tUnderStart = null, tUnderMax = 0, tUnderMaxRange = null;
        let tOverStart = null, tOverMax = 0, tOverMaxRange = null;
        let hUnderStart = null, hUnderMax = 0, hUnderMaxRange = null;
        let hOverStart = null, hOverMax = 0, hOverMaxRange = null;

        for (let i = 0; i < logs.length; i++) {
          const l = logs[i];
          const prevT = i > 0 ? logs[i-1].t : l.t;
          const dt = l.t - prevT;
          
          const isOutage = dt > 600000; // > 10 minutes gap
          const effectiveDt = isOutage ? 0 : dt;
          
          if (isOutage) {
            if (tUnderStart) { const dur = prevT - tUnderStart; if (dur > tUnderMax) { tUnderMax = dur; tUnderMaxRange = {s: tUnderStart, e: prevT}; } tUnderStart = null; }
            if (tOverStart) { const dur = prevT - tOverStart; if (dur > tOverMax) { tOverMax = dur; tOverMaxRange = {s: tOverStart, e: prevT}; } tOverStart = null; }
            if (hUnderStart) { const dur = prevT - hUnderStart; if (dur > hUnderMax) { hUnderMax = dur; hUnderMaxRange = {s: hUnderStart, e: prevT}; } hUnderStart = null; }
            if (hOverStart) { const dur = prevT - hOverStart; if (dur > hOverMax) { hOverMax = dur; hOverMaxRange = {s: hOverStart, e: prevT}; } hOverStart = null; }
          }

          // Temp Under
          if (l.temp < tempUnderThreshold) {
            tUnderTime += effectiveDt;
            if (!tUnderStart) tUnderStart = l.t;
          } else {
            if (tUnderStart) {
              const dur = prevT - tUnderStart;
              if (dur > tUnderMax) { tUnderMax = dur; tUnderMaxRange = {s: tUnderStart, e: prevT}; }
              tUnderStart = null;
            }
          }

          // Temp Over
          if (l.temp > tempOverThreshold) {
            tOverTime += effectiveDt;
            if (!tOverStart) tOverStart = l.t;
          } else {
            if (tOverStart) {
              const dur = prevT - tOverStart;
              if (dur > tOverMax) { tOverMax = dur; tOverMaxRange = {s: tOverStart, e: prevT}; }
              tOverStart = null;
            }
          }

          // Hum Under
          if (l.hum < humUnderThreshold) {
            hUnderTime += effectiveDt;
            if (!hUnderStart) hUnderStart = l.t;
          } else {
            if (hUnderStart) {
              const dur = prevT - hUnderStart;
              if (dur > hUnderMax) { hUnderMax = dur; hUnderMaxRange = {s: hUnderStart, e: prevT}; }
              hUnderStart = null;
            }
          }

          // Hum Over
          if (l.hum > humOverThreshold) {
            hOverTime += effectiveDt;
            if (!hOverStart) hOverStart = l.t;
          } else {
            if (hOverStart) {
              const dur = prevT - hOverStart;
              if (dur > hOverMax) { hOverMax = dur; hOverMaxRange = {s: hOverStart, e: prevT}; }
              hOverStart = null;
            }
          }
        }

        // Close trailing ranges
        const lastT = logs[logs.length-1].t;
        if (tUnderStart) { const dur = lastT - tUnderStart; if (dur > tUnderMax) { tUnderMax = dur; tUnderMaxRange = {s: tUnderStart, e: lastT}; } }
        if (tOverStart) { const dur = lastT - tOverStart; if (dur > tOverMax) { tOverMax = dur; tOverMaxRange = {s: tOverStart, e: lastT}; } }
        if (hUnderStart) { const dur = lastT - hUnderStart; if (dur > hUnderMax) { hUnderMax = dur; hUnderMaxRange = {s: hUnderStart, e: lastT}; } }
        if (hOverStart) { const dur = lastT - hOverStart; if (dur > hOverMax) { hOverMax = dur; hOverMaxRange = {s: hOverStart, e: lastT}; } }

        document.getElementById('tUnder' + prefix).textContent = formatDuration(tUnderTime);
        document.getElementById('tOver' + prefix).textContent = formatDuration(tOverTime);
        document.getElementById('hUnder' + prefix).textContent = formatDuration(hUnderTime);
        document.getElementById('hOver' + prefix).textContent = formatDuration(hOverTime);

        document.getElementById('tUnderMax' + prefix).textContent = tUnderMax > 0 ? `Longest: ${formatDuration(tUnderMax)} (${formatDateRange(tUnderMaxRange.s, tUnderMaxRange.e)})` : 'Longest: None';
        document.getElementById('tOverMax' + prefix).textContent = tOverMax > 0 ? `Longest: ${formatDuration(tOverMax)} (${formatDateRange(tOverMaxRange.s, tOverMaxRange.e)})` : 'Longest: None';
        document.getElementById('hUnderMax' + prefix).textContent = hUnderMax > 0 ? `Longest: ${formatDuration(hUnderMax)} (${formatDateRange(hUnderMaxRange.s, hUnderMaxRange.e)})` : 'Longest: None';
        document.getElementById('hOverMax' + prefix).textContent = hOverMax > 0 ? `Longest: ${formatDuration(hOverMax)} (${formatDateRange(hOverMaxRange.s, hOverMaxRange.e)})` : 'Longest: None';
      }

      analyze(logs24h, '24');
      analyze(logsAll, 'All');
    }

    async function deriveBootHistoryFromLogs() {
      const logs = await db.logs.orderBy('t').toArray();
      if (logs.length === 0) return [];

      const bootMap = {};
      logs.forEach(l => {
        if (!bootMap[l.bootId]) {
          bootMap[l.bootId] = { bootId: l.bootId, firstT: l.t, lastT: l.t, firstTimeSec: l.timeSec };
        } else {
          if (l.t < bootMap[l.bootId].firstT) {
            bootMap[l.bootId].firstT = l.t;
            bootMap[l.bootId].firstTimeSec = l.timeSec;
          }
          if (l.t > bootMap[l.bootId].lastT) bootMap[l.bootId].lastT = l.t;
        }
      });

      const result = Object.values(bootMap);
      result.sort((a, b) => a.bootId - b.bootId);

      return result.map(b => ({
        bootId: b.bootId,
        startUnix: Math.round((b.firstT - b.firstTimeSec * 1000) / 1000),
        duration: Math.round((b.lastT - b.firstT) / 1000)
      }));
    }

    async function syncSAT() {
      const progressEl = document.getElementById('loadingProgress');
      if (progressEl) progressEl.textContent = 'SAT sync: fetching ESP timestamps...';

      let espBootTable = [];
      let espBootId = 0;
      try {
        const resp = await fetch('/timestamps');
        const data = await resp.json();
        espBootTable = data.bootTable || [];
        espBootId = data.currentBootId || 0;
        console.log('ESP boot table:', espBootTable);
      } catch (e) {
        console.warn('SAT sync failed (GET /timestamps):', e);
        return;
      }

      if (progressEl) progressEl.textContent = 'SAT sync: reading Dexie boot history...';
      await loadBootStartCache();
      const dexieHistory = await deriveBootHistoryFromLogs();

      if (progressEl) progressEl.textContent = 'SAT sync: reconciling timelines...';
      const mergedMap = {};
      espBootTable.forEach(bt => { mergedMap[bt.bootId] = { ...bt, source: 'esp' }; });
      dexieHistory.forEach(dh => {
        if (mergedMap[dh.bootId]) {
          if (dh.startUnix > mergedMap[dh.bootId].startUnix)
            mergedMap[dh.bootId] = { ...dh, source: 'dexie' };
        } else {
          mergedMap[dh.bootId] = { ...dh, source: 'dexie' };
        }
      });

      const mergedTimeline = Object.values(mergedMap).sort((a, b) => a.bootId - b.bootId);

      if (progressEl) progressEl.textContent = 'SAT sync: pushing to ESP...';
      try {
        const putBody = JSON.stringify(mergedTimeline.map(t => ({ bootId: t.bootId, startUnix: t.startUnix, duration: t.duration || 0 })));
        const putResp = await fetch('/timestamps', { method: 'PUT', body: putBody });
        const putResult = await putResp.json();
        console.log('PUT /timestamps result:', putResult);

        await db.bootTimestamps.clear();
        await db.bootTimestamps.bulkPut(mergedTimeline.map(t => ({
          bootId: t.bootId, startUnix: t.startUnix, duration: t.duration || 0
        })));
        await loadBootStartCache();

        const unixNow = Math.floor(Date.now() / 1000);
        await fetch('/settings/api?action=syncTime&timestamp=' + unixNow);
      } catch (e) {
        console.warn('SAT sync failed:', e);
      }

      if (progressEl) progressEl.textContent = 'SAT sync complete';
    }

    async function checkAutoReset() {
      // Step 0: SAT sync first
      await syncSAT();

      const lastLog = await db.logs.orderBy('t').last();
      const unixNow = Math.floor(Date.now() / 1000);
      
      if (lastLog) {
        const gap = Date.now() - lastLog.t;
        if (gap > 259200000) { // 3 days in ms
          console.log("Incubator was off for >3 days. Resetting to Day 1.");
          await fetch('/settings/api?action=newBatch&timestamp=' + unixNow);
        }
      } else {
        // No logs at all, maybe first run?
        await fetch('/settings/api?action=newBatch&timestamp=' + unixNow);
      }
    }

    async function updateChart() {
      const now = Date.now();
      const viewStart = now - (24 * 3600 * 1000); // Up to 24 hours ago
      
      const rawLogs = await db.logs.where('t').above(viewStart).toArray();
      if (rawLogs.length > 0) {
        let visibleSeconds = 4 * 3600;
        if (mainChart.scales && mainChart.scales.x && mainChart.scales.x.max !== undefined) {
           visibleSeconds = mainChart.scales.x.max - mainChart.scales.x.min;
        }

        let bucketSize = 0;
        if (visibleSeconds > 12 * 3600) bucketSize = 5 * 60 * 1000; // 5 mins
        else if (visibleSeconds > 4 * 3600) bucketSize = 2 * 60 * 1000; // 2 mins
        else if (visibleSeconds > 1 * 3600) bucketSize = 30 * 1000; // 30 secs
        
        let logs = rawLogs;
        if (bucketSize > 0) {
          const buckets = [];
          let currentBucket = null;
          rawLogs.forEach(l => {
            const bTime = Math.floor(l.t / bucketSize) * bucketSize;
            if (!currentBucket || currentBucket.t !== bTime) {
              if (currentBucket) buckets.push(currentBucket);
              currentBucket = { t: bTime, tempSum: 0, humSum: 0, hSum: 0, aSum: 0, fSum: 0, sCounts: {0:0, 1:0, 2:0, 3:0}, count: 0 };
            }
            currentBucket.tempSum += l.temp;
            currentBucket.humSum += l.hum;
            currentBucket.hSum += (l.h ? 1 : 0);
            currentBucket.aSum += (l.a ? 1 : 0);
            currentBucket.fSum += (l.f ? 1 : 0);
            currentBucket.sCounts[l.s]++;
            currentBucket.count++;
          });
          if (currentBucket) buckets.push(currentBucket);
          
          logs = buckets.map(b => {
            let dominantS = 0;
            let maxCount = 0;
            for (let s in b.sCounts) {
              if (b.sCounts[s] > maxCount) { maxCount = b.sCounts[s]; dominantS = parseInt(s); }
            }
            return {
              t: b.t + bucketSize/2,
              temp: b.tempSum / b.count,
              hum: b.humSum / b.count,
              h_dc: b.hSum / b.count,
              a_dc: b.aSum / b.count,
              f_dc: b.fSum / b.count,
              s: dominantS
            };
          });
        } else {
          logs = rawLogs.map(l => ({
            ...l,
            h_dc: l.h ? 1.0 : 0.0,
            a_dc: l.a ? 1.0 : 0.0,
            f_dc: l.f ? 1.0 : 0.0
          }));
        }

        const isStepped = (bucketSize === 0);
        mainChart.data.datasets[2].stepped = isStepped;
        mainChart.data.datasets[3].stepped = isStepped;
        mainChart.data.datasets[4].stepped = isStepped;

        mainChart.data.datasets[0].data = logs.map(l => ({ x: (l.t - now)/1000, y: l.temp }));
        mainChart.data.datasets[1].data = logs.map(l => ({ x: (l.t - now)/1000, y: l.hum }));
        mainChart.data.datasets[2].data = logs.map(l => ({ x: (l.t - now)/1000, y: l.h_dc }));
        mainChart.data.datasets[3].data = logs.map(l => ({ x: (l.t - now)/1000, y: 2.0 + l.a_dc }));
        mainChart.data.datasets[4].data = logs.map(l => ({ x: (l.t - now)/1000, y: 4.0 + l.f_dc }));
        mainChart.data.datasets[5].data = logs.map(l => ({ x: (l.t - now)/1000, y: l.s * 6 }));
        
        mainChart.update('none');
      }
    }

    function hideOverlay() {
      const overlay = document.getElementById('loadingOverlay');
      if (overlay) {
        overlay.style.opacity = '0';
        setTimeout(() => overlay.style.display = 'none', 300);
      }
    }

    async function fetchNextBatch(bootId, timeSec) {
      let d;
      try {
        const r = await fetch('/data?boot=' + bootId + '&time=' + timeSec + '&count=200');
        d = await r.json();
      } catch(e) {
        console.error('Fetch data failed:', e);
        initialLoadDone = true;
        hideOverlay();
        return;
      }

      // Update our boot table cache from server
      if (d.bootStartUnix) {
        serverBootStartUnix = d.bootStartUnix;
        bootStartCache[d.bootId] = d.bootStartUnix;
        currentBootId = d.bootId;
        currentUptimeSec = d.uptimeSec;
      }

      const progressEl = document.getElementById('loadingProgress');
      if (progressEl) progressEl.textContent = 'Loading ' + (totalLogsLoaded + d.sentCount) + ' records...';

      const newEntries = decodeLogs(d.logs, d.sentCount);
      if (newEntries.length > 0) {
        await db.logs.bulkPut(newEntries);
        totalLogsLoaded += newEntries.length;
      }
      if (d.sentCount >= 200 && totalLogsLoaded < d.totalLogs) {
        const nextBootId = newEntries.length > 0 ? newEntries[newEntries.length-1].bootId : bootId;
        const nextTimeSec = newEntries.length > 0 ? newEntries[newEntries.length-1].timeSec : timeSec;

        if (nextBootId === bootId && nextTimeSec === timeSec) {
          console.warn("Pagination stuck on same boot/time. Stopping.");
        } else {
          await fetchNextBatch(nextBootId, nextTimeSec);
          return;
        }
      }

      // End of pagination
      initialLoadDone = true;
      if (newEntries.length > 0) {
        latestBootId = newEntries[newEntries.length-1].bootId;
        latestTimeSec = newEntries[newEntries.length-1].timeSec;
      }
      await updateChart();
      await calculateAverages();
      await calculateStability();
      await cleanupDB();
      hideOverlay();
    }

    let lastChartUpdate = 0;

    async function mainLoop() {
      try {
        const statusRes = await fetch('/status');
        const statusData = await statusRes.json();
        currentUptimeSec = statusData.uptimeSec;
        currentBootId = statusData.bootId;
        updateLiveData(statusData);

        const now = Date.now();
        if (now - lastChartUpdate >= refreshRate) {
          if (!initialLoadDone) {
            const lastLogArr = await db.logs.orderBy('t').reverse().limit(1).toArray();
            if (lastLogArr.length > 0) {
               latestBootId = lastLogArr[0].bootId;
               latestTimeSec = lastLogArr[0].timeSec;
            }
            await fetchNextBatch(latestBootId, latestTimeSec);
          } else {
            try {
              const dataRes = await fetch('/data?boot=' + latestBootId + '&time=' + latestTimeSec + '&count=200');
              const dataData = await dataRes.json();
              if (dataData.bootStartUnix) {
                bootStartCache[dataData.bootId] = dataData.bootStartUnix;
                currentUptimeSec = dataData.uptimeSec;
              }
              const newEntries = decodeLogs(dataData.logs, dataData.sentCount);
              if (newEntries.length > 0) {
                await db.logs.bulkPut(newEntries);
                latestBootId = newEntries[newEntries.length-1].bootId;
                latestTimeSec = newEntries[newEntries.length-1].timeSec;
                await updateChart();
                await calculateAverages();
                await calculateStability();
                await cleanupDB();
              }
            } catch(e) {
              console.error('Incremental fetch error:', e);
            }
          }
          lastChartUpdate = now;
        }
      } catch (e) {
        console.error("Fetch error", e);
      } finally {
        setTimeout(mainLoop, 1000); 
      }
    }

    function update() {
       fetch('/status').then(r=>r.json()).then(d => updateLiveData(d));
    }

    function updateRefreshInterval() {
      const rate = parseInt(document.getElementById('refreshRate').value);
      refreshRate = rate;
      // Removed fetch to ESP to prevent EEPROM wear
    }

    initCharts();

// Set initial refresh rate
    refreshRate = 5000;
    const sel = document.getElementById('refreshRate');
    if(sel) sel.value = refreshRate;

    // Initialize
    (async () => {
      try {
        // Step 0: Fetch status first
        const statusRes = await fetch('/status');
        const statusData = await statusRes.json();
        currentUptimeSec = statusData.uptimeSec;
        currentBootId = statusData.bootId;
        updateLiveData(statusData);

        // Step 1: SAT sync first to populate bootStartCache
        const satProgress = document.getElementById('loadingProgress');
        if (satProgress) satProgress.textContent = 'Syncing timestamps...';
        try {
          await syncSAT();
        } catch (e) {
          console.warn('Initial SAT sync failed:', e);
        }

        // Step 2: Purge any remaining invalid entries (t < year 2000)
        const invalidCount = await db.logs.where('t').below(946684800000).count();
        if (invalidCount > 0) {
          await db.logs.where('t').below(946684800000).delete();
          console.log('Purged', invalidCount, 'invalid entries with bad timestamps');
        }

        // Step 3: Check if we already have logs in Dexie — skip full pagination if so
        const existingCount = await db.logs.count();
        if (existingCount > 0) {
          initialLoadDone = true;
          const lastLog = await db.logs.orderBy('t').reverse().limit(1).toArray();
          if (lastLog.length > 0) {
            latestBootId = lastLog[0].bootId;
            latestTimeSec = lastLog[0].timeSec;
            console.log('Dexie has ' + existingCount + ' logs, skipping full re-fetch. Resuming from bootId=' + latestBootId + ' timeSec=' + latestTimeSec);
          }
          hideOverlay();
        } else {
          // No logs in Dexie — do full pagination from the start
          await fetchNextBatch(0, 0);
        }

        // Step 4: Check if auto-reset needed
        await checkAutoReset();

        // Step 5: Start main loop
        mainLoop();
      } catch(e) {
        console.error('Init failed:', e);
        mainLoop();
      }
    })();
  </script>
</body>
</html>
)rawliteral";

const char SETTINGS_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>EGGubator - Settings</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <script src="https://cdn.jsdelivr.net/npm/dexie@3.2.4/dist/dexie.min.js"></script>
  <style>
    body { font-family: system-ui, sans-serif; padding: 15px; background: #f8f9fa; }
    .card { background: #fff; border-radius: 12px; padding: 16px; margin-bottom: 16px; box-shadow: 0 2px 8px rgba(0,0,0,0.08); }
    h3 { color: #1877f2; font-weight: 700; border-bottom: 1px solid #eee; padding-bottom: 10px; margin: 0 0 16px 0; }
    .form-group { margin-bottom: 14px; }
    .form-group label { display: block; font-weight: 600; color: #444; margin-bottom: 6px; }
    .form-control, .form-select { width: 100%; padding: 10px 12px; border-radius: 8px; border: 1px solid #ddd; font-size: 14px; }
    .form-select { background-color: #fff; }
    .btn { padding: 10px 16px; border-radius: 8px; border: none; font-weight: 600; cursor: pointer; }
    .btn-primary { background: #1877f2; color: #fff; width: 100%; }
    .btn-outline { background: #e7f3ff; color: #1877f2; flex: 1; }
    .btn-danger { background: #dc3545; color: #fff; width: 100%; margin-top: 20px; }
    .btn-sm { padding: 8px 12px; font-size: 13px; }
    .btn-group { display: flex; gap: 8px; margin-bottom: 16px; }
    .sys-item { display: flex; justify-content: space-between; padding: 10px 0; border-bottom: 1px solid #f0f0f0; }
    .sys-item span:first-child { color: #6c757d; font-weight: 600; font-size: 11px; text-transform: uppercase; }
    .sys-item span:last-child { color: #212529; font-weight: 600; font-family: monospace; }
    .day-display { text-align: center; margin-bottom: 16px; }
    .day-display .label { font-size: 11px; color: #888; text-transform: uppercase; font-weight: 700; }
    .day-display .value { font-size: 40px; font-weight: 800; color: #1877f2; line-height: 1.2; margin: 8px 0; }
    .day-display .sub { font-size: 13px; color: #888; }
    .angle-control { display: flex; align-items: center; gap: 8px; }
    .angle-control button { width: 36px; padding: 8px; background: #e7f3ff; color: #1877f2; border: none; border-radius: 6px; font-weight: 700; }
    .angle-control input { width: 60px; text-align: center; padding: 8px; border-radius: 6px; border: 1px solid #ddd; }
    .checkbox-group { display: flex; align-items: center; gap: 8px; }
    .checkbox-group input { width: 20px; height: 20px; }
    a { display: block; text-align: center; color: #1877f2; font-weight: 600; margin-top: 16px; text-decoration: none; }
  </style>
</head>
<body>
  <div class="card">
    <h3>System Telemetry</h3>
    <div><div class="sys-item"><span>IP Address</span><span id="ip">--</span></div>
      <div class="sys-item"><span>WiFi RSSI</span><span id="rssi">-- dBm</span></div>
      <div class="sys-item"><span>Free RAM</span><span id="heap">-- KB</span></div>
      <div class="sys-item"><span>Current Sector</span><span id="sector">--</span></div>
      <div class="sys-item"><span>Start Sector</span><span id="startSector">--</span></div>
      <div class="sys-item"><span>Logs Since Boot</span><span id="bootLogs">--</span></div>
      <div class="sys-item"><span>Dexie Records</span><span id="dexieCount">--</span></div>
      <div class="sys-item"><span>Firmware</span><span id="version">--</span></div>
      <div class="sys-item"><span>Uptime</span><span id="uptimeSys">--</span></div></div>
  </div>

  <div class="card">
    <h3>Incubation Cycle</h3>
    <div class="day-display">
      <div class="label">Current Day</div>
      <div class="value" id="currentDayVal">--</div>
      <div class="sub">Started: <span id="startDateVal">--</span></div>
    </div>
    <div class="btn-group">
      <button class="btn btn-outline" onclick="adjustDay(-1)">- 1 Day</button>
      <button class="btn btn-outline" onclick="adjustDay(1)">+ 1 Day</button>
    </div>
    <button class="btn btn-primary" style="margin-top:0;" onclick="startNewBatch()">Start New Batch (Day 0)</button>
  </div>

  <div class="card">
    <h3>Sensor Simulation</h3>
    <div class="checkbox-group form-group">
      <input type="checkbox" id="mockEnable" onchange="toggle('enable')">
      <label>Use Mock Sensor</label>
    </div>
    <div class="checkbox-group form-group">
      <input type="checkbox" id="autoSim" onchange="toggle('autosim')">
      <label>Physics Simulation</label>
    </div>
    <div id="mockFields">
      <div class="form-group">
        <label>Mock Temp (°C)</label>
        <input type="number" id="mTemp" class="form-control" step="0.1" value="37.5">
      </div>
      <div class="form-group">
        <label>Mock Hum (%)</label>
        <input type="number" id="mHum" class="form-control" step="0.1" value="60">
      </div>
      <button class="btn btn-primary" onclick="setMock()">Apply Simulation Values</button>
    </div>
  </div>

  <div class="card">
    <h3>Maintenance</h3>
    <div class="form-group">
      <label>Egg Turn Interval</label>
      <select id="eggTurnInterval" class="form-select" onchange="save('eggTurnInterval')">
        <option value="7200000" selected>2 hours</option>
        <option value="10800000">3 hours</option>
        <option value="14400000">4 hours</option>
        <option value="21600000">6 hours</option>
      </select>
    </div>
    <div class="form-group">
      <label>Egg Sweep Duration</label>
      <select id="eggTurnDuration" class="form-select" onchange="save('eggTurnDuration')">
        <option value="1">1 sec</option>
        <option value="2">2 sec</option>
        <option value="3">3 sec</option>
        <option value="4">4 sec</option>
        <option value="5">5 sec</option>
        <option value="6">6 sec</option>
        <option value="7">7 sec</option>
        <option value="8">8 sec</option>
        <option value="9">9 sec</option>
        <option value="10">10 sec</option>
      </select>
    </div>
    <div class="form-group">
      <label>Angle Adjustment (-40 to +40)</label>
      <div class="angle-control">
        <button type="button" onclick="adjustAngle(-5)">-</button>
        <input type="number" id="angleAdjustment" min="-42" max="42" step="6" value="0" readonly onchange="save('angleAdjustment')">
        <button type="button" onclick="adjustAngle(5)">+</button>
      </div>
    </div>
    <div class="form-group">
      <label>Log Heartbeat Interval</label>
      <select id="logInterval" class="form-select" onchange="save('logInterval')">
        <option value="5000">5 sec</option>
        <option value="10000">10 sec</option>
        <option value="30000">30 sec</name>
        <option value="60000">60 sec</option>
        <option value="90000">90 sec</option>
        <option value="180000">3 min</option>
      </select>
    </div>
    <div class="form-group">
      <label>Atomizer Spray Time</label>
      <select id="pulseOnTime" class="form-select" onchange="save('pulseOnTime')">
        <option value="2000">2 sec</option>
        <option value="3000">3 sec</option>
        <option value="4000">4 sec</option>
        <option value="5000">5 sec</option>
      </select>
    </div>
    <button class="btn btn-danger" onclick="reboot()">Restart Controller</button>
  </div>
  
  <a href="/">&larr; Return to Dashboard</a>

  <script>
    const db = new Dexie('EggubatorDB');
    db.version(3).stores({
      logs: 't, timeSec, bootId, temp, hum, h, a, f, s',
      bootTimestamps: 'bootId, startUnix, duration'
    });
    db.version(4).stores({
      logs: 't, timeSec, bootId, temp, hum, h, a, f, s',
      bootTimestamps: 'bootId, startUnix, duration'
    }).upgrade(async () => { await db.logs.clear(); });

    async function mainLoop() {
      try {
        const statusRes = await fetch('/status');
        const d = await statusRes.json();
        
        document.getElementById('ip').textContent = d.ip;
        document.getElementById('rssi').textContent = d.rssi;
        document.getElementById('heap').textContent = Math.round(d.heapFree/1024);
        document.getElementById('sector').textContent = d.currentSector;
        document.getElementById('startSector').textContent = d.startSector;
        document.getElementById('bootLogs').textContent = d.logsInCurrentBoot;
        
        const count = await db.logs.count();
        document.getElementById('dexieCount').textContent = count;
        
        document.getElementById('version').textContent = d.version;
        document.getElementById('uptimeSys').textContent = d.uptime;
        document.getElementById('mockEnable').checked = d.mock === 1;
        document.getElementById('autoSim').checked = d.autosim === 1;
        document.getElementById('mTemp').value = d.temperature.toFixed(1);
        document.getElementById('mHum').value = d.humidity.toFixed(1);
        
        document.getElementById('currentDayVal').textContent = (d.currentDay + 1);
        const sd = document.getElementById('startDateVal');
        if (d.startTimestamp > 0) {
          sd.textContent = new Date(d.startTimestamp * 1000).toLocaleDateString();
        } else {
          sd.textContent = 'Unknown';
        }
        
        document.getElementById('mockFields').style.display = d.mock ? 'block' : 'none';

        const mockRes = await fetch('/settings/api');
        const m = await mockRes.json();
        updateOneMinOption();
        document.getElementById('eggTurnInterval').value = m.eggTurnInterval;
        document.getElementById('eggTurnDuration').value = m.eggTurnDuration;
        document.getElementById('angleAdjustment').value = m.angleAdjustment;
        document.getElementById('logInterval').value = m.logInterval;
        document.getElementById('pulseOnTime').value = m.pulseOnTime;
      } catch (e) {
        console.error("Fetch error", e);
      } finally {
        setTimeout(mainLoop, 5000); 
      }
    }

    function updateOneMinOption() {
      const isMock = document.getElementById('mockEnable').checked;
      const isAuto = document.getElementById('autoSim').checked;
      const select = document.getElementById('eggTurnInterval');
      const opt5sec = document.getElementById('opt5sec');

      if (isMock || isAuto) {
        if (!opt5sec) {
            let opt = document.createElement('option');
            opt.value = '5000';
            opt.id = 'opt5sec';
            opt.textContent = '5 sec';
            select.appendChild(opt);
        }
      } else {
        if (opt5sec) {
            select.removeChild(opt5sec);
        }
      }
    }

    function toggle(key) {
      const isMock = document.getElementById('mockEnable').checked;
      const isAuto = document.getElementById('autoSim').checked;
      
      let endpoint = `/settings/api?`;
      if (key === 'enable') {
        endpoint += `enable=${isMock ? 1 : 0}`;
        if(isMock && isAuto) { document.getElementById('autoSim').checked = false; endpoint += `&autosim=0`; }
      } else {
        endpoint += `autosim=${isAuto ? 1 : 0}`;
        if(isAuto && isMock) { document.getElementById('mockEnable').checked = false; endpoint += `&enable=0`; }
      }
      
      document.getElementById('mockFields').style.display = document.getElementById('mockEnable').checked ? 'block' : 'none';
      updateOneMinOption();
      fetch(endpoint).then(() => fetch('/status'));
    }

    function save(key) {
      const val = document.getElementById(key).value;
      fetch(`/settings/api?${key}=${val}`).then(() => fetch('/status'));
    }
    
    function adjustAngle(delta) {
      const input = document.getElementById('angleAdjustment');
      let val = parseInt(input.value) + delta;
      if (val < -40) val = -40;
      if (val > 40) val = 40;
      input.value = val;
      save('angleAdjustment');
    }
    
    function setMock() {
      if (!document.getElementById('mockEnable').checked) return;
      const t = document.getElementById('mTemp').value;
      const h = document.getElementById('mHum').value;
      fetch(`/settings/api?temp=${t}&hum=${h}`).then(() => fetch('/status'));
    }
    
    function adjustDay(dir) {
      if(confirm('Are you sure you want to ' + (dir > 0 ? 'add' : 'subtract') + ' 1 day?')) {
        fetch(`/settings/api?action=adjustDay&dir=${dir}`).then(() => fetch('/status'));
      }
    }
    
    function startNewBatch() {
      if(confirm('WARNING: This will reset the incubator to Day 0. Are you sure?')) {
        const unixNow = Math.floor(Date.now() / 1000);
        fetch(`/settings/api?action=newBatch&timestamp=${unixNow}`).then(() => fetch('/status'));
      }
    }

    function reboot() { 
      if(confirm('Reboot device?')) {
        fetch('/reboot');
        document.body.innerHTML = '<div style="display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;color:#1877f2;font-weight:bold;">Restarting... Please wait.</div>';
        setTimeout(() => location.reload(), 10000);
      }
    }
    
    mainLoop();
   </script>
 </body>
 </html>
 )rawliteral";

const char DEXIE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>EGGubator - Dexie DB</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <script src="https://cdn.jsdelivr.net/npm/chart.js@4.4.1/dist/chart.umd.min.js"></script>
  <script src="https://cdn.jsdelivr.net/npm/dexie@3.2.4/dist/dexie.min.js"></script>
  <script src="https://cdn.jsdelivr.net/npm/indexeddb-debug-bar@latest/dist/browser/indexeddb-debug-bar-browser.umd.js"></script>
  <style>
    body { font-family: 'Segoe UI', sans-serif; margin: 0; background: #f0f2f5; }
    .header { background: linear-gradient(135deg, #1877f2, #166fe5); padding: 20px; color: white; text-align: center; }
    .header h1 { margin: 0; font-size: 24px; }
    .header a { color: white; text-decoration: none; font-weight: 700; }
    #idb-debug-bar { position: relative !important; }
  </style>
</head>
<body>
  <div class="header">
    <h1>🥚 EGGubator DB</h1>
    <a href="/">&larr; Dashboard</a>
  </div>
  <script>
    const db = new Dexie('EggubatorDB');
    db.version(3).stores({
      logs: 't, timeSec, bootId, temp, hum, h, a, f, s',
      bootTimestamps: 'bootId, startUnix, duration'
    });
    db.version(4).stores({
      logs: 't, timeSec, bootId, temp, hum, h, a, f, s',
      bootTimestamps: 'bootId, startUnix, duration'
    }).upgrade(async () => { await db.logs.clear(); });
    new IndexedDBDebugBar(db, { position: 'left', isCollapsed: false });
  </script>
</body>
</html>
)rawliteral";

#endif
