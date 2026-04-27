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
  <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.5.1/css/all.min.css">
  <script src="https://cdn.jsdelivr.net/npm/chart.js@4.4.1/dist/chart.umd.min.js"></script>
  <script src="https://cdn.jsdelivr.net/npm/hammerjs@2.0.8"></script>
  <script src="https://cdn.jsdelivr.net/npm/chartjs-plugin-zoom@2.0.1/dist/chartjs-plugin-zoom.min.js"></script>
  <script src="https://cdn.jsdelivr.net/npm/dexie@3.2.4/dist/dexie.min.js"></script>
  <style>
    :root {
      --primary: #1877f2;
      --primary-dark: #166fe5;
      --primary-soft: #e7f3ff;
      --bg: #fdfaf6;
      --card-bg: #ffffff;
      --text: #1c1e21;
      --text-muted: #65676b;
      --on: #42b72a;
      --off: #f02849;
      --idle: #8a8d91;
    }
    * { box-sizing: border-box; }
    body { font-family: 'Segoe UI', Roboto, Helvetica, Arial, sans-serif; margin: 0; background: var(--bg); color: var(--text); line-height: 1.5; }
    .container { max-width: 1100px; margin: 0 auto; padding: 10px; }
    .header { position: relative; display: flex; justify-content: center; align-items: center; margin-bottom: 24px; background: linear-gradient(135deg, var(--primary), var(--primary-dark)); padding: 20px 28px; border-radius: 20px; box-shadow: 0 4px 12px rgba(24, 119, 242, 0.3); color: white; }
    h1 { margin: 0; font-size: 26px; font-weight: 800; letter-spacing: -0.5px; color: white; text-align: center; }
    .card { background: var(--card-bg); padding: 5px; border-radius: 15px; box-shadow: 0 2px 10px rgba(0,0,0,0.04); margin-bottom: 5px; border: 1px solid rgba(0,0,0,0.05); }
    .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(120px, 1fr)); gap: 5px; }
    .stat-card { background: #ffffff; padding: 5px; border-radius: 12px; text-align: center; border: 1px solid #edf0f5; transition: all 0.2s ease; box-shadow: 0 1px 2px rgba(0,0,0,0.02); }
    .stat-label { font-size: 8px; color: var(--text-muted); font-weight: 800; text-transform: uppercase; letter-spacing: 1px; margin-bottom: 2px; }
    .stat-value { font-size: 16px; font-weight: 800; margin-top: 2px; }
    .target-val { font-size: 9px; color: #adb5bd; margin-top: 6px; font-weight: 600; }
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
    .refresh-control select { padding: 4px 8px; border-radius: 8px; background: rgba(255,255,255,0.2); border: 1px solid rgba(255,255,255,0.3); color: white; font-size: 12px; appearance: auto; -webkit-appearance: auto; }
    .refresh-control select option { color: black; }
    .chart-box { height: 400px; width: 100%; margin-top: 5px; position: relative; }
    .chart-box canvas { width: 100% !important; height: 100% !important; }
    .footer { text-align: center; font-size: 13px; color: var(--text-muted); margin-top: 40px; padding: 30px 0; border-top: 1px solid #e0e4e9; }
    .footer a { color: var(--primary); text-decoration: none; font-weight: 700; }
    h3 { margin: 0 0 5px 0; font-size: 18px; font-weight: 800; color: #333; display: flex; align-items: center; gap: 8px; }
    .icon { font-size: 24px; transition: all 0.3s ease; display: inline-block; color: #8a8d91; }
    .heater-active { color: #f39c12 !important; animation: bulb-glow 1.5s infinite alternate; }
    .fan-active { color: #42b72a !important; }
    .atomizer-active { color: #1877f2 !important; animation: spray-puff 0.5s infinite; }
    .atomizer-idle { filter: grayscale(100%); opacity: 0.5; color: #8a8d91; }
    @keyframes bulb-glow { 0% { filter: drop-shadow(0 0 2px #f39c12); } 100% { filter: drop-shadow(0 0 15px #f39c12); } }
    @keyframes spray-puff { 0% { transform: scale(1); opacity: 1; } 50% { transform: scale(1.2); opacity: 0.7; } 100% { transform: scale(1); opacity: 1; } }
  </style>
</head>
<body>
  <div class="container">
    <div class="header">
      <h1>🥚 EGGubator 🐣</h1>
    </div>

    <div class="card">
        <div style="display:flex; justify-content: space-between; align-items: center; margin-bottom: 24px;">
        <select id="stageSelect" class="badge badge-incubation" onchange="confirmStageChange()" style="border:none; outline:none; cursor:pointer; font-size:12px; font-weight:800; padding:6px 16px;">
          <option value="incubation">Incubation Stage (1-18 days)</option>
          <option value="lockdown">Lockdown Stage (19-21 days)</option>
        </select>
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
          <div style="display:flex; justify-content: space-evenly; align-items: center;">
             <div><div class="stat-label" style="font-size:7px; margin-bottom: 0;">Temp</div><div class="stat-value" style="font-size:12px; margin-top: 0;" id="avgTemp1h">--</div></div>
             <div style="width:1px; height:20px; background:#e0e4e9;"></div>
             <div><div class="stat-label" style="font-size:7px; margin-bottom: 0;">Hum</div><div class="stat-value" style="font-size:12px; margin-top: 0;" id="avgHum1h">--</div></div>
          </div>
        </div>
        <div class="stat-card">
          <div class="stat-label" style="margin-bottom: 6px;">4 Hours</div>
          <div style="display:flex; justify-content: space-evenly; align-items: center;">
             <div><div class="stat-label" style="font-size:7px; margin-bottom: 0;">Temp</div><div class="stat-value" style="font-size:12px; margin-top: 0;" id="avgTemp4h">--</div></div>
             <div style="width:1px; height:20px; background:#e0e4e9;"></div>
             <div><div class="stat-label" style="font-size:7px; margin-bottom: 0;">Hum</div><div class="stat-value" style="font-size:12px; margin-top: 0;" id="avgHum4h">--</div></div>
          </div>
        </div>
        <div class="stat-card">
          <div class="stat-label" style="margin-bottom: 6px;">8 Hours</div>
          <div style="display:flex; justify-content: space-evenly; align-items: center;">
             <div><div class="stat-label" style="font-size:7px; margin-bottom: 0;">Temp</div><div class="stat-value" style="font-size:12px; margin-top: 0;" id="avgTemp8h">--</div></div>
             <div style="width:1px; height:20px; background:#e0e4e9;"></div>
             <div><div class="stat-label" style="font-size:7px; margin-bottom: 0;">Hum</div><div class="stat-value" style="font-size:12px; margin-top: 0;" id="avgHum8h">--</div></div>
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

    const db = new Dexie('EggubatorDB');
    db.version(2).stores({
      logs: 't, timeSec, bootId, temp, hum, h, a, f, s'
    });

    async function cleanupDB() {
      const thirtyDaysAgo = Date.now() - (30 * 24 * 60 * 60 * 1000);
      await db.logs.where('t').below(thirtyDaysAgo).delete();
    }

    function decodeLogs(hex, logCount) {
      if (!hex) return [];
      const bytes = new Uint8Array(hex.match(/.{1,2}/g).map(byte => parseInt(byte, 16)));
      const entries = [];
      const now = Date.now();
      const bootStartEstimate = now - (currentUptimeSec * 1000);
      
      for (let i = 0; i < logCount; i++) {
        const offset = i * 8;
        if (offset + 8 > bytes.length) break;
        const timeSec = bytes[offset] | (bytes[offset+1] << 8) | (bytes[offset+2] << 16) | (bytes[offset+3] << 24);
        const temp = bytes[offset+4] / 10 + 20;
        const hum = bytes[offset+5];
        const states = bytes[offset+6];
        const bootId = bytes[offset+7];

        let absoluteT;
        if (bootId === currentBootId) {
          absoluteT = bootStartEstimate + (timeSec * 1000);
        } else {
          let diff = (currentBootId - bootId);
          if (diff < 0) diff += 256;
          absoluteT = bootStartEstimate - (diff * 86400 * 1000) + (timeSec * 1000);
        }

        entries.push({ t: absoluteT, timeSec: timeSec, bootId: bootId, temp, hum, h: states & 1, a: (states >> 1) & 1, f: (states >> 2) & 1, s: ((states >> 3) & 3) === 2 ? -1 : ((states >> 3) & 3) });
      }
      return entries;
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
            { label: 'Turner', borderColor: 'rgba(146, 94, 13, 0.5)', data: [], yAxisID: 'yControls', stepped: true, pointRadius: 0, borderWidth: 2 }
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
              title: { display: true, text: 'Time', font: { weight: 'bold' } }, 
              grid: { color: '#f0f0f0' },
              ticks: {
                callback: function(value) {
                   const now = Date.now();
                   const date = new Date(now + value * 1000);
                   return date.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', hour12: true });
                }
              }
            },
            yTemp: { type: 'linear', position: 'left', afterDataLimits: (s) => { let r = s.max - s.min; if(r===0)r=1; s.min -= r*0.35; s.max += r*0.05; }, title: { display: true, text: 'Temp °C', font: { weight: 'bold' } } },
            yHum: { type: 'linear', position: 'right', afterDataLimits: (s) => { let r = s.max - s.min; if(r===0)r=1; s.min -= r*0.35; s.max += r*0.05; }, title: { display: true, text: 'Hum %', font: { weight: 'bold' } }, grid: { display: false } },
            yControls: { type: 'linear', position: 'right', min: 0, max: 40, display: false }
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
                }
              }
            },
            zoom: { pan: { enabled: true, mode: 'x' }, zoom: { wheel: { enabled: true }, pinch: { enabled: true }, mode: 'x' } }
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
      
      const tIcon = document.getElementById('turnerIcon');
      tIcon.style.transform = (d.servo == 1 ? 'rotate(-45deg)' : (d.servo == 2 ? 'rotate(45deg)' : 'rotate(0deg)'));
      tIcon.style.color = (d.servo !== 0 ? 'var(--on)' : 'var(--idle)');

      const s = document.getElementById('stageSelect');
      s.value = d.stageLockdown ? 'lockdown' : 'incubation';
      s.className = 'badge ' + (d.stageLockdown ? 'badge-lockdown' : 'badge-incubation');
      
      calculateAverages();
    }

    async function calculateAverages() {
      const now = Date.now();
      const windows = [3600, 14400, 28800]; // 1h, 4h, 8h in seconds
      const ids = [['avgTemp1h', 'avgHum1h'], ['avgTemp4h', 'avgHum4h'], ['avgTemp8h', 'avgHum8h']];
      
      for (let i = 0; i < windows.length; i++) {
        const win = windows[i];
        const logs = await db.logs.where('t').above(now - win * 1000).toArray();
        if (logs.length > 0) {
          const avgTemp = logs.reduce((sum, l) => sum + l.temp, 0) / logs.length;
          const avgHum = logs.reduce((sum, l) => sum + l.hum, 0) / logs.length;
          document.getElementById(ids[i][0]).textContent = avgTemp.toFixed(1) + '°C';
          document.getElementById(ids[i][1]).textContent = avgHum.toFixed(1) + '%';
        }
      }
    }

    function confirmStageChange() {
      const s = document.getElementById('stageSelect');
      const val = s.value;
      if(confirm('Are you sure you want to change to ' + (val === 'lockdown' ? 'Lockdown' : 'Incubation') + ' Stage?')) {
        fetch('/settings/api?stageType=' + val)
          .then(() => update())
          .catch(e => { console.error("Stage update failed:", e); load(); });
      } else {
        // Revert dropdown to previous state if cancelled (via reload/fetch)
        update(); 
      }
    }

    async function updateChart() {
      const now = Date.now();
      const viewStart = now - (4 * 3600 * 1000); // 4 hours ago
      
      const logs = await db.logs.where('t').above(viewStart).toArray();
      if (logs.length > 0) {
        mainChart.data.datasets[0].data = logs.map(l => ({ x: Math.min(0, (l.t - now)/1000), y: l.temp }));
        mainChart.data.datasets[1].data = logs.map(l => ({ x: Math.min(0, (l.t - now)/1000), y: l.hum }));
        mainChart.data.datasets[2].data = logs.map(l => ({ x: Math.min(0, (l.t - now)/1000), y: l.h ? 1.0 : 0.0 }));
        mainChart.data.datasets[3].data = logs.map(l => ({ x: Math.min(0, (l.t - now)/1000), y: l.a ? 3.0 : 2.0 }));
        mainChart.data.datasets[4].data = logs.map(l => ({ x: Math.min(0, (l.t - now)/1000), y: l.f ? 5.0 : 4.0 }));
        mainChart.data.datasets[5].data = logs.map(l => ({ x: Math.min(0, (l.t - now)/1000), y: (l.s == -1 ? 6.0 : (l.s == 0 ? 7.0 : 8.0)) }));
        
        mainChart.update('none');
      }
    }

    async function fetchNextBatch(bootId, timeSec) {
      const r = await fetch('/data?boot=' + bootId + '&time=' + timeSec + '&count=200');
      const d = await r.json();
      const newEntries = decodeLogs(d.logs, d.sentCount);
      if (newEntries.length > 0) {
        await db.logs.bulkPut(newEntries);
      }
      if (d.sentCount >= 200) {
        const nextBootId = newEntries.length > 0 ? newEntries[newEntries.length-1].bootId : bootId;
        const nextTimeSec = newEntries.length > 0 ? newEntries[newEntries.length-1].timeSec : timeSec;
        await fetchNextBatch(nextBootId, nextTimeSec);
      } else {
        initialLoadDone = true;
        if (newEntries.length > 0) {
          latestBootId = newEntries[newEntries.length-1].bootId;
          latestTimeSec = newEntries[newEntries.length-1].timeSec;
        }
        await updateChart();
        await calculateAverages();
        await cleanupDB();
      }
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
            const dataRes = await fetch('/data?boot=' + latestBootId + '&time=' + latestTimeSec + '&count=200');
            const dataData = await dataRes.json();
            const newEntries = decodeLogs(dataData.logs, dataData.sentCount);
            if (newEntries.length > 0) {
              await db.logs.bulkPut(newEntries);
              latestBootId = newEntries[newEntries.length-1].bootId;
              latestTimeSec = newEntries[newEntries.length-1].timeSec;
              await updateChart();
              await calculateAverages();
              await cleanupDB();
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
      fetch(`/settings/api?logInterval=${rate}`);
    }

    function setStage() {
      const s = document.getElementById('stageSelect').value;
      fetch(`/settings/api?stageType=${s}`)
        .then(() => update())
        .catch(e => console.error("Stage update failed:", e));
    }

    initCharts();
    
    // Set initial refresh rate
    fetch('/settings/api')
      .then(r => r.json())
      .then(d => {
         refreshRate = d.logInterval;
         const sel = document.getElementById('refreshRate');
         if(sel) sel.value = refreshRate;
         mainLoop();
      })
      .catch(e => {
         console.error("Initial fetch failed:", e);
         mainLoop();
      });
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
    body { font-family: 'Segoe UI', sans-serif; padding: 20px; background: #f0f2f5; color: #1c1e21; }
    .card { background: white; padding: 28px; border-radius: 20px; max-width: 540px; margin: 0 auto 24px; box-shadow: 0 4px 15px rgba(0,0,0,0.06); }
    h3 { margin-top: 0; color: #1877f2; font-weight: 800; border-bottom: 2px solid #f0f2f5; padding-bottom: 12px; margin-bottom: 24px; }
    .row { margin-bottom: 18px; display: flex; justify-content: space-between; align-items: center; }
    input, select { padding: 12px; border-radius: 10px; border: 1px solid #e0e4e9; width: 140px; font-weight: 600; }
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
  </style>
</head>
<body>
  <div class="card">
    <h3>System Telemetry</h3>
    <div class="sys-grid">
      <div class="sys-item"><span>IP Address</span><span id="ip">--</span></div>
      <div class="sys-item"><span>WiFi RSSI</span><span id="rssi">-- dBm</span></div>
      <div class="sys-item"><span>Free RAM</span><span id="heap">-- KB</span></div>
      <div class="sys-item"><span>Current Sector</span><span id="sector">--</span></div>
      <div class="sys-item"><span>Logs Since Boot</span><span id="bootLogs">--</span></div>
      <div class="sys-item"><span>Dexie Records</span><span id="dexieCount">--</span></div>
      <div class="sys-item"><span>Firmware</span><span id="version">--</span></div>
      <div class="sys-item"><span>Uptime</span><span id="uptimeSys">--</span></div>
    </div>
  </div>

    <div class="card">
      <h3>Sensor Simulation</h3>
      <div class="row">
        <label>Use Mock Sensor</label>
        <input type="checkbox" id="mockEnable" style="width:24px; height:24px;" onchange="toggle('enable')">
      </div>
      <div class="row">
        <label>Physics Simulation</label>
        <input type="checkbox" id="autoSim" style="width:24px; height:24px;" onchange="toggle('autosim')">
      </div>
      <div id="mockFields">
        <div class="row">
          <label>Mock Temp (°C)</label>
          <input type="number" id="mTemp" step="0.1" value="37.5">
        </div>
        <div class="row">
          <label>Mock Hum (%)</label>
          <input type="number" id="mHum" step="0.1" value="60">
        </div>
        <button onclick="setMock()">Apply Simulation Values</button>
      </div>
    </div>

  <div class="card">
    <h3>Maintenance</h3>
        <div class="row">
          <label>Egg Turn Interval</label>
          <select id="eggTurnInterval" onchange="save('eggTurnInterval')">
            <option value="1800000">30 min</option>
            <option value="3600000">1 hour</option>
            <option value="5400000">1.5 hours</option>
            <option value="7200000">2 hours</option>
            <option value="9000000">2.5 hours</option>
            <option value="10800000">3 hours</option>
            <option value="12600000">3.5 hours</option>
            <option value="14400000">4 hours</option>
          </select>
        </div>
        <div class="row">
          <label>Log Heartbeat Interval</label>
          <select id="logInterval" onchange="save('logInterval')">
            <option value="5000">5 sec</option>
            <option value="10000">10 sec</option>
            <option value="30000">30 sec</option>
            <option value="60000">60 sec</option>
            <option value="90000">90 sec</option>
            <option value="180000">3 min</option>
          </select>
        </div>
        <div class="row">
          <label>Atomizer Spray Time</label>
          <select id="pulseOnTime" onchange="save('pulseOnTime')">
            <option value="2000">2 sec</option>
            <option value="3000">3 sec</option>
            <option value="4000">4 sec</option>
            <option value="5000">5 sec</option>
          </select>
        </div>
    <button class="danger" onclick="reboot()">Restart Controller</button>
  </div>
  
  <a href="/">&larr; Return to Dashboard</a>

  <script>
    const db = new Dexie('EggubatorDB');
    db.version(2).stores({
      logs: 't, timeSec, bootId, temp, hum, h, a, f, s'
    });

    async function mainLoop() {
      try {
        const statusRes = await fetch('/status');
        const d = await statusRes.json();
        
        document.getElementById('ip').textContent = d.ip;
        document.getElementById('rssi').textContent = d.rssi;
        document.getElementById('heap').textContent = Math.round(d.heapFree/1024);
        document.getElementById('sector').textContent = d.currentSector;
        document.getElementById('bootLogs').textContent = d.logsInCurrentBoot;
        
        const count = await db.logs.count();
        document.getElementById('dexieCount').textContent = count;
        
        document.getElementById('version').textContent = d.version;
        document.getElementById('uptimeSys').textContent = d.uptime;
        document.getElementById('mockEnable').checked = d.mock === 1;
        document.getElementById('autoSim').checked = d.autosim === 1;
        document.getElementById('mTemp').value = d.temperature.toFixed(1);
        document.getElementById('mHum').value = d.humidity.toFixed(1);
        
        document.getElementById('mockFields').style.display = d.mock ? 'block' : 'none';

        const mockRes = await fetch('/settings/api');
        const m = await mockRes.json();
        document.getElementById('eggTurnInterval').value = m.eggTurnInterval;
        document.getElementById('logInterval').value = m.logInterval;
        document.getElementById('pulseOnTime').value = m.pulseOnTime;
      } catch (e) {
        console.error("Fetch error", e);
      } finally {
        setTimeout(mainLoop, 5000); 
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
      fetch(endpoint).then(() => fetch('/status'));
    }

    function save(key) {
      const val = document.getElementById(key).value;
      fetch(`/settings/api?${key}=${val}`).then(() => fetch('/status'));
    }
    
    function setMock() {
      if (!document.getElementById('mockEnable').checked) return;
      const t = document.getElementById('mTemp').value;
      const h = document.getElementById('mHum').value;
      fetch(`/settings/api?temp=${t}&hum=${h}`).then(() => fetch('/status'));
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
    db.version(2).stores({ logs: 't, timeSec, bootId, temp, hum, h, a, f, s' });
    new IndexedDBDebugBar(db, { position: 'left', isCollapsed: false });
  </script>
</body>
</html>
)rawliteral";

#endif
