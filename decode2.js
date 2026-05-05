const fs = require('fs');
const hex = fs.readFileSync('hex.txt', 'utf8').trim();
let turner1 = 0, turner2 = 0, turner0 = 0;
for (let i = 0; i < hex.length; i += 16) {
  let chunk = hex.substr(i, 16);
  if (chunk.length < 16) break;
  let states = parseInt(chunk.substr(12, 2), 16);
  let turner = (states >> 3) & 0x03;
  if (turner === 1) turner1++;
  else if (turner === 2) turner2++;
  else turner0++;
}
console.log(`Turner 0: ${turner0}, Turner 1: ${turner1}, Turner 2: ${turner2}`);
