#include <set>
#include "LiveClipBridge.h"
namespace {
bool number(const juce::var& n) { return (n.isInt() || n.isInt64() || n.isDouble()) && std::isfinite(static_cast<double>(n)); }
bool integral(const juce::var& n) { return number(n) && std::floor(static_cast<double>(n)) == static_cast<double>(n); }
juce::Result error(const juce::String& s) { return juce::Result::fail(s); }
}
juce::var LiveClipBridge::read(const char* name) const {
    const auto file = folder.getChildFile(name);
    if (!file.existsAsFile() || file.getSize() > 8 * 1024 * 1024) return {};
    return juce::JSON::parse(file.loadFileAsString());
}
juce::Result LiveClipBridge::connect(const juce::File& directory) {
    if (!directory.isDirectory()) return error("The Live companion folder is missing.");
    folder=directory; session.clear(); clipId.clear(); baseline.reset(); sourceIds.clear(); pending.clear(); revision=0;
    const auto heartbeat=read("heartbeat.json");
    const double age=juce::Time::currentTimeMillis()-static_cast<double>(heartbeat["timeMs"]);
    if(heartbeat["protocol"]!=juce::var(1) || heartbeat["session"].toString().isEmpty() || age < -1000 || age >= 5000) {
        folder={}; message="The Live companion is not connected."; return error(message);
    }
    session=heartbeat["session"].toString();
    if(static_cast<bool>(heartbeat["available"])) {
        const auto snapshot=read("snapshot.json");
        if(snapshot["session"].toString()!=session) {folder={};return error("The Live clip snapshot belongs to another session.");}
        auto result=load(snapshot);
        if(result.failed()) {folder={};message=result.getErrorMessage();return result;}
    } else {
        auto empty=std::make_unique<pr::Pattern>(); processor.writePattern(*empty);
        message="Select a MIDI clip in Live. The editor follows while there are no unapplied edits.";
    }
    processor.externalClipMode.store(true); processor.playing.store(false); processor.hostSync.store(false); processor.monitor.store(false);
    return juce::Result::ok();
}
bool LiveClipBridge::online() const {
    const auto beat = read("heartbeat.json");
    const double age = juce::Time::currentTimeMillis() - static_cast<double>(beat["timeMs"]);
    return beat["protocol"] == juce::var(1) && beat["session"].toString() == session && static_cast<bool>(beat["available"]) && age >= -1000 && age < 5000;
}
bool LiveClipBridge::dirty() const {
    if (!baseline) return false;
    const auto current = processor.readPattern();
    if (current.count != baseline->count || current.loop != baseline->loop) return true;
    for (int i = 0; i < current.count; ++i) {
        const auto& a = current.notes[static_cast<size_t>(i)]; const auto& b = baseline->notes[static_cast<size_t>(i)];
        if (a.id != b.id || a.start != b.start || a.length != b.length || a.pitch != b.pitch || a.velocity != b.velocity || a.muted != b.muted) return true;
    }
    return false;
}
juce::Result LiveClipBridge::load(const juce::var& snapshot) {
    if (!snapshot.isObject() || snapshot["protocol"] != juce::var(1) || snapshot["session"].toString().isEmpty()
        || snapshot["clipId"].toString().containsNonWhitespaceChars() == false || !integral(snapshot["revision"]) || static_cast<int>(snapshot["revision"]) <= 0
        || !snapshot["notes"].isArray() || !number(snapshot["loopStart"]) || !number(snapshot["loopEnd"])) return error("Invalid Live clip snapshot.");
    const double start = snapshot["loopStart"], finish = snapshot["loopEnd"], duration = finish-start;
    if (start < 0 || duration <= 0 || duration > 128 || snapshot["fingerprint"].toString().isEmpty()) return error("Unsupported Live clip loop.");
    auto next = std::make_unique<pr::Pattern>(); next->loop = std::max(4.0,duration);
    std::map<uint32_t,juce::int64> ids; std::set<juce::int64> unique;
    int skipped = 0;
    for (const auto& n : *snapshot["notes"].getArray()) {
        if (!n.isObject() || !integral(n["note_id"]) || static_cast<juce::int64>(n["note_id"]) <= 0 || static_cast<double>(n["note_id"]) > 9007199254740991.0
            || !integral(n["pitch"]) || static_cast<int>(n["pitch"]) < 0 || static_cast<int>(n["pitch"]) > 127
            || !number(n["start_time"]) || !number(n["duration"]) || static_cast<double>(n["duration"]) <= 0
            || !number(n["velocity"]) || static_cast<double>(n["velocity"]) < 0 || static_cast<double>(n["velocity"]) > 127) return error("Invalid note in Live clip.");
        const auto id = static_cast<juce::int64>(n["note_id"]);
        if (!unique.insert(id).second) return error("Repeated Live note ID.");
        const double noteStart = n["start_time"], length = n["duration"];
        if (noteStart < start || noteStart+length > finish+1e-8) { ++skipped; continue; }
        if (next->count >= pr::capacity) return error("The clip loop exceeds 2048 editable notes.");
        const auto localId = next->nextId++;
        next->notes[static_cast<size_t>(next->count++)] = {localId,noteStart-start,length,static_cast<int>(n["pitch"]),static_cast<int>(std::lround(static_cast<double>(n["velocity"]))),1,static_cast<bool>(n["mute"])};
        ids[localId] = id;
    }
    session = snapshot["session"].toString(); clipId = snapshot["clipId"].toString(); revision = snapshot["revision"];
    fingerprint = snapshot["fingerprint"].toString(); origin=start; end=finish; sourceIds=std::move(ids);
    clipName=snapshot["name"].toString().substring(0,256); baseline=std::move(next); processor.writePattern(*baseline);
    message = "Clip loaded. Apply edits to update Live.";
    if (skipped) message += " / " + juce::String(skipped) + " boundary/outside-loop notes preserved";
    return juce::Result::ok();
}
bool LiveClipBridge::poll(bool force) {
    if (!active()) return false;
    if (!online()) { message = "Live companion unavailable, or no editable MIDI clip selected."; return false; }
    if (pending.isNotEmpty()) {
        const auto ack = read("ack.json");
        if (ack["session"].toString() == session && ack["requestId"].toString() == pending) {
            if (!static_cast<bool>(ack["ok"])) { message=ack["message"].toString(); pending.clear(); return false; }
            const auto snapshot = read("snapshot.json");
            if (static_cast<int>(snapshot["revision"]) < static_cast<int>(ack["revision"])) return false;
            // Local edits are disabled while awaiting the acknowledgement.
            auto result = load(snapshot); message=result.wasOk() ? ack["message"].toString() : result.getErrorMessage(); pending.clear(); return result.wasOk();
        }
        if (juce::Time::getMillisecondCounterHiRes()-sentAt > 10000) { pending.clear(); message="Live did not acknowledge Apply. Reload and inspect the clip before retrying."; }
        return false;
    }
    const auto snapshot=read("snapshot.json");
    if (snapshot["session"].toString()!=session) { message="The Live session changed. Reopen the editor from its companion."; return false; }
    if (!force && static_cast<int>(snapshot["revision"])==revision) return false;
    if (!force && dirty()) { message="Unapplied edits retained. Reload selected clip to discard them and follow Live."; return false; }
    auto result=load(snapshot); if (result.failed()) message=result.getErrorMessage(); return result.wasOk();
}
juce::Result LiveClipBridge::apply() {
    if (!active() || !baseline || !online()) return error("Select a MIDI clip with the Live companion connected.");
    if (waiting()) return error("Waiting for Live to acknowledge the previous edit.");
    if (!dirty()) return error("No clip edits to apply.");
    const auto current=processor.readPattern();
    if (current.loop!=baseline->loop) return error("Clip loop resizing is not supported by this bridge yet.");
    juce::Array<juce::var> notes;
    for (int i=0;i<current.count;++i) {
        const auto& n=current.notes[static_cast<size_t>(i)];
        if (!std::isfinite(n.start) || !std::isfinite(n.length) || n.start<0 || n.length<=0 || origin+n.start+n.length>end+1e-8) return error("Keep edited notes inside the selected Live loop.");
        auto item=std::make_unique<juce::DynamicObject>();
        item->setProperty("localId",static_cast<juce::int64>(n.id)); auto source=sourceIds.find(n.id);
        item->setProperty("sourceId",source==sourceIds.end()?juce::int64(0):source->second);
        item->setProperty("pitch",n.pitch); item->setProperty("start_time",origin+n.start); item->setProperty("duration",n.length);
        item->setProperty("velocity",n.velocity); item->setProperty("mute",n.muted); notes.add(juce::var(item.release()));
    }
    const auto id=juce::Uuid().toString();
    auto request=std::make_unique<juce::DynamicObject>(); request->setProperty("protocol",1); request->setProperty("session",session); request->setProperty("requestId",id);
    request->setProperty("clipId",clipId); request->setProperty("baseRevision",revision); request->setProperty("baseFingerprint",fingerprint); request->setProperty("notes",notes);
    if (!folder.getChildFile("request.json").replaceWithText(juce::JSON::toString(juce::var(request.release())))) return error("Could not send the edit to Live.");
    pending=id; sentAt=juce::Time::getMillisecondCounterHiRes(); message="Applying clip edits in Live..."; return juce::Result::ok();
}
