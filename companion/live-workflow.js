/* Live-specific workflow. ES5 for Max's legacy js engine. */
(function(root){
'use strict';
function create(LiveAPI, emitMidi, report) {
    var route=null,held={},lastEditor=0;
    function api(id){return new LiveAPI(null,typeof id==='string' && !/^[0-9]+$/.test(id)?id:'id '+id);}
    function scalar(v){return Array.isArray(v)?v[v.length-1]:v;}
    function get(a,k){return scalar(a.get(k));}
    function name(a){return String(a.getstring('name')).replace(/^"|"$/g,'');}
    function dict(v){if(v && !Array.isArray(v) && typeof v==='object')return v;if(Array.isArray(v)&&v[0]==='dictionary')return JSON.parse(new Dict(v[1]).stringify());if(Array.isArray(v)&&v.length===1)v=v[0];return JSON.parse(String(v));}
    function routing(a,key){var value=dict(a.get(key));return value[key]||value;}
    function setRouting(a,key,value){var payload={};payload[key]=value;a.set(key,[JSON.stringify(payload)]);}
    function selectClip(view,id){try{view.set('detail_clip',['id',id]);}catch(_){} }
    function parentTrack(a){for(var i=0;i<8;i++){if(a.type==='Track')return a;var id=Number(get(a,'canonical_parent'));if(!id)break;a=api(id);}throw Error('The target track is no longer available.');}
    function helper(){return parentTrack(api('this_device'));}
    function selectedTrack(){var t=api(Number(get(api('live_set view'),'selected_track')));if(!Number(get(t,'has_midi_input'))||Number(get(t,'is_foldable'))||Number(get(t,'is_frozen')))throw Error('Select an unfrozen MIDI instrument track.');return t;}
    function ids(value){var result=[];for(var i=0;i<value.length;i++)if(value[i]==='id'&&Number(value[i+1]))result.push(Number(value[++i]));return result;}
    function arranged(track){return ids(track.get('arrangement_clips')).map(function(id){return api(id);});}
    function arrangementDraft(track,position,length){return {clipId:'arrangement:'+track.id+':'+Math.round(position*960),isDraft:true,isArrangement:true,trackId:String(track.id),position:position,name:name(track)+' (new clip)',loopStart:0,loopEnd:length,notes:[]};}
    function draft(slot,track){return {clipId:'empty:'+slot.id,isDraft:true,slotId:String(slot.id),trackId:String(track.id),name:name(track)+' (new clip)',loopStart:0,loopEnd:16,notes:[]};}
    function captureClip(c){
        if(!Number(get(c,'is_midi_clip')))throw Error('The selected clip is not MIDI.');
        if(Number(get(c,'is_recording')))throw Error('Finish recording this clip before editing it.');
        var track=parentTrack(c),data=dict(c.call('get_all_notes_extended'));
        return {clipId:String(c.id),trackId:String(track.id),name:name(c),loopStart:Number(get(c,'loop_start')),loopEnd:Number(get(c,'loop_end')),notes:data.notes};
    }
    function target(){
        var view=api('live_set view'),track=selectedTrack(),id=Number(get(view,'detail_clip'));
        if(track.id===helper().id && !Number(get(track,'has_audio_output')))throw Error('Select the MIDI track you want to compose for.');
        if(id){var clip=api(id);if(parentTrack(clip).id===track.id)return captureClip(clip);}
        if(Number(scalar(api('live_app view').call('is_view_visible','Arranger')))){
            var position=Math.max(0,Math.round(Number(get(api('live_set'),'current_song_time'))*960)/960),length=16,clips=arranged(track);
            for(var c=0;c<clips.length;c++){var start=Number(get(clips[c],'start_time')),finish=Number(get(clips[c],'end_time'));if(position>=start&&position<finish)return captureClip(clips[c]);if(start>position)length=Math.min(length,start-position);}
            return arrangementDraft(track,position,length);
        }
        id=Number(get(view,'highlighted_clip_slot'));
        if(id){var slot=api(id);if(parentTrack(slot).id===track.id){if(Number(get(slot,'has_clip')))return captureClip(api(Number(get(slot,'clip'))));return draft(slot,track);}}
        var slots=track.get('clip_slots');
        for(var i=0;i<slots.length;i++)if(slots[i]==='id' && Number(slots[i+1])){var empty=api(Number(slots[++i]));if(!Number(get(empty,'has_clip')))return draft(empty,track);}
        throw Error('Select an empty Session slot or an existing MIDI clip.');
    }
    function bound(base){if(!base.isDraft)return captureClip(api(Number(base.clipId)));if(base.isArrangement){var track=api(Number(base.trackId)),clips=arranged(track);for(var i=0;i<clips.length;i++)if(Number(get(clips[i],'start_time'))<base.position+base.loopEnd&&Number(get(clips[i],'end_time'))>base.position)throw Error('The draft area now contains a clip. Reload before saving.');return arrangementDraft(track,base.position,base.loopEnd);}var slot=api(Number(base.slotId));if(Number(get(slot,'has_clip')))throw Error('The empty slot changed. Reload this track before saving.');var t=parentTrack(slot);if(String(t.id)!==String(base.trackId))throw Error('The draft track changed.');return draft(slot,t);}
    function materialize(base){if(!base.isDraft)return api(Number(base.clipId));if(base.isArrangement){bound(base);var track=api(Number(base.trackId));track.call('create_midi_clip',base.position,base.loopEnd);var clips=arranged(track);for(var i=0;i<clips.length;i++)if(Math.abs(Number(get(clips[i],'start_time'))-base.position)<1e-7){var v=api('live_set view');if(String(get(v,'selected_track'))===String(base.trackId))selectClip(v,clips[i].id);return clips[i];}throw Error('Live did not confirm the new Arrangement clip.');}var slot=api(Number(base.slotId));if(Number(get(slot,'has_clip')))throw Error('The draft slot is no longer empty.');slot.call('create_clip',base.loopEnd-base.loopStart);var c=api(Number(get(slot,'clip')));if(!c.id)throw Error('Live did not create the clip.');var v=api('live_set view');if(String(get(v,'selected_track'))===String(base.trackId))selectClip(v,c.id);return c;}
    function telemetry(base){var song=api('live_set'),value={clipId:base?String(base.clipId):'',playing:false,playhead:0,tempo:Number(get(song,'tempo')),timeMs:Date.now()};if(base&&!base.isDraft){var c=api(Number(base.clipId));value.playing=!!Number(get(song,'is_playing'))&&!!Number(get(c,'is_playing'));value.playhead=Number(get(c,'playing_position'))-base.loopStart;}return value;}
    function restoreTarget(){if(!route)return;try{var t=api(route.target);if(route.implicitChanged && Number(get(t,'implicit_arm'))===1)t.set('implicit_arm',route.implicit); }catch(e){report(e.message);} }
    function panic(restoreRouting){
        Object.keys(held).forEach(function(p){emitMidi([128,Number(p),0]);});held={};restoreTarget();
        if(route && restoreRouting && route.originalType){try{var h=api(route.helper);if(routing(h,'output_routing_type').identifier===route.assignedType.identifier&&(!route.assignedChannel||routing(h,'output_routing_channel').identifier===route.assignedChannel.identifier)){
            setRouting(h,'output_routing_type',route.originalType);setRouting(h,'output_routing_channel',route.originalChannel);
        }}catch(e){report(e.message);}route=null;}
        else if(restoreRouting)route=null;
        else if(route)route.implicitChanged=false;
    }
    function prepare(base){
        var track=selectedTrack();if(String(track.id)!==String(base.trackId))throw Error('The audition target changed.');
        if(route && route.target!==track.id)panic(true);
        var h=helper();
        if(!route){
            route={target:track.id,helper:h.id,implicit:Number(get(track,'implicit_arm')),implicitChanged:false};
            if(h.id!==track.id){
                if(!Number(get(h,'has_midi_output'))){route=null;throw Error('Place the companion on a MIDI helper track without an instrument.');}
                var types=dict(h.get('available_output_routing_types')).available_output_routing_types;
                var matches=types.filter(function(x){return x.display_name===name(track);});
                if(matches.length!==1){route=null;throw Error('Audition needs a uniquely named target MIDI track.');}
                route.originalType=routing(h,'output_routing_type');route.originalChannel=routing(h,'output_routing_channel');route.assignedType=matches[0];
                setRouting(h,'output_routing_type',route.assignedType);
                if(String(routing(h,'output_routing_type').identifier)!==String(route.assignedType.identifier))throw Error('Live could not route audition to this synth.');
                var channels=dict(h.get('available_output_routing_channels')).available_output_routing_channels;
                for(var i=0;i<channels.length;i++)if(channels[i].display_name==='Track In'){setRouting(h,'output_routing_channel',channels[i]);break;}
                route.assignedChannel=routing(h,'output_routing_channel');
            }
        }
        if(!Number(get(track,'arm'))&&!Number(get(track,'implicit_arm'))){
            var song=api('live_set');if(Number(get(song,'record_mode'))||Number(get(song,'session_record')))throw Error('Arm the synth track yourself before auditioning while Live is recording.');
            route.implicit=0;track.set('implicit_arm',1);route.implicitChanged=true;if(!Number(get(track,'implicit_arm')))throw Error('Live could not enable MIDI audition on this track.');
        }
    }
    function command(event,base){
        if(event.kind==='panic'){panic(true);return;}
        if(event.kind==='keepalive'){lastEditor=Date.now();return;}
        if(event.kind==='transport'){
            var song=api('live_set');
            if(event.action==='stop'){song.call('stop_playing');return;}
            if(!base)throw Error('Select a MIDI track or clip to play.');
            var c=materialize(base);
            if(Number(get(c,'is_session_clip')))c.call('fire');
            else {parentTrack(c).set('back_to_arranger',0);song.set('current_song_time',Number(get(c,'start_time')));song.call('start_playing');}
            return;
        }
        if(event.kind!=='audition'||!base)return;
        var pitch=event.pitch;if(typeof pitch!=='number'||pitch!==Math.floor(pitch)||pitch<0||pitch>127)throw Error('Invalid audition pitch.');
        lastEditor=Date.now();
        if(event.on){prepare(base);if(!held[pitch])emitMidi([144,pitch,Math.max(1,Math.min(127,event.velocity|0))]);held[pitch]=(held[pitch]||0)+1;}
        else if(route && String(route.target)===String(base.trackId) && held[pitch]){if(--held[pitch]===0){delete held[pitch];emitMidi([128,pitch,0]);}if(!Object.keys(held).length)restoreTarget();}
    }
    function tick(){if(route){try{if(selectedTrack().id!==route.target || (Object.keys(held).length && Date.now()-lastEditor>3500))panic(true);}catch(e){panic(true);}}}
    return {target:target,bound:bound,materialize:materialize,captureClip:captureClip,telemetry:telemetry,command:command,tick:tick,cleanup:function(){panic(true);}};
}
var api={create:create};if(typeof module!=='undefined'&&module.exports)module.exports=api;else root.LiveWorkflow=api;
}(this));
