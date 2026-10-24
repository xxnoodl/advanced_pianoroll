#include "Processor.h"
#include "Editor.h"

PianoRollProcessor::PianoRollProcessor() : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)) {
    for (auto& key : keyActivity) key.store(false);
    // A small editable demonstration pattern; no FL Studio assets or presets are used.
    for (int bar = 0; bar < 4; ++bar) {
        const int root = std::array<int, 4>{60, 57, 65, 67}[static_cast<size_t>(bar)];
        for (int tone : {0, 4, 7}) pattern.add({0, bar * 4.0, 1.75, root + tone, 82, 1, false});
        pattern.add({0, bar * 4.0 + 2, 0.75, root + 12, 105, 1, false});
        pattern.add({0, bar * 4.0 + 3, 0.75, root + 7, 90, 1, false});
    }
}
bool PianoRollProcessor::isBusesLayoutSupported(const BusesLayout& layout) const {
    return layout.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}
void PianoRollProcessor::prepareToPlay(double rate, int expectedBlockSize) {
    sampleRateHz = rate; previews.reset(rate); generated.ensureSize(static_cast<size_t>(std::max(65536, expectedBlockSize * 64)));
    previewMidi.ensureSize(65536);
    resetVoices(); wasPlaying = false; localBeat = 0;
}
pr::Pattern PianoRollProcessor::readPattern() const { const juce::SpinLock::ScopedLockType lock(patternLock); return pattern; }
void PianoRollProcessor::writePattern(const pr::Pattern& p) {
    const juce::SpinLock::ScopedLockType lock(patternLock); pattern = p; revision.fetch_add(1);
}
void PianoRollProcessor::preview(int pitch, int velocity, bool on, int channel) {
    if (externalClipMode.load()) return;
    auto m = on ? juce::MidiMessage::noteOn(channel, pitch, static_cast<juce::uint8>(velocity)) : juce::MidiMessage::noteOff(channel, pitch);
    m.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001); previews.addMessageToQueue(m);
}
void PianoRollProcessor::resetVoices() { for (auto& v : voices) v = {}; for (auto& key : keyActivity) key.store(false); transportRunning.store(false); }
void PianoRollProcessor::synthEvent(const juce::MidiMessage& m) {
    if (m.isAllNotesOff() || m.isAllSoundOff()) { for (auto& v : voices) if (v.channel == m.getChannel()) v.releasing = true; }
    if (m.isNoteOff()) for (auto& v : voices) if (v.pitch == m.getNoteNumber() && v.channel == m.getChannel()) v.releasing = true;
    if (m.isNoteOn()) {
        Voice* chosen = &voices.front();
        for (auto& v : voices) {
            if (v.pitch == m.getNoteNumber() && v.channel == m.getChannel()) v.releasing = true;
            if (v.amplitude < chosen->amplitude) chosen = &v;
        }
        *chosen = {0, juce::MathConstants<double>::twoPi * juce::MidiMessage::getMidiNoteInHertz(m.getNoteNumber()) / sampleRateHz,
                   m.getFloatVelocity() * 0.12f, m.getNoteNumber(), m.getChannel(), false};
    }
}
void PianoRollProcessor::render(juce::AudioBuffer<float>& audio, int begin, int end) {
    for (int s = begin; s < end; ++s) {
        float value = 0;
        for (auto& v : voices) if (v.amplitude > 0.00001f) {
            value += v.amplitude * static_cast<float>(std::sin(v.phase) + 0.2 * std::sin(v.phase * 2));
            v.phase = std::fmod(v.phase + v.increment, juce::MathConstants<double>::twoPi);
            v.amplitude *= v.releasing ? static_cast<float>(std::exp(-1.0 / (sampleRateHz * 0.012))) : 0.9999995f;
        }
        if (monitor.load()) for (int ch = 0; ch < audio.getNumChannels(); ++ch) audio.setSample(ch, s, std::tanh(value));
    }
}
void PianoRollProcessor::processBlock(juce::AudioBuffer<float>& audio, juce::MidiBuffer& midi) {
    juce::ScopedNoDenormals noDenormals;
    audio.clear();
    const auto currentRevision = revision.load();
    bool changed = false;
    if (currentRevision != audioRevision) {
        const juce::SpinLock::ScopedTryLockType lock(patternLock);
        if (lock.isLocked()) { audioPattern = pattern; audioRevision = revision.load(); changed = true; }
    }
    bool running = playing.load(), sync = hostSync.load();
    if (externalClipMode.load()) { running = false; sync = false; }
    double bpm = std::clamp(tempo.load(), 20.0, 300.0), beat = localBeat;
    double hostTempo = 0;
    if (sync) {
        running = false;
        if (auto* ph = getPlayHead()) if (auto pos = ph->getPosition()) {
            running = pos->getIsPlaying();
            if (auto b = pos->getBpm(); b && std::isfinite(*b)) { bpm = std::clamp(*b, 20.0, 300.0); hostTempo = bpm; }
            if (auto q = pos->getPpqPosition()) beat = *q;
            else running = false;
        }
    }
    reportedHostTempo.store(hostTempo);
    const auto step = bpm / (60 * sampleRateHz);
    const bool jumped = running && wasPlaying && std::abs(beat - expectedBeat) > step * 2;
    generated.clear();
    if ((wasPlaying && !running) || jumped || changed || sync != lastSync) {
        for (int ch = 1; ch <= 16; ++ch) generated.addEvent(juce::MidiMessage::allNotesOff(ch), 0);
    }
    if (running) {
        // Chase held notes when starting, seeking, or replacing an edited pattern.
        if (!wasPlaying || jumped || changed || sync != lastSync) {
            auto cursor = pr::wrap(beat, audioPattern.loop);
            for (int i = 0; i < audioPattern.count; ++i) {
                const auto& n = audioPattern.notes[static_cast<size_t>(i)];
                if (!n.muted && cursor > n.start + 1.e-8 && cursor < n.start + n.length - 1.e-8)
                    generated.addEvent(juce::MidiMessage::noteOn(n.channel, n.pitch, static_cast<juce::uint8>(n.velocity)), 0);
            }
        }
        pr::schedule(audioPattern, beat, step, audio.getNumSamples(), [&](int sample, const pr::Note& n, bool on) {
            generated.addEvent(on ? juce::MidiMessage::noteOn(n.channel, n.pitch, static_cast<juce::uint8>(n.velocity))
                                  : juce::MidiMessage::noteOff(n.channel, n.pitch), sample);
        });
    }
    previewMidi.clear();
    previews.removeNextBlockOfMessages(previewMidi, audio.getNumSamples());
    generated.addEvents(midi, 0, audio.getNumSamples(), 0);
    generated.addEvents(previewMidi, 0, audio.getNumSamples(), 0);
    int frame = 0;
    for (const auto metadata : generated) {
        const int next = std::clamp(metadata.samplePosition, frame, audio.getNumSamples());
        render(audio, frame, next); synthEvent(metadata.getMessage()); frame = next;
    }
    render(audio, frame, audio.getNumSamples());
    for (auto& key : keyActivity) key.store(false, std::memory_order_relaxed);
    for (const auto& voice : voices) if (voice.pitch >= 0 && voice.amplitude > 0.00001f && !voice.releasing)
        keyActivity[static_cast<size_t>(voice.pitch)].store(true, std::memory_order_relaxed);
    transportRunning.store(running);
    midi.swapWith(generated);
    wasPlaying = running; lastSync = sync;
    expectedBeat = beat + (running ? audio.getNumSamples() * step : 0);
    if (!sync) localBeat = running ? expectedBeat : 0;
    playhead.store(running ? pr::wrap(beat, audioPattern.loop) : 0);
}
void PianoRollProcessor::getStateInformation(juce::MemoryBlock& destination) {
    const auto p = readPattern(); juce::ValueTree root("PianoRoll");
    root.setProperty("version", 1, nullptr); root.setProperty("loop", p.loop, nullptr);
    root.setProperty("tempo", tempo.load(), nullptr); root.setProperty("hostSync", hostSync.load(), nullptr); root.setProperty("monitor", monitor.load(), nullptr);
    for (int i = 0; i < p.count; ++i) {
        const auto& n = p.notes[static_cast<size_t>(i)]; juce::ValueTree child("Note");
        child.setProperty("start", n.start, nullptr); child.setProperty("length", n.length, nullptr);
        child.setProperty("pitch", n.pitch, nullptr); child.setProperty("velocity", n.velocity, nullptr);
        child.setProperty("channel", n.channel, nullptr); child.setProperty("muted", n.muted, nullptr); root.addChild(child, -1, nullptr);
    }
    if (auto xml = root.createXml()) copyXmlToBinary(*xml, destination);
}
void PianoRollProcessor::setStateInformation(const void* data, int size) {
    auto xml = getXmlFromBinary(data, size); if (!xml || !xml->hasTagName("PianoRoll")) return;
    auto root = juce::ValueTree::fromXml(*xml);
    if (static_cast<int>(root.getProperty("version", 0)) != 1) return;
    pr::Pattern p;
    const auto loop = static_cast<double>(root.getProperty("loop", 16.0));
    p.loop = std::isfinite(loop) ? std::clamp(loop, 4.0, 128.0) : 16;
    for (const auto& child : root) {
        if (!child.hasType("Note")) continue;
        double start = child.getProperty("start", 0.0), length = child.getProperty("length", 1.0);
        if (!std::isfinite(start) || !std::isfinite(length)) continue;
        p.add({0, start, length, static_cast<int>(child.getProperty("pitch", 60)), static_cast<int>(child.getProperty("velocity", 100)),
               static_cast<int>(child.getProperty("channel", 1)), static_cast<bool>(child.getProperty("muted", false))});
    }
    const auto bpm = static_cast<double>(root.getProperty("tempo", 120.0));
    tempo.store(std::isfinite(bpm) ? std::clamp(bpm, 20.0, 300.0) : 120);
    hostSync.store(root.getProperty("hostSync", false)); monitor.store(root.getProperty("monitor", true));
    playing.store(false); writePattern(p);
}
juce::Result PianoRollProcessor::savePattern(const juce::File& file) {
    juce::MemoryBlock data; getStateInformation(data);
    return file.replaceWithData(data.getData(), data.getSize()) ? juce::Result::ok() : juce::Result::fail("Could not save pattern.");
}
juce::Result PianoRollProcessor::loadPattern(const juce::File& file) {
    juce::MemoryBlock data; if (!file.loadFileAsData(data)) return juce::Result::fail("Could not read pattern.");
    auto xml = getXmlFromBinary(data.getData(), static_cast<int>(data.getSize()));
    if (!xml || !xml->hasTagName("PianoRoll") || xml->getIntAttribute("version") != 1) return juce::Result::fail("Unsupported pattern file.");
    setStateInformation(data.getData(), static_cast<int>(data.getSize())); return juce::Result::ok();
}
juce::Result PianoRollProcessor::importMidi(const juce::File& file) {
    auto input = file.createInputStream(); juce::MidiFile midi;
    if (!input || !midi.readFrom(*input)) return juce::Result::fail("Could not read MIDI file.");
    const auto ppq = midi.getTimeFormat();
    if (ppq <= 0) return juce::Result::fail("SMPTE MIDI timing is not supported; use a PPQ MIDI file.");
    pr::Pattern p; p.loop = 128; double last = 4;
    bool overflow = false;
    for (int track = 0; track < midi.getNumTracks(); ++track) {
        juce::MidiMessageSequence sequence(*midi.getTrack(track)); sequence.updateMatchedPairs();
        for (int i = 0; i < sequence.getNumEvents(); ++i) {
            auto* e = sequence.getEventPointer(i);
            if (!e->message.isNoteOn()) continue;
            double start = e->message.getTimeStamp() / ppq;
            if (start >= 128) return juce::Result::fail("MIDI exceeds the 32-bar pattern limit; split it into shorter files.");
            double length = e->noteOffObject ? (e->noteOffObject->message.getTimeStamp() - e->message.getTimeStamp()) / ppq : 0.25;
            last = std::max(last, start + length);
            overflow |= !p.add({0, start, length, e->message.getNoteNumber(), e->message.getVelocity(), e->message.getChannel(), false});
        }
    }
    if (overflow) return juce::Result::fail("MIDI exceeds 2048 notes; nothing was imported.");
    if (last > 128) return juce::Result::fail("MIDI exceeds the 32-bar pattern limit.");
    p.loop = std::clamp(std::ceil(last / 4) * 4, 4.0, 128.0); writePattern(p); return juce::Result::ok();
}
juce::Result PianoRollProcessor::exportMidi(const juce::File& file) const {
    auto p = readPattern(); juce::MidiMessageSequence sequence;
    sequence.addEvent(juce::MidiMessage::tempoMetaEvent(static_cast<int>(60000000 / tempo.load())));
    for (int i = 0; i < p.count; ++i) {
        const auto& n = p.notes[static_cast<size_t>(i)]; if (n.muted) continue;
        auto on = juce::MidiMessage::noteOn(n.channel, n.pitch, static_cast<juce::uint8>(n.velocity)); on.setTimeStamp(n.start * 960); sequence.addEvent(on);
        auto off = juce::MidiMessage::noteOff(n.channel, n.pitch); off.setTimeStamp((n.start + n.length) * 960); sequence.addEvent(off);
    }
    auto end = juce::MidiMessage::endOfTrack(); end.setTimeStamp(p.loop * 960); sequence.addEvent(end);
    juce::MidiFile midi; midi.setTicksPerQuarterNote(960); midi.addTrack(sequence);
    juce::MemoryOutputStream output;
    if (!midi.writeTo(output)) return juce::Result::fail("MIDI encoding failed.");
    return file.replaceWithData(output.getData(), output.getDataSize()) ? juce::Result::ok() : juce::Result::fail("Could not write MIDI file.");
}
juce::AudioProcessorEditor* PianoRollProcessor::createEditor() { return new PianoRollEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PianoRollProcessor(); }
