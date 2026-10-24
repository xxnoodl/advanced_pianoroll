/* Max Live API entry point, always called at low priority. */
autowatch=1; inlets=1; outlets=3;
include(typeof jsarguments!=='undefined'&&jsarguments.length>1?String(jsarguments[1]):'clip-protocol.js');
include(typeof jsarguments!=='undefined'&&jsarguments.length>2?String(jsarguments[2]):'live-workflow.js');
var bridgeReady=false,bridgeSession='',baseline=null,serial=0,lastFingerprint='',replies={},known={},order=[],workflow=null;
function status(text){outlet(1,'set',String(text));}
function init(){workflow=LiveWorkflow.create(LiveAPI,function(bytes){outlet(2,bytes);},status);bridgeReady=true;outlet(0,'boot');status('Select a MIDI instrument track and start writing.');}
function session(id){bridgeSession=String(id);if(bridgeReady)loadclip();}
function version(clip){
 ClipProtocol.validateClip(clip);clip.protocol=1;clip.session=bridgeSession;clip.revision=++serial;clip.fingerprint=ClipProtocol.fingerprint(clip);
 var key=clip.clipId+':'+clip.revision;known[key]=clip;order.push(key);while(order.length>64)delete known[order.shift()];return clip;
}
function latest(id){for(var i=order.length-1;i>=0;i--){var item=known[order[i]];if(item&&String(item.clipId)===String(id))return item;}return null;}
function publish(force){
 if(!bridgeReady||!bridgeSession)return;
 var clip=workflow.target(),fp=ClipProtocol.fingerprint(clip);
 if(!force&&fp===lastFingerprint)return;
 baseline=version(clip);lastFingerprint=fp;outlet(0,'snapshot',JSON.stringify(baseline));
 status(baseline.isDraft?'New clip ready on '+baseline.name:'Editing '+baseline.name);
}
function telemetry(){if(workflow&&baseline)outlet(0,'telemetry',JSON.stringify(workflow.telemetry(baseline)));}
function loadclip(){try{publish(true);telemetry();}catch(e){status(e.message);outlet(0,'unavailable',e.message);}}
function poll(){try{if(workflow)workflow.tick();publish(false);telemetry();}catch(e){lastFingerprint='';status(e.message);outlet(0,'unavailable',e.message);}}
function openeditor(){poll();outlet(0,'openeditor');}
function result(value){outlet(0,'result',JSON.stringify(value));}
function matching(n,r){return n.pitch===r.pitch&&Math.abs(n.start_time-r.start_time)<1e-7&&Math.abs(n.duration-r.duration)<1e-7&&Math.round(n.velocity)===r.velocity&&!!n.mute===r.mute;}
function apply(text){
 var request,reply;
 try{
  request=JSON.parse(String(text));
  if(typeof request.requestId!=='string'||!/^[a-zA-Z0-9-]{8,80}$/.test(request.requestId))throw Error('Invalid edit request.');
  if(replies[request.requestId]){result(replies[request.requestId]);return;}
  var base=known[request.clipId+':'+request.baseRevision];if(!base)throw Error('The clip snapshot expired. Reload this track.');
  var current=workflow.bound(base),diff=ClipProtocol.plan(base,current,request),api=workflow.materialize(base);
  if(diff.modifications.length)api.call('apply_note_modifications',JSON.stringify({notes:diff.modifications}));
  if(diff.additions.length)api.call('add_new_notes',JSON.stringify({notes:diff.additions}));
  if(diff.removals.length)api.call.apply(api,['remove_notes_by_id'].concat(diff.removals));
  var after=workflow.captureClip(api),editable=ClipProtocol.editable(after),used={},old={},mapping=[];
  base.notes.forEach(function(n){old[n.note_id]=true;});
  request.notes.forEach(function(r){var match=null;for(var i=0;i<editable.length;i++){var n=editable[i];if(!used[n.note_id]&&matching(n,r)&&(r.sourceId?n.note_id===r.sourceId:!old[n.note_id])){match=n;break;}}
   if(!match)throw Error('Live did not confirm every edit. Inspect the clip, then reload.');used[match.note_id]=true;mapping.push({localId:r.localId,sourceId:match.note_id});
  });
  if(editable.length!==request.notes.length)throw Error('Live returned an unexpected note list. Reload the clip.');
  after=version(after);
  try{var selected=workflow.target();
   if(String(selected.clipId)===String(after.clipId)){baseline=after;lastFingerprint=after.fingerprint;outlet(0,'snapshot',JSON.stringify(after));}
   else publish(false);
  }catch(selectionError){/* The bound clip was saved even if no other editable target is selected. */}
  reply={protocol:1,session:bridgeSession,requestId:request.requestId,ok:true,revision:after.revision,snapshot:after,sourceIds:mapping,message:'Synced to '+after.name};
 }catch(e){reply={protocol:1,session:bridgeSession,requestId:request?request.requestId:'',ok:false,message:e.message};status(e.message);}
 if(reply.requestId)replies[reply.requestId]=reply;result(reply);
}
function command(text){
 var event;
 try{event=JSON.parse(String(text));if(event.session!==bridgeSession)return;var base=latest(event.targetId);workflow.command(event,base);if(event.kind==='transport'){publish(false);telemetry();}}
 catch(e){if(workflow&&event&&event.kind==='audition')workflow.cleanup();status(e.message);outlet(0,'eventerror',e.message);}
}
function notifydeleted(){if(workflow)workflow.cleanup();}
function nodeerror(text){status(text);}
