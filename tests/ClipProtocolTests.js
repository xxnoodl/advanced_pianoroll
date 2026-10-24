const assert = require('node:assert/strict');
const protocol = require('../companion/clip-protocol');
function clip() { return {clipId:'42',session:'session-a',revision:7,loopStart:4,loopEnd:8,notes:[
    {note_id:101,pitch:60,start_time:4,duration:1,velocity:80.5,mute:0,probability:.4,velocity_deviation:12,release_velocity:55},
    {note_id:102,pitch:64,start_time:6,duration:1,velocity:100,mute:0},
    {note_id:103,pitch:48,start_time:0,duration:1,velocity:64,mute:0},
    {note_id:104,pitch:67,start_time:7.5,duration:2,velocity:64,mute:0}]}; }
function request(base) { return {protocol:1,session:base.session,clipId:base.clipId,baseRevision:base.revision,baseFingerprint:protocol.fingerprint(base),notes:protocol.editable(base).map((n,i)=>({localId:i+1,sourceId:n.note_id,pitch:n.pitch,start_time:n.start_time,duration:n.duration,velocity:Math.round(n.velocity),mute:!!n.mute}))}; }
let checks=0;
function test(name, fn) { fn(); ++checks; console.log('PASS: '+name); }
test('unchanged notes preserve fractional velocities and produce no writes',()=>{const b=clip(); assert.deepEqual(protocol.plan(b,b,request(b)),{modifications:[],additions:[],removals:[]});});
test('moving a note retains note ID and advanced properties',()=>{const b=clip(),r=request(b); r.notes[0].start_time=4.25;const p=protocol.plan(b,b,r);assert.equal(p.modifications[0].note_id,101);assert.equal(p.modifications[0].probability,.4);assert.equal(p.modifications[0].velocity,80.5);assert.equal(p.modifications[0].release_velocity,55);});
test('add/delete affect only editable notes',()=>{const b=clip(),r=request(b);r.notes.shift();r.notes.push({localId:3,sourceId:0,pitch:72,start_time:5,duration:.5,velocity:90,mute:false});const p=protocol.plan(b,b,r);assert.deepEqual(p.removals,[101]);assert.equal(p.additions.length,1);assert(!p.removals.includes(103));assert(!p.removals.includes(104));});
test('external note changes reject a stale edit',()=>{const b=clip(),c=clip();c.notes[0].velocity=90;assert.throws(()=>protocol.plan(b,c,request(b)),/changed/);});
test('loop changes reject a stale edit',()=>{const b=clip(),c=clip();c.loopEnd=12;assert.throws(()=>protocol.plan(b,c,request(b)),/changed/);});
test('another clip or session is rejected',()=>{const b=clip(),r=request(b);r.clipId='99';assert.throws(()=>protocol.plan(b,b,r));r.clipId='42';r.session='other';assert.throws(()=>protocol.plan(b,b,r));});
test('duplicate and unknown IDs are rejected',()=>{const b=clip(),r=request(b);r.notes[1].sourceId=101;assert.throws(()=>protocol.plan(b,b,r));r.notes[1].sourceId=999;assert.throws(()=>protocol.plan(b,b,r));});
test('out-of-range and non-finite notes are rejected',()=>{const b=clip(),r=request(b);r.notes[0].start_time=3;assert.throws(()=>protocol.plan(b,b,r));r.notes[0].start_time=NaN;assert.throws(()=>protocol.plan(b,b,r));});
test('malformed snapshots and capacity overflow are rejected',()=>{let b=clip();b.notes[1].note_id=101;assert.throws(()=>protocol.validateClip(b));b=clip();b.notes=Array.from({length:2049},(_,i)=>({...b.notes[0],note_id:i+1}));assert.throws(()=>protocol.validateClip(b),/capacity/);});
test('fingerprints ignore dictionary and note-list order',()=>{const b=clip(),c=clip();c.notes.reverse();assert.equal(protocol.fingerprint(b),protocol.fingerprint(c));});
console.log(checks+' clip protocol checks passed');
