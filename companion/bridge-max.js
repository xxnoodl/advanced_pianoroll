/* Max's low-priority Live API adapter. */
autowatch = 1;
inlets = 1;
outlets = 2;
include(typeof jsarguments !== 'undefined' && jsarguments.length > 1 ? String(jsarguments[1]) : 'clip-protocol.js');
var bridgeReady = false, bridgeSession = '', baseline = null, serial = 0, lastFingerprint = '', replies = {};
function status(text) { outlet(1, 'set', String(text)); }
function init() { bridgeReady = true; outlet(0, 'boot'); status('Select a MIDI clip, then Open editor.'); }
function session(id) { bridgeSession = String(id); if (bridgeReady) loadclip(); }
function scalar(value) { return Array.isArray(value) ? value[value.length-1] : value; }
function dictionary(value) {
    if (value && value.notes) return value;
    if (value && typeof value.stringify === 'function') return JSON.parse(value.stringify());
    if (Array.isArray(value) && value[0] === 'dictionary') return JSON.parse(new Dict(value[1]).stringify());
    if (Array.isArray(value) && value.length === 1) value = value[0];
    if (typeof value === 'string') return JSON.parse(value);
    throw new Error('Live returned an unsupported note dictionary.');
}
function selected() {
    var view = new LiveAPI(null, 'live_set view');
    var id = Number(scalar(view.get('detail_clip')));
    if (!id) throw new Error('Select a MIDI clip in Live first.');
    var clip = new LiveAPI(null, 'id ' + id);
    if (!Number(scalar(clip.get('is_midi_clip')))) throw new Error('The selected clip is not MIDI.');
    if (Number(scalar(clip.get('is_recording')))) throw new Error('Finish recording the clip before editing it.');
    return clip;
}
function capture() {
    var api = selected(), notes = dictionary(api.call('get_all_notes_extended'));
    return ClipProtocol.validateClip({protocol:1,session:bridgeSession,clipId:String(api.id),name:String(api.getstring('name')),
        loopStart:Number(scalar(api.get('loop_start'))),loopEnd:Number(scalar(api.get('loop_end'))),notes:notes.notes});
}
function publish(force) {
    if (!bridgeReady || !bridgeSession) return;
    var clip = capture(), fp = ClipProtocol.fingerprint(clip);
    if (!force && fp === lastFingerprint) return;
    clip.revision = ++serial; clip.fingerprint = fp;
    baseline = clip; lastFingerprint = fp;
    outlet(0, 'snapshot', JSON.stringify(clip));
    status('Selected: ' + clip.name + ' / ' + ClipProtocol.editable(clip).length + ' editable notes');
}
function loadclip() { try { publish(true); } catch (e) { status(e.message); outlet(0,'unavailable',e.message); } }
function poll() { try { publish(false); } catch (e) { lastFingerprint=''; status(e.message); outlet(0,'unavailable',e.message); } }
function openeditor() { poll(); outlet(0,'openeditor'); }
function result(value) { outlet(0,'result',JSON.stringify(value)); }
function apply(text) {
    var request, reply;
    try {
        request = JSON.parse(String(text));
        if (typeof request.requestId !== 'string' || !/^[a-zA-Z0-9-]{8,80}$/.test(request.requestId)) throw new Error('Invalid edit request.');
        if (replies[request.requestId]) { result(replies[request.requestId]); return; }
        if (!baseline) throw new Error('Reload the selected MIDI clip first.');
        var current = capture();
        if (String(current.clipId) !== String(request.clipId)) throw new Error('A different Live clip is selected. Reload it before applying.');
        var diff = ClipProtocol.plan(baseline,current,request), api = selected();
        // Keep existing note IDs. Each operation is checked against a complete preflight diff.
        // Live provides undo, but this API does not expose a cross-call atomic transaction.
        if (diff.modifications.length) api.call('apply_note_modifications',JSON.stringify({notes:diff.modifications}));
        if (diff.additions.length) api.call('add_new_notes',JSON.stringify({notes:diff.additions}));
        if (diff.removals.length) api.call.apply(api,['remove_notes_by_id'].concat(diff.removals));
        var after = capture();
        var expected = request.notes.map(function(n){return [n.pitch,n.start_time,n.duration,n.velocity,n.mute];});
        var actual = ClipProtocol.editable(after).map(function(n){return [n.pitch,n.start_time,n.duration,Math.round(n.velocity),!!n.mute];});
        function sortNotes(a) { return a.sort(function(x,y) { for(var i=0;i<5;i++) if(x[i]!==y[i]) return Number(x[i])-Number(y[i]); return 0; }); }
        sortNotes(expected); sortNotes(actual);
        var confirmed=expected.length===actual.length;
        for(var i=0;confirmed && i<expected.length;i++) for(var j=0;j<5;j++) {
            if(j===1 || j===2) confirmed=confirmed && Math.abs(expected[i][j]-actual[i][j])<1e-7;
            else confirmed=confirmed && expected[i][j]===actual[i][j];
        }
        if (!confirmed) throw new Error('Live did not confirm all edits. Inspect the clip and use Live Undo if needed, then reload.');
        publish(true);
        reply = {protocol:1,session:bridgeSession,requestId:request.requestId,ok:true,revision:baseline.revision,message:'Applied to '+baseline.name};
    } catch(e) {
        reply = {protocol:1,session:bridgeSession,requestId:request ? request.requestId : '',ok:false,message:e.message};
        status(e.message);
    }
    if (reply.requestId) replies[reply.requestId] = reply;
    result(reply);
}
function nodeerror(text) { status(text); }
