const { chromium } = require('playwright');

(async () => {
  const url = 'https://exposure-copying-synthetic-freight.trycloudflare.com/';
  console.log('Opening browser for ' + url);
  const browser = await chromium.launch({ headless: true });
  const page = await browser.newPage();
  
  // Capture console messages
  const consoleLogs = [];
  page.on('console', msg => {
    consoleLogs.push({ type: msg.type(), text: msg.text() });
    console.log(`[${msg.type()}] ${msg.text()}`);
  });
  
  try {
    console.log('Navigating to dashboard...');
    await page.goto(url, { timeout: 60000 });
    
    console.log('Waiting for initial data sync (30s)...');
    await page.waitForTimeout(30000); 
    
    const loadingVisible = await page.isVisible('#loadingOverlay');
    console.log('\nLoading overlay visible:', loadingVisible);
    
    const progressText = await page.textContent('#loadingProgress').catch(() => 'N/A');
    console.log('Progress text:', progressText);
    
    const dbCount = await page.evaluate(async () => {
      if (typeof Dexie === 'undefined') return 'Dexie not loaded';
      const db = new Dexie('EggubatorDB');
      await db.open();
      return await db.logs.count();
    }).catch((e) => 'Error: ' + e.message);
    console.log('Dexie log count:', dbCount);

    const bootTimestampsCount = await page.evaluate(async () => {
      if (typeof Dexie === 'undefined') return 'Dexie not loaded';
      const db = new Dexie('EggubatorDB');
      await db.open();
      return await db.bootTimestamps.count();
    }).catch((e) => 'Error: ' + e.message);
    console.log('Dexie bootTimestamps count:', bootTimestampsCount);
    
  } catch (e) {
    console.error('Test failed:', e);
  } finally {
    await browser.close();
    console.log('\n=== Done ===');
  }
})();
