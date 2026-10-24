#pragma once
#include "Processor.h"
#include <map>

class LiveClipBridge {
public:
    explicit LiveClipBridge(PianoRollProcessor& p) : processor(p) {}
    juce::Result connect(const juce::File&);
    bool poll(bool force = false);
    juce::Result apply();
    juce::Result toggleTransport();
    void preview(int,int,bool,int);
    void panic();
    void updatePlayback() { telemetry(); }
    bool lastPollWasAck() const { return acceptedAcknowledgement; }
    bool autoSyncPaused() const { return paused; }
    bool auditionEnabled = true;
    juce::String targetId() const { return clipId; }
    bool connected() const;
    double loopLength() const { return end-origin; }
    bool active() const { return folder != juce::File(); }
    bool dirty() const;
    bool hasClip() const { return baseline != nullptr && clipId.isNotEmpty(); }
    bool waiting() const { return pending.isNotEmpty(); }
    juce::String message, clipName;
private:
    PianoRollProcessor& processor;
    juce::File folder;
    juce::String session, clipId, fingerprint, pending;
    int revision = 0;
    double origin = 0, end = 0, sentAt = 0;
    std::unique_ptr<pr::Pattern> baseline, sentPattern;
    std::map<int,int> held;
    bool paused = false, queuedTransport = false, acceptedAcknowledgement = false;
    juce::int64 eventSequence = 0;
    double lastKeepAlive = 0;
    juce::Result sendEvent(juce::DynamicObject*);
    void telemetry();
    std::map<uint32_t,juce::int64> sourceIds;
    juce::var read(const char*) const;
    bool online() const;
    juce::Result load(const juce::var&, const std::map<juce::int64,uint32_t>* = nullptr);
};
