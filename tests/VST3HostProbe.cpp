// This executable hosts the delivered DLL. It never links the PianoRoll shared-code target.
#include <juce_audio_utils/juce_audio_utils.h>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <limits>

static void require(bool condition, const juce::String& message) {
    if (!condition) throw std::runtime_error(message.toStdString());
}
struct HostPosition final : juce::AudioPlayHead {
    double sampleRate = 48000, bpm = 123;
    double beatOverride = std::numeric_limits<double>::quiet_NaN();
    int64_t samples = 0;
    bool playing = true;
    juce::Optional<PositionInfo> getPosition() const override {
        PositionInfo p; p.setBpm(bpm); p.setTimeSignature(TimeSignature {4, 4});
        p.setTimeInSamples(samples); p.setTimeInSeconds(samples / sampleRate);
        p.setPpqPosition(std::isfinite(beatOverride) ? beatOverride : samples * bpm / (60 * sampleRate)); p.setIsPlaying(playing);
        return p;
    }
};
static juce::MemoryBlock componentState(bool monitor = true) {
    juce::XmlElement root("PianoRoll"); root.setAttribute("version", 1); root.setAttribute("loop", 4.0);
    root.setAttribute("tempo", 123.0); root.setAttribute("hostSync", true); root.setAttribute("monitor", monitor);
    auto note = [&](double start, double length, int pitch, int channel) {
        auto* n = root.createNewChildElement("Note"); n->setAttribute("start", start); n->setAttribute("length", length);
        n->setAttribute("pitch", pitch); n->setAttribute("velocity", 100); n->setAttribute("channel", channel); n->setAttribute("muted", false);
    };
    note(0, 0.25, 60, 1); note(0.5, 0.125, 67, 3);
    juce::MemoryBlock raw; juce::AudioProcessor::copyXmlToBinary(root, raw); return raw;
}
static juce::MemoryBlock wrappedState(const juce::MemoryBlock& raw) {
    // JUCE's host stores VST3 component/controller streams inside this outer envelope.
    juce::XmlElement root("VST3PluginState"); root.createNewChildElement("IComponent")->addTextElement(raw.toBase64Encoding());
    juce::MemoryBlock state; juce::AudioProcessor::copyXmlToBinary(root, state); return state;
}
static std::unique_ptr<juce::XmlElement> readComponentState(juce::AudioPluginInstance& plugin) {
    juce::MemoryBlock state; plugin.getStateInformation(state);
    auto outer = juce::AudioProcessor::getXmlFromBinary(state.getData(), static_cast<int>(state.getSize()));
    require(outer != nullptr, "Host state is not readable"); auto* child = outer->getChildByName("IComponent");
    require(child != nullptr, "No component state in VST3 state"); juce::MemoryBlock raw;
    require(raw.fromBase64Encoding(child->getAllSubText()), "Invalid component-state encoding");
    auto root = juce::AudioProcessor::getXmlFromBinary(raw.getData(), static_cast<int>(raw.getSize()));
    require(root != nullptr && root->hasTagName("PianoRoll"), "VST3 returned invalid processor state"); return root;
}
struct Event { int64_t sample; int pitch, channel; bool on; };
static bool contains(const std::vector<Event>& events, int64_t sample, int pitch, int channel, bool on) {
    int matches = 0;
    for (auto e : events) if (e.sample == sample && e.pitch == pitch && e.channel == channel && e.on == on) ++matches;
    return matches == 1;
}
int main(int argc, char** argv) {
    juce::ScopedJuceInitialiser_GUI initialise;
    juce::Array<juce::var> cases;
    juce::File report;
    auto log = [](const juce::String& message) { std::cout << message << std::endl; };
    try {
        require(argc >= 3, "Usage: PianoRollHostProbe <bundle.vst3> <report.json>");
        const juce::File bundle(juce::String::fromUTF8(argv[1])); report = juce::File(juce::String::fromUTF8(argv[2]));
        require(bundle.exists(), "VST3 bundle does not exist");
        juce::VST3PluginFormat format; juce::OwnedArray<juce::PluginDescription> descriptions;
        format.findAllTypesForFile(descriptions, bundle.getFullPathName()); require(descriptions.size() == 1, "Expected one instrument in VST3 bundle");
        log("SCANNED: " + descriptions[0]->name + " " + descriptions[0]->version);
        juce::String error;
        auto plugin = format.createInstanceFromDescription(*descriptions[0], 48000, 256, error);
        require(plugin != nullptr, "VST3 instance creation failed: " + error);
        require(plugin->getTotalNumOutputChannels() == 2 && plugin->acceptsMidi() && plugin->producesMidi(), "Stereo/MIDI bus contract mismatch");
        for (int i = 0; i < 3; ++i) {
            std::unique_ptr<juce::AudioProcessorEditor> editor(plugin->createEditorIfNeeded());
            require(editor != nullptr && editor->getWidth() >= 1080 && editor->getHeight() >= 600, "VST3 editor creation/size failed");
        }
        log("PASS: DLL editor creation/recreation (native window attachment not tested)");

        int matrixCases = 0;
        for (double rate : {44100.0, 48000.0, 96000.0}) for (int size : {64, 257, 1024, 8192}) for (bool offline : {false, true}) {
            HostPosition position; position.sampleRate = rate; plugin->setPlayHead(&position); plugin->setNonRealtime(offline);
            auto state = wrappedState(componentState()); plugin->setStateInformation(state.getData(), static_cast<int>(state.getSize()));
            auto restored = readComponentState(*plugin);
            require(restored->getNumChildElements() == 2 && restored->getBoolAttribute("hostSync"), "Known pattern state did not load through VST3");
            plugin->prepareToPlay(rate, size);
            const double samplesPerBeat = rate * 60 / position.bpm;
            const int64_t total = static_cast<int64_t>(std::ceil(5 * samplesPerBeat));
            std::vector<Event> events; float peak = 0; bool finite = true; int blocks = 0;
            for (int64_t base = 0; base < total; base += size) {
                int count = static_cast<int>(std::min<int64_t>(size, total - base));
                juce::AudioBuffer<float> audio(2, count); audio.clear(); juce::MidiBuffer midi;
                if (base == 0) midi.addEvent(juce::MidiMessage::noteOn(5, 72, static_cast<juce::uint8>(90)), 17);
                if (base == size) midi.addEvent(juce::MidiMessage::noteOff(5, 72), 13);
                position.samples = base; plugin->processBlock(audio, midi); ++blocks;
                for (int ch = 0; ch < audio.getNumChannels(); ++ch) for (int s = 0; s < count; ++s) {
                    const auto value = audio.getSample(ch, s); finite &= std::isfinite(value); peak = std::max(peak, std::abs(value));
                }
                for (auto event : midi) { auto message = event.getMessage(); if (message.isNoteOnOrOff()) events.push_back({base + event.samplePosition, message.getNoteNumber(), message.getChannel(), message.isNoteOn()}); }
            }
            require(finite && peak > 0.001f && peak <= 1.001f, "Invalid preview audio through VST3 at " + juce::String(rate) + "/" + juce::String(size));
            auto frame = [&](double beat) { return static_cast<int64_t>(std::ceil(beat * samplesPerBeat - 1.e-7)); };
            for (double cycle : {0.0, 4.0}) {
                require(contains(events, frame(cycle), 60, 1, true), "Missing/mistimed C note-on through VST3");
                require(contains(events, frame(cycle + 0.25), 60, 1, false), "Missing/mistimed C note-off through VST3");
                require(contains(events, frame(cycle + 0.5), 67, 3, true), "Missing/mistimed/channel G note-on through VST3");
                require(contains(events, frame(cycle + 0.625), 67, 3, false), "Missing/mistimed G note-off through VST3");
            }
            require(contains(events, 17, 72, 5, true) && contains(events, size + 13, 72, 5, false), "VST3 MIDI passthrough lost sample/channel information");
            require(events.size() == 10, "Unexpected duplicate/missing VST3 note events");
            position.playing = false; position.samples = total;
            juce::AudioBuffer<float> stoppedAudio(2, size); juce::MidiBuffer stoppedMidi; plugin->processBlock(stoppedAudio, stoppedMidi);
            int released = 0; for (auto event : stoppedMidi) if (event.getMessage().isAllNotesOff()) ++released;
            require(released == 16, "Stop did not release all MIDI channels through VST3");
            plugin->releaseResources(); plugin->setPlayHead(nullptr);
            auto* item = new juce::DynamicObject(); item->setProperty("sampleRate", rate); item->setProperty("blockSize", size);
            item->setProperty("offline", offline); item->setProperty("blocks", blocks); item->setProperty("noteEvents", static_cast<int>(events.size())); item->setProperty("peak", peak); item->setProperty("passed", true); cases.add(juce::var(item));
            ++matrixCases; log("PASS: " + juce::String(rate) + " Hz / " + juce::String(size) + " samples / " + (offline ? "offline" : "realtime flag"));
        }
        HostPosition changing; changing.bpm = 120; plugin->setPlayHead(&changing); plugin->setNonRealtime(false);
        auto changingState = wrappedState(componentState()); plugin->setStateInformation(changingState.getData(), static_cast<int>(changingState.getSize())); plugin->prepareToPlay(48000, 8192);
        juce::AudioBuffer<float> beforeTempo(2, 256); juce::MidiBuffer beforeMidi; plugin->processBlock(beforeTempo, beforeMidi);
        changing.samples = 256; changing.beatOverride = 256.0 / 24000; changing.bpm = 240;
        juce::AudioBuffer<float> afterTempo(2, 4096); juce::MidiBuffer afterMidi; plugin->processBlock(afterTempo, afterMidi);
        bool correctOff = false; for (auto event : afterMidi) { auto m = event.getMessage(); if (m.isNoteOff() && m.getNoteNumber() == 60 && event.samplePosition == 2872) correctOff = true; }
        require(correctOff, "Sustained note-off did not follow a continuous-PPQ tempo change through VST3");
        changing.samples = 3000; changing.bpm = 120; changing.beatOverride = 0.125;
        juce::AudioBuffer<float> seekAudio(2, 256); juce::MidiBuffer seekMidi; plugin->processBlock(seekAudio, seekMidi);
        int chased = 0, seekReleases = 0;
        for (auto event : seekMidi) { auto m = event.getMessage(); if (m.isNoteOn() && m.getNoteNumber() == 60 && event.samplePosition == 0) ++chased; if (m.isAllNotesOff()) ++seekReleases; }
        require(chased == 1 && seekReleases == 16, "Seek did not release channels and chase the held note through VST3");
        plugin->releaseResources(); plugin->setPlayHead(nullptr); log("PASS: continuous-PPQ tempo change and seek/chase through VST3");

        auto saved = wrappedState(componentState(false)); plugin->setStateInformation(saved.getData(), static_cast<int>(saved.getSize()));
        juce::MemoryBlock hostSaved; plugin->getStateInformation(hostSaved);
        auto second = format.createInstanceFromDescription(*descriptions[0], 48000, 256, error); require(second != nullptr, "Second instance creation failed");
        second->setStateInformation(hostSaved.getData(), static_cast<int>(hostSaved.getSize()));
        auto secondState = readComponentState(*second); require(secondState->getNumChildElements() == 2 && !secondState->getBoolAttribute("monitor"), "State round trip into fresh VST3 instance failed");
        HostPosition silentPosition; second->setPlayHead(&silentPosition); second->prepareToPlay(48000, 256);
        juce::AudioBuffer<float> silentAudio(2, 256); juce::MidiBuffer silentMidi; second->processBlock(silentAudio, silentMidi);
        require(silentAudio.getMagnitude(0, 256) == 0 && !silentMidi.isEmpty(), "Monitor-off state did not retain MIDI output through fresh VST3 instance");
        auto isolated = readComponentState(*second); isolated->setAttribute("tempo", 200.0); isolated->setAttribute("monitor", true);
        juce::MemoryBlock isolatedRaw; juce::AudioProcessor::copyXmlToBinary(*isolated, isolatedRaw); auto isolatedWrapped = wrappedState(isolatedRaw);
        second->setStateInformation(isolatedWrapped.getData(), static_cast<int>(isolatedWrapped.getSize()));
        auto unchanged = readComponentState(*plugin); require(unchanged->getDoubleAttribute("tempo") == 123 && !unchanged->getBoolAttribute("monitor"), "One instance changed another instance's state");
        second->releaseResources(); second->setPlayHead(nullptr); second.reset(); plugin.reset();
        auto* result = new juce::DynamicObject(); result->setProperty("passed", true); result->setProperty("plugin", bundle.getFullPathName()); result->setProperty("version", descriptions[0]->version);
        result->setProperty("matrixCases", matrixCases); result->setProperty("cases", juce::var(cases)); result->setProperty("freshInstanceStateRecall", true); result->setProperty("editorCreations", 3);
        result->setProperty("continuousTempoChange", true); result->setProperty("seekAndChase", true); result->setProperty("instanceStateIsolation", true);
        require(report.replaceWithText(juce::JSON::toString(juce::var(result), false)), "Could not save host probe report");
        log("ALL BINARY HOST CHECKS PASSED");
    } catch (const std::exception& e) {
        std::cerr << "FAIL: " << e.what() << std::endl;
        if (report != juce::File()) { auto* result = new juce::DynamicObject(); result->setProperty("passed", false); result->setProperty("error", juce::String(e.what())); result->setProperty("completedCases", juce::var(cases)); report.replaceWithText(juce::JSON::toString(juce::var(result), false)); }
        return 1;
    }
}
