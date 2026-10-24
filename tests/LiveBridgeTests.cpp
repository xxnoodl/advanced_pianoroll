#include "LiveClipBridge.h"
#include "Editor.h"
#include <iostream>
#include <stdexcept>
static void require(bool value,const char* message) {if(!value)throw std::runtime_error(message);}
static juce::var snapshot(int revision=1,const char* clip="42") {
    auto value=juce::JSON::parse(R"({"protocol":1,"session":"test-session","clipId":"42","revision":1,"name":"Bass","loopStart":4,"loopEnd":8,"fingerprint":"test-fingerprint","notes":[{"note_id":101,"pitch":60,"start_time":4.25,"duration":0.5,"velocity":80.5,"mute":false,"probability":0.4},{"note_id":102,"pitch":64,"start_time":6,"duration":1,"velocity":100,"mute":false},{"note_id":103,"pitch":48,"start_time":0,"duration":1,"velocity":64,"mute":false},{"note_id":104,"pitch":67,"start_time":7.5,"duration":2,"velocity":64,"mute":false}]})");
    value.getDynamicObject()->setProperty("revision",revision);value.getDynamicObject()->setProperty("clipId",clip);return value;
}
static void write(const juce::File& folder,const char* name,const juce::var& value) {require(folder.getChildFile(name).replaceWithText(juce::JSON::toString(value)),"write fixture");}
static void heartbeat(const juce::File& folder,bool available=true) {
    auto beat=juce::JSON::parse(R"({"protocol":1,"session":"test-session","timeMs":0,"available":true})");
    beat.getDynamicObject()->setProperty("timeMs",juce::Time::currentTimeMillis());beat.getDynamicObject()->setProperty("available",available);write(folder,"heartbeat.json",beat);
}
int main() {
    juce::ScopedJuceInitialiser_GUI initialise;
    const auto folder=juce::File::getSpecialLocation(juce::File::tempDirectory).getNonexistentChildFile("pianoroll-bridge-test","",false);folder.createDirectory();
    try {
        auto processor=std::make_unique<PianoRollProcessor>();
        LiveClipBridge bridge(*processor);write(folder,"snapshot.json",snapshot());heartbeat(folder);
        require(bridge.connect(folder).wasOk(),"connect fresh session");require(processor->externalClipMode.load()&&!processor->monitor.load(),"clip editor disables own sound and sequencer");
        auto pattern=processor->readPattern();require(pattern.count==2&&pattern.notes[0].start==.25&&pattern.notes[0].velocity==81,"load loop-relative score without outside/boundary notes");
        require(!bridge.dirty(),"fresh snapshot clean");
        pattern.notes[0].start=.5;processor->writePattern(pattern);require(bridge.dirty(),"local edits detected");
        write(folder,"snapshot.json",snapshot(2,"99"));require(!bridge.poll()&&processor->readPattern().notes[0].start==.5,"clip follow retains unapplied edits");
        write(folder,"snapshot.json",snapshot());require(bridge.apply().wasOk()&&bridge.waiting(),"send edit request");
        auto request=juce::JSON::parse(folder.getChildFile("request.json").loadFileAsString());
        require(request["notes"].getArray()->getReference(0)["sourceId"]==juce::var(101)&&static_cast<double>(request["notes"].getArray()->getReference(0)["start_time"])==4.5,"request retains source IDs and restores loop offset");
        require(bridge.apply().failed(),"only one outstanding request");
        auto ack=juce::JSON::parse(R"({"protocol":1,"session":"test-session","requestId":"","ok":false,"message":"Clip changed"})");
        ack.getDynamicObject()->setProperty("requestId",request["requestId"]);write(folder,"ack.json",ack);
        require(!bridge.poll()&&!bridge.waiting()&&bridge.dirty(),"rejected edit remains available locally");
        require(bridge.apply().wasOk(),"retry request allowed");request=juce::JSON::parse(folder.getChildFile("request.json").loadFileAsString());
        auto updated=snapshot(3);updated["notes"].getArray()->getReference(0).getDynamicObject()->setProperty("start_time",4.5);write(folder,"snapshot.json",updated);
        ack.getDynamicObject()->setProperty("requestId",request["requestId"]);ack.getDynamicObject()->setProperty("ok",true);ack.getDynamicObject()->setProperty("revision",3);ack.getDynamicObject()->setProperty("message","Applied");write(folder,"ack.json",ack);
        require(bridge.poll()&&!bridge.dirty()&&!bridge.waiting(),"confirmed apply establishes new clean snapshot");
        heartbeat(folder,false);require(bridge.apply().failed(),"unavailable Live companion rejects sends");
        auto waitingProcessor=std::make_unique<PianoRollProcessor>(); LiveClipBridge waitingBridge(*waitingProcessor);
        require(waitingBridge.connect(folder).wasOk()&&!waitingBridge.hasClip()&&waitingProcessor->readPattern().count==0,"editor can open before a MIDI clip is selected");
        auto stale=juce::JSON::parse(R"({"protocol":1,"session":"test-session","timeMs":0,"available":false})");write(folder,"heartbeat.json",stale);
        auto other=std::make_unique<PianoRollProcessor>();const int oldCount=other->readPattern().count;LiveClipBridge offline(*other);
        require(offline.connect(folder).failed()&&other->readPattern().count==oldCount,"offline connect preserves existing score");
        heartbeat(folder);auto bad=snapshot(4);bad["notes"].getArray()->getReference(0).getDynamicObject()->setProperty("pitch",200);write(folder,"snapshot.json",bad);
        require(!bridge.poll(true)&&processor->readPattern().notes[0].start==.5,"invalid snapshot never replaces valid score");
        write(folder,"snapshot.json",snapshot(5,"99"));require(bridge.poll()&&processor->readPattern().notes[0].start==.25,"clean editor follows selected clip");
        {
            auto editor=std::make_unique<PianoRollEditor>(*processor);
            require(editor->connectLiveBridge(folder).wasOk(),"editor connects to Live snapshot"); editor->setVisible(true); editor->refreshControls();
            auto* apply=dynamic_cast<juce::TextButton*>(editor->findChildWithID("live-apply"));
            auto* target=dynamic_cast<juce::Label*>(editor->findChildWithID("live-target"));
            require(apply && target && target->getText().contains("Bass") && !apply->isEnabled(),"clip target displayed and clean Apply disabled");
            juce::Image image(juce::Image::ARGB,editor->getWidth(),editor->getHeight(),true,juce::SoftwareImageType());
            {juce::Graphics g(image);editor->paintEntireComponent(g,true);}
            juce::FileOutputStream stream(juce::File::getCurrentWorkingDirectory().getChildFile("pianoroll-live-bridge-preview.png"));stream.setPosition(0);stream.truncate();juce::PNGImageFormat png;
            require(png.writeImageToStream(image,stream),"clip editor preview renders");
            auto edited=processor->readPattern();edited.notes[0].pitch=61;processor->writePattern(edited);editor->refreshControls();require(!apply->isVisible(),"normal editing has no Apply step");
            juce::Thread::sleep(220);editor->refreshControls();
            auto automatic=juce::JSON::parse(folder.getChildFile("request.json").loadFileAsString());
            require(automatic["notes"].getArray()->getReference(0)["pitch"]==juce::var(61),"editing sends notes automatically after debounce");
            auto* undo=dynamic_cast<juce::TextButton*>(editor->findChildWithID("action-undo"));
            require(undo && undo->isEnabled() && !apply->isEnabled(),"editing remains available while note sync is in flight");
        }
        processor->prepareToPlay(48000,256);processor->playing.store(true);processor->monitor.store(true);processor->preview(60,100,true);
        juce::AudioBuffer<float> audio(2,256);juce::MidiBuffer midi;processor->processBlock(audio,midi);
        for(auto event:midi)require(!event.getMessage().isNoteOn(),"clip mode never emits duplicate local sequencer or synth MIDI");
        require(audio.getMagnitude(0,256)==0,"clip mode emits no built-in preview sound");
        {
            auto p=std::make_unique<PianoRollProcessor>();LiveClipBridge link(*p);heartbeat(folder);write(folder,"snapshot.json",snapshot(10));require(link.connect(folder).wasOk(),"connect rebase fixture");
            auto local=p->readPattern();local.notes[0].pitch=61;p->writePattern(local);require(link.apply().wasOk(),"send first automatic edit");
            auto sent=juce::JSON::parse(folder.getChildFile("request.json").loadFileAsString());local.notes[0].velocity=99;p->writePattern(local);
            auto remote=snapshot(11);remote["notes"].getArray()->getReference(0).getDynamicObject()->setProperty("pitch",61);
            auto response=juce::JSON::parse(R"({"protocol":1,"session":"test-session","requestId":"","ok":true,"revision":11,"sourceIds":[{"localId":1,"sourceId":101},{"localId":2,"sourceId":102}]})");
            response.getDynamicObject()->setProperty("requestId",sent["requestId"]);response.getDynamicObject()->setProperty("snapshot",remote);write(folder,"ack.json",response);
            require(link.poll()&&link.dirty()&&p->readPattern().notes[0].velocity==99,"edits made while saving survive acknowledgement");
            require(link.apply().wasOk(),"send rebased next edit");sent=juce::JSON::parse(folder.getChildFile("request.json").loadFileAsString());
            remote["notes"].getArray()->getReference(0).getDynamicObject()->setProperty("velocity",99);remote.getDynamicObject()->setProperty("revision",12);
            std::reverse(remote["notes"].getArray()->begin(),remote["notes"].getArray()->end());
            response.getDynamicObject()->setProperty("requestId",sent["requestId"]);response.getDynamicObject()->setProperty("revision",12);response.getDynamicObject()->setProperty("snapshot",remote);write(folder,"ack.json",response);
            require(link.poll()&&!link.dirty(),"native note order cannot cause an endless autosave loop");
            require(link.toggleTransport().wasOk(),"editor play sends transport command");link.preview(60,100,true,1);link.preview(60,0,false,1);
            juce::StringArray events;events.addLines(folder.getChildFile("events.jsonl").loadFileAsString());bool play=false,on=false,off=false;
            for(const auto& line:events){const auto e=juce::JSON::parse(line);play|=e["kind"].toString()=="transport"&&e["action"].toString()=="play";on|=e["kind"].toString()=="audition"&&static_cast<bool>(e["on"]);off|=e["kind"].toString()=="audition"&&!static_cast<bool>(e["on"]);}
            require(play&&on&&off,"transport and balanced audition events reach the companion queue");
        }
        {
            auto p=std::make_unique<PianoRollProcessor>();LiveClipBridge link(*p);heartbeat(folder);
            auto draft=snapshot(20,"empty:202");draft.getDynamicObject()->setProperty("loopStart",0);draft.getDynamicObject()->setProperty("loopEnd",16);draft.getDynamicObject()->setProperty("notes",juce::Array<juce::var>());write(folder,"snapshot.json",draft);
            require(link.connect(folder).wasOk()&&link.hasClip()&&p->readPattern().count==0,"empty target is immediately editable");
            auto score=p->readPattern();score.add({0,0,1,60,100,1,false});p->writePattern(score);
            require(link.toggleTransport().wasOk()&&link.waiting(),"play flushes new notes before starting transport");
            const auto createRequest=juce::JSON::parse(folder.getChildFile("request.json").loadFileAsString());
            auto native=snapshot(21,"999");native.getDynamicObject()->setProperty("loopStart",0);native.getDynamicObject()->setProperty("loopEnd",16);
            juce::Array<juce::var> notes;notes.add(juce::JSON::parse(R"({"note_id":9001,"pitch":60,"start_time":0,"duration":1,"velocity":100,"mute":false})"));native.getDynamicObject()->setProperty("notes",notes);
            auto createAck=juce::JSON::parse(R"({"protocol":1,"session":"test-session","ok":true,"revision":21,"sourceIds":[{"localId":1,"sourceId":9001}]})");createAck.getDynamicObject()->setProperty("requestId",createRequest["requestId"]);createAck.getDynamicObject()->setProperty("snapshot",native);write(folder,"ack.json",createAck);
            require(link.poll()&&!link.dirty()&&p->readPattern().notes[0].id==1,"new native IDs retain editor identity");
            juce::StringArray events;events.addLines(folder.getChildFile("events.jsonl").loadFileAsString());
            bool startsCreated=false;for(const auto& line:events){auto e=juce::JSON::parse(line);startsCreated|=e["kind"].toString()=="transport"&&e["targetId"].toString()=="999"&&e["action"].toString()=="play";}require(startsCreated,"queued playback starts the newly created clip");
        }
        // Remove only these test-created files, without recursive deletion.
        for(const auto* name:{"snapshot.json","heartbeat.json","request.json","ack.json","events.jsonl","telemetry.json","event-status.json"})folder.getChildFile(name).deleteFile();folder.deleteFile();
        std::cout<<"All live bridge checks passed\n";
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
