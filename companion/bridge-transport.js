'use strict';
const fs = require('node:fs');
const path = require('node:path');
const os = require('node:os');
const crypto = require('node:crypto');
const {spawn} = require('node:child_process');
function start(maxApi) {
    const folder = fs.mkdtempSync(path.join(os.tmpdir(),'pianoroll-live-'));
    const session = crypto.randomUUID();
    const log = text => {
        try { fs.appendFileSync(path.join(__dirname,'runtime.log'),new Date().toISOString()+' '+text+'\n'); } catch (_) {}
    };
    log('Session started: '+folder);
    let child, lastRequest='', available=false, eventOffset=0, eventTail='', lastEvent=0, lastBeat=0;
    const atomic = (name, value) => {
        const file = path.join(folder,name), temp = file+'.tmp';
        fs.writeFileSync(temp,JSON.stringify(value),'utf8');
        for(let attempt=0;;attempt++){
            try{fs.renameSync(temp,file);break;}
            catch(error){if(attempt>=4||!['EPERM','EACCES','EBUSY'].includes(error.code))throw error;Atomics.wait(new Int32Array(new SharedArrayBuffer(4)),0,0,2);}
        }
    };
    const status = text => { log(text); maxApi.outlet('nodeerror',String(text)); };
    const boot = () => maxApi.outlet('session',session,folder);
    maxApi.addHandler('boot',boot);
    maxApi.addHandler('snapshot',text=>{
        try {
            const snapshot=JSON.parse(text); if(snapshot.session!==session||snapshot.protocol!==1) throw Error('Wrong bridge session.');
            atomic('snapshot.json',snapshot); available=true;
            atomic('heartbeat.json',{protocol:1,session,timeMs:Date.now(),available:true});
        } catch(e) { status(e.message); }
    });
    maxApi.addHandler('telemetry',text=>{try{atomic('telemetry.json',JSON.parse(text));}catch(e){status(e.message);}});
    maxApi.addHandler('eventerror',message=>{atomic('event-status.json',{message:String(message),timeMs:Date.now()});status(message);});
    maxApi.addHandler('unavailable',message=>{available=false;atomic('heartbeat.json',{protocol:1,session,timeMs:Date.now(),available:false,message});});
    maxApi.addHandler('result',text=>{try{const result=JSON.parse(text);if(result.session===session) atomic('ack.json',result);}catch(e){status(e.message);}});
    maxApi.addHandler('openeditor',()=>{
        if(child && child.exitCode===null) {status('The shared editor is already open.');return;}
        const executable=path.join(__dirname,'bin','Piano Roll Clip Editor.exe');
        if(!fs.existsSync(executable)){status('Run companion/install.ps1 to install the clip editor.');return;}
        log('Launching editor: '+executable);
        child=spawn(executable,['--live-bridge',folder],{windowsHide:true,stdio:'ignore'});
        child.on('exit',(code,signal)=>{log('Editor exited: '+code+' '+signal);maxApi.outlet('command',JSON.stringify({session,kind:'panic'}));});
        child.on('error',e=>{status('Editor launch failed: '+e.message);child=null;});
    });
    const timer=setInterval(()=>{
        try {
            if(Date.now()-lastBeat>500){atomic('heartbeat.json',{protocol:1,session,timeMs:Date.now(),available});lastBeat=Date.now();}
            const events=path.join(folder,'events.jsonl');
            if(fs.existsSync(events)){
                const size=fs.statSync(events).size;
                if(size>eventOffset){
                    const bytes=Buffer.alloc(Math.min(65536,size-eventOffset));const fd=fs.openSync(events,'r');
                    let length;try{length=fs.readSync(fd,bytes,0,bytes.length,eventOffset);}finally{fs.closeSync(fd);}
                    eventOffset+=length;const lines=(eventTail+bytes.toString('utf8',0,length)).split('\n');eventTail=lines.pop();
                    for(const line of lines)if(line.trim()){const event=JSON.parse(line);if(event.session===session&&event.sequence>lastEvent){lastEvent=event.sequence;maxApi.outlet('command',JSON.stringify(event));}}
                }
            }
            const file=path.join(folder,'request.json');
            if(fs.existsSync(file)&&fs.statSync(file).size<8*1024*1024){
                const req=JSON.parse(fs.readFileSync(file,'utf8'));
                if(req.session===session&&req.requestId&&req.requestId!==lastRequest){lastRequest=req.requestId;maxApi.outlet('apply',JSON.stringify(req));}
            }
        } catch(e){status(e.message);}
    },20);
    atomic('heartbeat.json',{protocol:1,session,timeMs:Date.now(),available:false});
    boot();
    return {folder,session,stop(){clearInterval(timer);atomic('heartbeat.json',{protocol:1,session,timeMs:0,available:false});}};
}
module.exports={start};
