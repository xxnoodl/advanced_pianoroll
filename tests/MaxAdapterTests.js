const assert=require('node:assert/strict'),vm=require('node:vm'),fs=require('node:fs'),path=require('node:path');
const protocol=require('../companion/clip-protocol');
let messages=[],notes=[{note_id:101,pitch:60,start_time:0,duration:1,velocity:80,mute:false,probability:.3,release_velocity:44}],selected=42,nextId=200,writes=0;
function LiveAPI(callback,target){this.id=target==='live_set view'?1:Number(target.replace('id ',''));}
LiveAPI.prototype.getstring=function(){return 'Clip';};
LiveAPI.prototype.get=function(key){if(key==='detail_clip')return ['id',selected];return [key==='is_midi_clip'?1:key==='is_recording'?0:key==='loop_start'?0:key==='loop_end'?4:'Clip'];};
LiveAPI.prototype.call=function(name,text){if(name==='get_all_notes_extended')return [JSON.stringify({notes})];++writes;
 if(name==='apply_note_modifications')JSON.parse(text).notes.forEach(n=>{notes[notes.findIndex(x=>x.note_id===n.note_id)]=n;});
 if(name==='add_new_notes')JSON.parse(text).notes.forEach(n=>notes.push({...n,note_id:nextId++}));
 if(name==='remove_notes_by_id'){const ids=Array.from(arguments).slice(1);notes=notes.filter(n=>!ids.includes(n.note_id));}
};
const context={LiveAPI,ClipProtocol:protocol,jsarguments:['bridge-max.js','C:/installed companion/clip-protocol.js'],include(file){assert.equal(file,'C:/installed companion/clip-protocol.js');},outlet(...args){messages.push(args);},JSON,Math,Number,String,Array,Object,Error,isFinite};
vm.createContext(context);vm.runInContext(fs.readFileSync(path.join(__dirname,'../companion/bridge-max.js'),'utf8'),context);
context.init();context.session('test-session');
let snapshot=JSON.parse(messages.findLast(x=>x[1]==='snapshot')[2]);
function req(){return {protocol:1,session:'test-session',requestId:'request-00001',clipId:'42',baseRevision:snapshot.revision,baseFingerprint:protocol.fingerprint(snapshot),notes:[{localId:1,sourceId:101,pitch:62,start_time:0,duration:1,velocity:80,mute:false}]};}
context.apply(JSON.stringify(req()));let ack=JSON.parse(messages.findLast(x=>x[1]==='result')[2]);assert(ack.ok);assert.equal(notes[0].pitch,62);assert.equal(notes[0].probability,.3);assert.equal(notes[0].release_velocity,44);
const before=writes;context.apply(JSON.stringify(req()));assert.equal(writes,before,'repeated request never replays mutations');
snapshot=JSON.parse(messages.findLast(x=>x[1]==='snapshot')[2]);const r=req();r.requestId='request-00002';selected=99;context.apply(JSON.stringify(r));ack=JSON.parse(messages.findLast(x=>x[1]==='result')[2]);assert(!ack.ok);assert.equal(writes,before,'changed clip rejects writes');
selected=42;notes[0].velocity=90;r.requestId='request-00003';context.apply(JSON.stringify(r));ack=JSON.parse(messages.findLast(x=>x[1]==='result')[2]);assert(!ack.ok);assert.equal(writes,before,'external edit rejects writes');
console.log('All Max adapter checks passed (mock Live API; native calls still require Live test).');
