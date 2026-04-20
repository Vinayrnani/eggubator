#ifndef WEB_UI_H
#define WEB_UI_H

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
      
      const minT = 27, maxT = 40;
      
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
      
      const minH = 0, maxH = 100;
      
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
          let val;
          if (dev.key === 's') {
            val = p.s === -1 ? 0 : (p.s === 1 ? 1 : 0.5);
          } else {
            val = p[dev.key] ? 1 : 0;
          }
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
    <h1><span>🥚</span> Settings</h1>
    <div class="header">
      <a href="/" class="back-link">← Back</a>
    </div>
    <div class="card">
      <span class="label">Logging Settings</span>
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
        <label>Save to Flash:</label>
        <select id="saveFlashInterval" onchange="saveFlashInterval()">
          <option value="3600000">60 min</option>
          <option value="5400000">90 min</option>
          <option value="7200000">120 min</option>
          <option value="9000000">150 min</option>
          <option value="10800000">180 min</option>
          <option value="12600000">210 min</option>
          <option value="14400000">240 min</option>
        </select>
      </div>
      <div class="input-row">
        <label>Egg Turner Interval:</label>
        <select id="eggTurnInterval" onchange="saveEggTurnInterval()">
          <option value="7200000">2 hours</option>
          <option value="10800000">3 hours</option>
          <option value="14400000">4 hours</option>
          <option value="21600000">6 hours</option>
        </select>
      </div>
    </div>
    <div class="header">
      <div class="mode-group">
        <span class="mode-text" id="mockText">OFF</span>
        <label class="switch">
          <input type="checkbox" id="mockSwitch" onchange="toggleMock()">
          <span class="slider"></span>
        </label>
      </div>
      <div class="mode-group">
        <span class="mode-text" id="autoSimText">AUTO-SIM</span>
        <label class="switch">
          <input type="checkbox" id="autoSimSwitch" onchange="toggleAutoSim()">
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
      <span class="label">Incubation Stage</span>
      <div class="input-row">
        <label>Stage:</label>
        <select id="incubationStage" onchange="saveIncubationStage()">
          <option value="incubation">Incubation (Days 1-18)</option>
          <option value="lockdown">Lockdown/Hatching (Days 19-21)</option>
        </select>
      </div>
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
            document.getElementById('stageText').textContent = d.stageLockdown ? 'LOCKDOWN' : 'INCUBATION';
            document.getElementById('stageSwitch').checked = d.stageLockdown;
            document.getElementById('incubationStage').value = d.stageLockdown ? 'lockdown' : 'incubation';
        }
      }).catch(e => console.error(e));
    }
    function loadMockValues() {
      fetch('/settings/api').then(r => r.json()).then(d => {
        document.getElementById('mockTemp').value = d.temp;
        document.getElementById('mockHum').value = d.hum;
      }).catch(e => console.error(e));
    }
    function onInputFocus() { userEditing = true; }
    function onInputBlur() { userEditing = false; }
    function toggleMock() {
      const enable = document.getElementById('mockSwitch').checked;
      fetch('/settings/api?enable=' + (enable ? '1' : '0')).then(r => r.text()).then(() => { userEditing = false; updateData(); loadMockValues(); }).catch(e => console.error(e));
    }
    function saveLogInterval() {
      const val = document.getElementById('logInterval').value;
      fetch('/settings/api?logInterval=' + val).then(r => r.text()).then(msg => console.log(msg)).catch(e => console.error(e));
    }
    function saveFlashInterval() {
      const val = document.getElementById('saveFlashInterval').value;
      fetch('/settings/api?saveFlashInterval=' + val).then(r => r.text()).then(msg => console.log(msg)).catch(e => console.error(e));
    }
    function toggleAutoSim() {
      const enable = document.getElementById('autoSimSwitch').checked;
      fetch('/settings/api?autosim=' + (enable ? '1' : '0')).then(r => r.text()).then(() => { updateData(); }).catch(e => console.error(e));
    }
    function toggleStage() {
      const enable = document.getElementById('stageSwitch').checked;
      fetch('/settings/api?stage=' + (enable ? '1' : '0')).then(r => r.text()).then(() => { updateData(); }).catch(e => console.error(e));
    }
    function saveIncubationStage() {
      const stage = document.getElementById('incubationStage').value;
      fetch('/settings/api?stageType=' + stage).then(r => r.text()).then(msg => { updateData(); }).catch(e => console.error(e));
    }
    function saveEggTurnInterval() {
      const val = document.getElementById('eggTurnInterval').value;
      fetch('/settings/api?eggTurnInterval=' + val).then(r => r.text()).then(msg => console.log(msg)).catch(e => console.error(e));
    }
    function setMockValues() {
      const t = document.getElementById('mockTemp').value;
      const h = document.getElementById('mockHum').value;
      if (t < 20 || t > 45 || h < 0 || h > 100) { document.getElementById('mockStatus').textContent = 'Invalid: Temp 20-45°C, Hum 0-100%'; return; }
      fetch('/settings/api?temp=' + t + '&hum=' + h).then(r => r.text()).then(msg => { document.getElementById('mockStatus').textContent = msg; updateData(); }).catch(e => console.error(e));
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

#endif
