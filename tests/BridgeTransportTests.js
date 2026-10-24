const assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path');
const {start}=require('../companion/bridge-node');
(async()=>{
 const handlers={},messages=[];
 const bridge=start({addHandler(k,fn){handlers[k]=fn;},outlet(...a){messages.push(a);}});
 try {
  handlers.snapshot(JSON.stringify({protocol:1,session:bridge.session,clipId:'42',revision:1,notes:[]}));
  assert.equal(JSON.parse(fs.readFileSync(path.join(bridge.folder,'snapshot.json'))).clipId,'42');
  fs.writeFileSync(path.join(bridge.folder,'request.json'),JSON.stringify({protocol:1,session:bridge.session,requestId:'request-one'}));
  await new Promise(r=>setTimeout(r,600));assert.equal(messages.filter(m=>m[0]==='apply').length,1);
  await new Promise(r=>setTimeout(r,350));assert.equal(messages.filter(m=>m[0]==='apply').length,1,'request delivered once');
  handlers.result(JSON.stringify({protocol:1,session:bridge.session,requestId:'request-one',ok:true}));
  assert(JSON.parse(fs.readFileSync(path.join(bridge.folder,'ack.json'))).ok);
  handlers.unavailable('Select a MIDI clip');assert(!JSON.parse(fs.readFileSync(path.join(bridge.folder,'heartbeat.json'))).available);
 } finally {bridge.stop();for(const name of fs.readdirSync(bridge.folder))fs.unlinkSync(path.join(bridge.folder,name));fs.rmdirSync(bridge.folder);}
 console.log('All local bridge transport checks passed.');
})().catch(e=>{console.error(e);process.exitCode=1;});
