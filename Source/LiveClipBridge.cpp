#include <set>
#include <vector>
#include "LiveClipBridge.h"
namespace {
bool number(const juce::var& n) { return (n.isInt() || n.isInt64() || n.isDouble()) && std::isfinite(static_cast<double>(n)); }
bool integral(const juce::var& n) { return number(n) && std::floor(static_cast<double>(n)) == static_cast<double>(n); }
bool same(const pr::Pattern& a,const pr::Pattern& b) {
    if(a.count!=b.count||a.loop!=b.loop)return false;
    std::vector<const pr::Note*> right;right.reserve(static_cast<size_t>(b.count));
    for(int i=0;i<b.count;++i)right.push_back(&b.notes[static_cast<size_t>(i)]);
    std::sort(right.begin(),right.end(),[](auto* x,auto* y){return x->id<y->id;});
    for(int i=0;i<a.count;++i){const auto& x=a.notes[static_cast<size_t>(i)];auto it=std::lower_bound(right.begin(),right.end(),x.id,[](auto* n,uint32_t id){return n->id<id;});if(it==right.end()||(*it)->id!=x.id)return false;const auto& y=**it;if(x.start!=y.start||x.length!=y.length||x.pitch!=y.pitch||x.velocity!=y.velocity||x.muted!=y.muted)return false;}
    return true;
}
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
bool LiveClipBridge::connected() const {
    if(!active())return false;
    const auto beat = read("heartbeat.json");
    const double age = juce::Time::currentTimeMillis() - static_cast<double>(beat["timeMs"]);
    return beat["protocol"] == juce::var(1) && beat["session"].toString() == session && age >= -1000 && age < 5000;
}
bool LiveClipBridge::online() const { return connected() && static_cast<bool>(read("heartbeat.json")["available"]); }
bool LiveClipBridge::dirty() const { return baseline && !same(processor.readPattern(),*baseline); }
juce::Result LiveClipBridge::load(const juce::var& snapshot,const std::map<juce::int64,uint32_t>* preserve) {
    if (!snapshot.isObject() || snapshot["protocol"] != juce::var(1) || snapshot["session"].toString().isEmpty()
        || snapshot["clipId"].toString().containsNonWhitespaceChars() == false || !integral(snapshot["revision"]) || static_cast<int>(snapshot["revision"]) <= 0
        || !snapshot["notes"].isArray() || !number(snapshot["loopStart"]) || !number(snapshot["loopEnd"])) return error("Invalid Live clip snapshot.");
    const double start = snapshot["loopStart"], finish = snapshot["loopEnd"], duration = finish-start;
    if (start < 0 || duration <= 0 || duration > 128 || snapshot["fingerprint"].toString().isEmpty()) return error("Unsupported Live clip loop.");
    auto next = std::make_unique<pr::Pattern>(); next->loop = std::max(4.0,duration);
    std::map<uint32_t,juce::int64> ids; std::set<juce::int64> unique;
    if(preserve) next->nextId=processor.readPattern().nextId;
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
        uint32_t localId=0;
        if(preserve){auto it=preserve->find(id);if(it!=preserve->end())localId=it->second;}
        if(!localId)localId=next->nextId++;
        next->nextId=std::max(next->nextId,localId+1);
        next->notes[static_cast<size_t>(next->count++)] = {localId,noteStart-start,length,static_cast<int>(n["pitch"]),static_cast<int>(std::lround(static_cast<double>(n["velocity"]))),1,static_cast<bool>(n["mute"])};
        ids[localId] = id;
    }
    if(clipId.isNotEmpty() && clipId!=snapshot["clipId"].toString())panic();
    session = snapshot["session"].toString(); clipId = snapshot["clipId"].toString(); revision = snapshot["revision"];
    fingerprint = snapshot["fingerprint"].toString(); origin=start; end=finish; sourceIds=std::move(ids);
    clipName=snapshot["name"].toString().substring(0,256); baseline=std::move(next); processor.writePattern(*baseline);
    message = "Edits sync automatically to Live."; paused=false;
    if (skipped) message += " / " + juce::String(skipped) + " boundary/outside-loop notes preserved";
    return juce::Result::ok();
}
bool LiveClipBridge::poll(bool force) {
    acceptedAcknowledgement=false;
    if(!active())return false;
    telemetry();
    const auto now=juce::Time::getMillisecondCounterHiRes();
    if(!held.empty()&&now-lastKeepAlive>800){auto* e=new juce::DynamicObject();e->setProperty("kind","keepalive");sendEvent(e);lastKeepAlive=now;}
    if(pending.isNotEmpty()) {
        const auto ack=read("ack.json");
        if(ack["session"].toString()==session&&ack["requestId"].toString()==pending) {
            if(!static_cast<bool>(ack["ok"])) {message=ack["message"].toString();pending.clear();paused=true;queuedTransport=false;return false;}
            const auto snapshot=ack["snapshot"].isObject()?ack["snapshot"]:read("snapshot.json");
            if(snapshot["session"].toString()!=session){pending.clear();paused=true;message="Live returned a snapshot from another session.";return false;}
            if(static_cast<int>(snapshot["revision"])<static_cast<int>(ack["revision"]))return false;
            const auto current=std::make_unique<pr::Pattern>(processor.readPattern());
            const bool more=sentPattern&&!same(*current,*sentPattern);
            std::map<juce::int64,uint32_t> mapping;
            if(ack["sourceIds"].isArray()) {
                for(const auto& item:*ack["sourceIds"].getArray())if(integral(item["sourceId"])&&integral(item["localId"]))mapping[static_cast<juce::int64>(item["sourceId"])]=static_cast<uint32_t>(static_cast<juce::int64>(item["localId"]));
            } else {for(const auto& pair:sourceIds)mapping[pair.second]=pair.first;}
            const auto result=load(snapshot,&mapping);pending.clear();sentPattern.reset();
            if(result.failed()){message=result.getErrorMessage();paused=true;return false;}
            if(more)processor.writePattern(*current);
            acceptedAcknowledgement=true;
            if(queuedTransport){queuedTransport=false;auto* event=new juce::DynamicObject();event->setProperty("kind","transport");event->setProperty("action","play");sendEvent(event);}
            return true;
        }
        if(now-sentAt>10000){pending.clear();paused=true;queuedTransport=false;message="Live did not confirm the save. Reload or retry after inspecting the clip.";}
        return false;
    }
    if(!online()){message="Select a MIDI instrument track in Live.";return false;}
    const auto snapshot=read("snapshot.json");
    if(snapshot["session"].toString()!=session){message="The Live session changed. Reopen the editor from its companion.";return false;}
    if(!force&&static_cast<int>(snapshot["revision"])==revision)return false;
    if(!force&&dirty())return false;
    auto result=load(snapshot);if(result.failed())message=result.getErrorMessage();return result.wasOk();
}
juce::Result LiveClipBridge::apply() {
    auto failure=[this](const juce::String& text){paused=true;message=text;return error(text);};
    if (!active() || !baseline || !connected()) return failure("Select a MIDI clip with the Live companion connected.");
    if (waiting()) return failure("Waiting for Live to acknowledge the previous edit.");
    if (!dirty()) return failure("No clip edits to apply.");
    const auto current=processor.readPattern();
    if (current.loop!=baseline->loop) return failure("Clip loop resizing is not supported by this bridge yet.");
    juce::Array<juce::var> notes;
    for (int i=0;i<current.count;++i) {
        const auto& n=current.notes[static_cast<size_t>(i)];
        if (!std::isfinite(n.start) || !std::isfinite(n.length) || n.start<0 || n.length<=0 || origin+n.start+n.length>end+1e-8) return failure("Keep edited notes inside the selected Live loop.");
        auto item=std::make_unique<juce::DynamicObject>();
        item->setProperty("localId",static_cast<juce::int64>(n.id)); auto source=sourceIds.find(n.id);
        item->setProperty("sourceId",source==sourceIds.end()?juce::int64(0):source->second);
        item->setProperty("pitch",n.pitch); item->setProperty("start_time",origin+n.start); item->setProperty("duration",n.length);
        item->setProperty("velocity",n.velocity); item->setProperty("mute",n.muted); notes.add(juce::var(item.release()));
    }
    const auto id=juce::Uuid().toString();
    auto request=std::make_unique<juce::DynamicObject>(); request->setProperty("protocol",1); request->setProperty("session",session); request->setProperty("requestId",id);
    request->setProperty("clipId",clipId); request->setProperty("baseRevision",revision); request->setProperty("baseFingerprint",fingerprint); request->setProperty("notes",notes);
    if (!folder.getChildFile("request.json").replaceWithText(juce::JSON::toString(juce::var(request.release())))) return failure("Could not send the edit to Live.");
    sentPattern=std::make_unique<pr::Pattern>(current); paused=false;
    pending=id; sentAt=juce::Time::getMillisecondCounterHiRes(); message="Syncing notes to Live..."; return juce::Result::ok();
}

juce::Result LiveClipBridge::sendEvent(juce::DynamicObject* raw) {
    std::unique_ptr<juce::DynamicObject> value(raw);
    if(!active())return error("The Live companion is not connected.");
    value->setProperty("protocol",1);value->setProperty("session",session);value->setProperty("targetId",clipId);value->setProperty("sequence",++eventSequence);
    juce::FileOutputStream output(folder.getChildFile("events.jsonl"));
    if(!output.openedOk())return error("Could not send the command to Live.");
    const auto line=juce::JSON::toString(juce::var(value.release()),true)+"\n";
    if(!output.writeText(line,false,false,nullptr))return error("Could not send the command to Live.");output.flush();return juce::Result::ok();
}
juce::Result LiveClipBridge::toggleTransport() {
    if(!connected())return error("The Live companion is not responding.");
    if(!hasClip())return error("Select a MIDI track in Live to play.");
    if(!processor.transportRunning.load()&&(dirty()||waiting())) {
        if(queuedTransport){queuedTransport=false;return juce::Result::ok();}
        queuedTransport=true;if(!waiting()){auto result=apply();if(result.failed()){queuedTransport=false;return result;}}return juce::Result::ok();
    }
    if(processor.transportRunning.load())queuedTransport=false;
    auto* event=new juce::DynamicObject();event->setProperty("kind","transport");event->setProperty("action",processor.transportRunning.load()?"stop":"play");return sendEvent(event);
}
void LiveClipBridge::preview(int pitch,int velocity,bool on,int) {
    if(on&&(!hasClip()||!auditionEnabled||!connected()))return;
    if(on)++held[pitch];else {auto it=held.find(pitch);if(it!=held.end()&&--it->second<=0)held.erase(it);}
    processor.keyActivity[static_cast<size_t>(pitch)].store(held.count(pitch)>0);
    auto* event=new juce::DynamicObject();event->setProperty("kind","audition");event->setProperty("pitch",pitch);event->setProperty("velocity",velocity);event->setProperty("on",on);sendEvent(event);
}
void LiveClipBridge::panic() {
    for(const auto& item:held)processor.keyActivity[static_cast<size_t>(item.first)].store(false);held.clear();
    if(active()){auto* event=new juce::DynamicObject();event->setProperty("kind","panic");sendEvent(event);}
}
void LiveClipBridge::telemetry() {
    const auto state=read("telemetry.json");
    if(state["clipId"].toString()!=clipId)return;
    const auto age=juce::Time::currentTimeMillis()-static_cast<double>(state["timeMs"]);
    if(age<0||age>2000)return;
    const bool running=static_cast<bool>(state["playing"]);const double tempo=state["tempo"];
    if(!std::isfinite(tempo)||tempo<=0)return;
    processor.reportedHostTempo.store(tempo);processor.transportRunning.store(running);processor.playing.store(running);
    const double beat=static_cast<double>(state["playhead"])+(running?age/1000.0*tempo/60.0:0);
    processor.playhead.store(pr::wrap(beat,std::max(1.0/960,end-origin)));
    for(auto& key:processor.keyActivity)key.store(false);
    for(const auto& key:held)processor.keyActivity[static_cast<size_t>(key.first)].store(true);
    if(running){const auto score=processor.readPattern();const auto position=processor.playhead.load();for(int i=0;i<score.count;++i){const auto& n=score.notes[static_cast<size_t>(i)];if(!n.muted&&position>=n.start&&position<n.start+n.length)processor.keyActivity[static_cast<size_t>(n.pitch)].store(true);}}
    const auto event=read("event-status.json");if(juce::Time::currentTimeMillis()-static_cast<double>(event["timeMs"])<2000)message=event["message"].toString();
}
