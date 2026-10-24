#pragma once
#include <JuceHeader.h>
#include "Core.h"

class PianoRollProcessor final : public juce::AudioProcessor {
public:
    PianoRollProcessor();
    const juce::String getName() const override { return "Piano Roll"; }
    void prepareToPlay(double, int) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.1; }
    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override;
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Pattern"; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;
    pr::Pattern readPattern() const;
    void writePattern(const pr::Pattern&);
    void preview(int pitch, int velocity, bool on, int channel = 1);
    juce::Result importMidi(const juce::File&);
    juce::Result exportMidi(const juce::File&) const;
    juce::Result savePattern(const juce::File&);
    juce::Result loadPattern(const juce::File&);
    std::function<void(int,int,bool,int)> externalPreview;
    std::atomic<bool> externalClipMode {false};
    std::atomic<bool> playing {false}, hostSync {false}, monitor {true};
    std::atomic<double> tempo {120}, playhead {0}, reportedHostTempo {0};
    std::atomic<uint64_t> revision {0};
    std::atomic<bool> transportRunning {false};
    std::array<std::atomic<bool>, 128> keyActivity {};
private:
    mutable juce::SpinLock patternLock;
    pr::Pattern pattern, audioPattern;
    uint64_t audioRevision = ~uint64_t(0);
    juce::MidiMessageCollector previews;
    juce::MidiBuffer generated, previewMidi;
    double sampleRateHz = 48000, localBeat = 0, expectedBeat = 0;
    bool wasPlaying = false, lastSync = false;
    struct Voice { double phase = 0, increment = 0; float amplitude = 0; int pitch = -1, channel = 1; bool releasing = false; };
    std::array<Voice, 64> voices {};
    void synthEvent(const juce::MidiMessage&);
    void render(juce::AudioBuffer<float>&, int, int);
    void resetVoices();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoRollProcessor)
};
