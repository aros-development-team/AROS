import fs from 'node:fs';
const expected=fs.readFileSync('RAM:paste-expected.bin');
let chunks=[],size=0,done=false;
function finish(ok,reason){if(done)return;done=true;clearTimeout(deadline);process.stdin.setRawMode(false);process.stdin.pause();const result=JSON.stringify({ok,reason,expected:expected.length,received:size});fs.writeFileSync('RAM:paste-result.json',result+'\n');console.log(result);process.exit(ok?0:1);}
const deadline=setTimeout(()=>finish(false,'timeout'),45000);
process.stdin.setRawMode(true);process.stdin.resume();
console.log('Paste now with Right-Amiga+V');
process.stdin.on('data',b=>{chunks.push(Buffer.from(b));size+=b.length;if(size>=expected.length)finish(Buffer.concat(chunks).equals(expected),'byte comparison');});
