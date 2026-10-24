const clone=value=>JSON.parse(JSON.stringify(value));
function makeLive(){
 const state={nodes:{},writes:[],midi:[],nextClip:500,nextNote:5000,view:'Session'};
 const add=(id,type,fields)=>state.nodes[id]={type,id,...fields};
 add(1,'Song',{is_playing:0,tempo:120,current_song_time:0,record_mode:0,session_record:0});
 add(2,'Song.View',{selected_track:20,detail_clip:201,highlighted_clip_slot:200});add(3,'MaxDevice',{canonical_parent:10});add(4,'Application.View',{});
 const track=(id,name,audio)=>add(id,'Track',{name,canonical_parent:1,has_midi_input:1,has_audio_output:audio,has_midi_output:audio?0:1,is_foldable:0,is_frozen:0,implicit_arm:0,arm:0,back_to_arranger:0,clip_slots:[],arrangement_clips:[],output_routing_type:{display_name:'No Output',identifier:'none'},output_routing_channel:{display_name:'No Output',identifier:'nonech'}});
 track(10,'Clip editor',0);track(20,'Synth A',1);track(30,'Synth B',1);
 function slot(id,owner,clip=0){add(id,'ClipSlot',{canonical_parent:owner,clip,has_clip:clip?1:0});state.nodes[owner].clip_slots.push(id);}
 slot(100,10);slot(200,20,201);slot(202,20);slot(300,30,301);slot(302,30);
 const note={note_id:101,pitch:60,start_time:0,duration:1,velocity:80,mute:false,probability:.3,release_velocity:44};
 function clip(id,parent,notes=[],length=4,arrangement=false,start=0){return add(id,'Clip',{canonical_parent:parent,name:'Clip '+id,is_midi_clip:1,is_recording:0,loop_start:0,loop_end:length,notes:clone(notes),is_session_clip:arrangement?0:1,is_playing:0,playing_position:0,start_time:start,end_time:start+length});}
 clip(201,200,[note]);clip(301,300,[{...note,note_id:102,pitch:67}]);
 function API(callback,target){let id=typeof target==='number'?target:({'live_set':1,'live_set view':2,'this_device':3,'live_app view':4}[target]||Number(String(target).replace('id ','')));if(!state.nodes[id])throw Error('Unknown Live object '+target);this.id=id;}
 Object.defineProperty(API.prototype,'type',{get(){return state.nodes[this.id].type;}});
 API.prototype.getstring=function(key){if(!(key in state.nodes[this.id]))throw Error('Unknown string '+key);return String(state.nodes[this.id][key]);};
 API.prototype.get=function(key){const n=state.nodes[this.id];
  if(key==='available_output_routing_types')return [JSON.stringify({[key]:[20,30].map(id=>({display_name:state.nodes[id].name,identifier:'track-'+id}))})];
  if(key==='available_output_routing_channels')return [JSON.stringify({[key]:[{display_name:'Track In',identifier:'in'}]})];
  if(!(key in n))throw Error('Unknown property '+key+' on '+n.type);
  if(key.startsWith('output_routing'))return [JSON.stringify({[key]:n[key]})];
  if(['clip_slots','arrangement_clips'].includes(key))return n[key].flatMap(id=>['id',id]);
  if(['selected_track','detail_clip','highlighted_clip_slot','canonical_parent','clip'].includes(key))return ['id',n[key]];
  return [n[key]];
 };
 API.prototype.set=function(key,...args){const n=state.nodes[this.id];state.writes.push({id:this.id,key,args});
  if(key.startsWith('output_routing')){if(!Array.isArray(args[0]))throw Error('Routing needs a JSON dictionary atom list');const wrapper=JSON.parse(args[0][0]);if(!wrapper[key])throw Error('Missing routing property wrapper');n[key]=wrapper[key];if(key==='output_routing_type')n.output_routing_channel={display_name:'Track In',identifier:'in'};}
  else n[key]=Array.isArray(args[0])&&args[0][0]==='id'?args[0][1]:args[0]==='id'?args[1]:args[0];
 };
 API.prototype.call=function(method,...args){const n=state.nodes[this.id];
  if(method==='get_all_notes_extended')return [JSON.stringify({notes:n.notes})];
  if(method==='is_view_visible')return [state.view===args[0]?1:0];
  state.writes.push({id:this.id,method,args});
  if(method==='apply_note_modifications')JSON.parse(args[0]).notes.forEach(note=>{const index=n.notes.findIndex(x=>x.note_id===note.note_id);if(index<0)throw Error('Unknown note');n.notes[index]=note;});
  else if(method==='add_new_notes')JSON.parse(args[0]).notes.forEach(note=>n.notes.push({...note,note_id:state.nextNote++}));
  else if(method==='remove_notes_by_id')n.notes=n.notes.filter(note=>!args.includes(note.note_id));
  else if(method==='create_clip'){if(n.has_clip)throw Error('Slot occupied');const id=state.nextClip++;clip(id,this.id,[],args[0]);n.has_clip=1;n.clip=id;}
  else if(method==='create_midi_clip'){const id=state.nextClip++;clip(id,this.id,[],args[1],true,args[0]);n.arrangement_clips.push(id);}
  else if(method==='fire'){state.nodes[1].is_playing=1;n.is_playing=1;}
  else if(method==='stop_playing'){state.nodes[1].is_playing=0;Object.values(state.nodes).filter(x=>x.type==='Clip').forEach(c=>c.is_playing=0);}
  else if(method==='start_playing'){state.nodes[1].is_playing=1;Object.values(state.nodes).filter(x=>x.type==='Clip'&&!x.is_session_clip&&state.nodes[1].current_song_time>=x.start_time&&state.nodes[1].current_song_time<x.end_time).forEach(c=>c.is_playing=1);}
  else if(!['apply_note_modifications','add_new_notes','remove_notes_by_id'].includes(method))throw Error('Unknown call '+method);
 };
 state.API=API;state.select=(track,clipId=0,slotId=0)=>Object.assign(state.nodes[2],{selected_track:track,detail_clip:clipId,highlighted_clip_slot:slotId});
 return state;
}
module.exports={makeLive};
