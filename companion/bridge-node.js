'use strict';
const fs = require('node:fs');
const path = require('node:path');
const os = require('node:os');
const crypto = require('node:crypto');
const {spawn} = require('node:child_process');
function start(maxApi) {
    const folder = fs.mkdtempSync(path.join(os.tmpdir(),'pianoroll-live-'));
    const session = crypto.randomUUID();
    let child, lastRequest='', available=false;
    const atomic = (name, value) => {
        const file = path.join(folder,name), temp = file+'.tmp';
        fs.writeFileSync(temp,JSON.stringify(value),'utf8'); fs.renameSync(temp,file);
    };
    const status = text => maxApi.outlet('nodeerror',String(text));
    const boot = () => maxApi.outlet('session',session,folder);
    maxApi.addHandler('boot',boot);
    maxApi.addHandler('snapshot',text=>{
        try {
            const snapshot=JSON.parse(text); if(snapshot.session!==session||snapshot.protocol!==1) throw Error('Wrong bridge session.');
            atomic('snapshot.json',snapshot); available=true;
            atomic('heartbeat.json',{protocol:1,session,timeMs:Date.now(),available:true});
        } catch(e) { status(e.message); }
    });
    maxApi.addHandler('unavailable',message=>{available=false;atomic('heartbeat.json',{protocol:1,session,timeMs:Date.now(),available:false,message});});
    maxApi.addHandler('result',text=>{try{const result=JSON.parse(text);if(result.session===session) atomic('ack.json',result);}catch(e){status(e.message);}});
    maxApi.addHandler('openeditor',()=>{
        if(child && child.exitCode===null) {status('The shared editor is already open.');return;}
        const executable=path.join(__dirname,'bin','Piano Roll Clip Editor.exe');
        if(!fs.existsSync(executable)){status('Run companion/install.ps1 to install the clip editor.');return;}
        child=spawn(executable,['--live-bridge',folder],{windowsHide:true,stdio:'ignore'});
        child.on('error',e=>{status('Editor launch failed: '+e.message);child=null;});
    });
    const timer=setInterval(()=>{
        try {
            atomic('heartbeat.json',{protocol:1,session,timeMs:Date.now(),available});
            const file=path.join(folder,'request.json');
            if(fs.existsSync(file)&&fs.statSync(file).size<8*1024*1024){
                const req=JSON.parse(fs.readFileSync(file,'utf8'));
                if(req.session===session&&req.requestId&&req.requestId!==lastRequest){lastRequest=req.requestId;maxApi.outlet('apply',JSON.stringify(req));}
            }
        } catch(e){status(e.message);}
    },250);
    boot();
    return {folder,session,stop(){clearInterval(timer);atomic('heartbeat.json',{protocol:1,session,timeMs:0,available:false});}};
}
module.exports={start};
if(require.main===module) start(require('max-api'));
