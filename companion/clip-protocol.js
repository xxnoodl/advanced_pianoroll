// Shared, dependency-free clip validation/diff logic. ES5 syntax also runs in Max's js engine.
(function (root) {
    'use strict';
    function fail(message) { throw new Error(message); }
    function finite(n) { return typeof n === 'number' && isFinite(n); }
    function integer(n) { return finite(n) && Math.floor(n) === n; }
    function canonical(value) {
        if (Array.isArray(value)) return '[' + value.map(canonical).join(',') + ']';
        if (value && typeof value === 'object') return '{' + Object.keys(value).sort().map(function (k) { return JSON.stringify(k) + ':' + canonical(value[k]); }).join(',') + '}';
        return JSON.stringify(value);
    }
    function fingerprint(clip) {
        return canonical({clipId: String(clip.clipId), loopStart:clip.loopStart, loopEnd:clip.loopEnd,
            notes:clip.notes.slice().sort(function(a,b) { return a.note_id-b.note_id; })});
    }
    function editable(clip) {
        return clip.notes.filter(function(n) { return n.start_time >= clip.loopStart && n.start_time + n.duration <= clip.loopEnd + 1e-8; });
    }
    function validateClip(clip) {
        if (!clip || !/^(?:[1-9][0-9]*|empty:[1-9][0-9]*|arrangement:[1-9][0-9]*:[0-9]+)$/.test(String(clip.clipId))) fail('Select a MIDI clip in Live.');
        if (!finite(clip.loopStart) || !finite(clip.loopEnd) || clip.loopStart < 0 || clip.loopEnd <= clip.loopStart || clip.loopEnd-clip.loopStart > 128) fail('This bridge supports clip loops up to 128 beats.');
        if (!Array.isArray(clip.notes)) fail('Live did not return a notes list.');
        var seen = {};
        clip.notes.forEach(function(n) {
            if (!integer(n.note_id) || n.note_id <= 0 || n.note_id > 9007199254740991 || seen[n.note_id]) fail('Invalid or repeated Live note ID.');
            seen[n.note_id] = true;
            if (!integer(n.pitch) || n.pitch < 0 || n.pitch > 127 || !finite(n.start_time) || !finite(n.duration) || n.duration <= 0
                || !finite(n.velocity) || n.velocity < 0 || n.velocity > 127) fail('Live returned an invalid note.');
        });
        if (editable(clip).length > 2048) fail('The active loop exceeds the editor capacity of 2048 notes.');
        return clip;
    }
    function plan(base, current, request) {
        validateClip(base); validateClip(current);
        if (!request || request.protocol !== 1 || request.session !== base.session || String(request.clipId) !== String(base.clipId)
            || request.baseRevision !== base.revision || request.baseFingerprint !== fingerprint(base)) fail('This edit belongs to a different clip snapshot. Reload the selected clip.');
        if (fingerprint(current) !== fingerprint(base)) fail('The Live clip changed. Reload it before applying edits.');
        if (!Array.isArray(request.notes) || request.notes.length > 2048) fail('Invalid edited note list.');
        var existing = {}, used = {}, locals = {}, modifications = [], additions = [], removals = [];
        editable(base).forEach(function(n) { existing[n.note_id] = n; });
        request.notes.forEach(function(n) {
            if (!integer(n.localId) || n.localId <= 0 || n.localId > 4294967295 || locals[n.localId]) fail('Repeated editor note ID.');
            locals[n.localId] = true;
            if (!integer(n.pitch) || n.pitch < 0 || n.pitch > 127 || !finite(n.start_time) || !finite(n.duration) || n.duration <= 0
                || n.start_time < base.loopStart-1e-8 || n.start_time+n.duration > base.loopEnd+1e-8
                || !integer(n.velocity) || n.velocity < 0 || n.velocity > 127 || typeof n.mute !== 'boolean') fail('An edited note is outside the clip bounds or supported ranges.');
            var item = {pitch:n.pitch,start_time:n.start_time,duration:n.duration,velocity:n.velocity,mute:n.mute};
            if (!integer(n.sourceId) || n.sourceId < 0 || n.sourceId > 9007199254740991) fail('Invalid source note ID.');
            if (!n.sourceId) { additions.push(item); return; }
            if (!existing[n.sourceId] || used[n.sourceId]) fail('Unknown or repeated source note ID.');
            used[n.sourceId] = true;
            var original = existing[n.sourceId], changed = false, update = JSON.parse(JSON.stringify(original));
            ['pitch','start_time','duration','velocity','mute'].forEach(function(key) {
                var previous = key === 'velocity' ? Math.round(original[key]) : key === 'mute' ? !!original[key] : original[key];
                var same = typeof previous === 'number' ? Math.abs(previous-item[key]) < 1e-8 : previous === item[key];
                if (!same) { changed = true; update[key] = item[key]; }
            });
            // Existing IDs and all fields that the editor doesn't expose are retained.
            if (changed) modifications.push(update);
        });
        Object.keys(existing).forEach(function(id) { if (!used[id]) removals.push(Number(id)); });
        return {modifications:modifications,additions:additions,removals:removals};
    }
    var api = {canonical:canonical,fingerprint:fingerprint,editable:editable,validateClip:validateClip,plan:plan};
    if (typeof module !== 'undefined' && module.exports) module.exports = api; else root.ClipProtocol = api;
}(this));
