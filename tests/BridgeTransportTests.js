const assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path');
const vm=require('node:vm');
let importedStart=false;
vm.runInNewContext(fs.readFileSync(path.join(__dirname,'../companion/bridge-node.js'),'utf8'),{
 require(name){if(name==='max-api')return {};if(name==='./bridge-transport')return {start(){importedStart=true;}};throw Error(name);}
});
assert(importedStart,'Max import starts launcher without require.main or environment guards');
const {start}=require('../companion/bridge-transport');
(async()=>{
 const handlers={},messages=[];
 const bridge=start({addHandler(k,fn){handlers[k]=fn;},outlet(...a){messages.push(a);}});
 try {
  handlers.snapshot(JSON.stringify({protocol:1,session:bridge.session,clipId:'42',revision:1,notes:[]}));
  assert.equal(JSON.parse(fs.readFileSync(path.join(bridge.folder,'snapshot.json'))).clipId,'42');
  fs.writeFileSync(path.join(bridge.folder,'request.json'),JSON.stringify({protocol:1,session:bridge.session,requestId:'request-one'}));
  await new Promise(r=>setTimeout(r,600));assert.equal(messages.filter(m=>m[0]==='apply').length,1);
  await new Promise(r=>setTimeout(r,350));assert.equal(messages.filter(m=>m[0]==='apply').length,1,'request delivered once');
  const events=path.join(bridge.folder,'events.jsonl');
  fs.writeFileSync(events,JSON.stringify({protocol:1,session:bridge.session,sequence:1,kind:'transport'})+'\n'+JSON.stringify({protocol:1,session:bridge.session,sequence:2,kind:'audition',pitch:60,on:true}).slice(0,20));
  await new Promise(r=>setTimeout(r,60));assert.equal(messages.filter(m=>m[0]==='command').length,1,'partial event waits for a complete line');
  fs.appendFileSync(events,JSON.stringify({protocol:1,session:bridge.session,sequence:2,kind:'audition',pitch:60,on:true}).slice(20)+'\n');
  await new Promise(r=>setTimeout(r,80));assert.equal(messages.filter(m=>m[0]==='command').length,2,'MIDI events are delivered once in order');
  handlers.telemetry(JSON.stringify({clipId:'42',playing:true,playhead:1,tempo:120,timeMs:Date.now()}));
  assert(JSON.parse(fs.readFileSync(path.join(bridge.folder,'telemetry.json'))).playing);
  handlers.result(JSON.stringify({protocol:1,session:bridge.session,requestId:'request-one',ok:true}));
  assert(JSON.parse(fs.readFileSync(path.join(bridge.folder,'ack.json'))).ok);
  handlers.unavailable('Select a MIDI clip');assert(!JSON.parse(fs.readFileSync(path.join(bridge.folder,'heartbeat.json'))).available);
 } finally {bridge.stop();for(const name of fs.readdirSync(bridge.folder))fs.unlinkSync(path.join(bridge.folder,name));fs.rmdirSync(bridge.folder);}
 console.log('All local bridge transport checks passed.');
})().catch(e=>{console.error(e);process.exitCode=1;});
