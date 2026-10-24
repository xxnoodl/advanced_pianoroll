#pragma once
#include "Processor.h"
#include <map>

class LiveClipBridge {
public:
    explicit LiveClipBridge(PianoRollProcessor& p) : processor(p) {}
    juce::Result connect(const juce::File&);
    bool poll(bool force = false);
    juce::Result apply();
    juce::String targetId() const { return clipId; }
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
    std::unique_ptr<pr::Pattern> baseline;
    std::map<uint32_t,juce::int64> sourceIds;
    juce::var read(const char*) const;
    bool online() const;
    juce::Result load(const juce::var&);
};
