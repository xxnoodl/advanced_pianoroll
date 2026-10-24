#pragma once
#include "Editing.h"
#include <string>

namespace pr {
struct Stamp {
    std::string name, category;
    std::vector<int> intervals;
};
inline const std::vector<Stamp>& stamps() {
    static const std::vector<Stamp> catalog {
        {"Augmented", "Chords", {0,4,8}}, {"Diminished", "Chords", {0,3,6}},
        {"Fifth", "Chords", {0,7}}, {"Fifth 9th", "Chords", {0,7,14}}, {"Fifth Octave", "Chords", {0,7,12}},
        {"Major", "Chords", {0,4,7}}, {"Major 7th", "Chords", {0,4,7,11}},
        {"Minor", "Chords", {0,3,7}}, {"Minor 7th", "Chords", {0,3,7,10}}, {"Minor 9th", "Chords", {0,3,7,10,14}},
        {"Octave", "Chords", {0,12}}, {"Suspended 2", "Chords", {0,2,7}}, {"Suspended 4", "Chords", {0,5,7}},
        {"6", "Advanced chords", {0,4,7,9}}, {"6add9", "Advanced chords", {0,4,7,9,14}}, {"6sus4", "Advanced chords", {0,5,7,9}},
        {"7", "Advanced chords", {0,4,7,10}}, {"7#5", "Advanced chords", {0,4,8,10}}, {"7#5#9", "Advanced chords", {0,4,8,10,15}},
        {"7#5b9", "Advanced chords", {0,4,8,10,13}}, {"7#9", "Advanced chords", {0,4,7,10,15}}, {"7#11", "Advanced chords", {0,4,7,10,18}},
        {"7add11", "Advanced chords", {0,4,7,10,17}}, {"7add13", "Advanced chords", {0,4,7,10,21}},
        {"7b5", "Advanced chords", {0,4,6,10}}, {"7b5b9", "Advanced chords", {0,4,6,10,13}}, {"7b9", "Advanced chords", {0,4,7,10,13}},
        {"7sus4", "Advanced chords", {0,5,7,10}}, {"9", "Advanced chords", {0,4,7,10,14}}, {"9#5", "Advanced chords", {0,4,8,10,14}},
        {"9#11", "Advanced chords", {0,4,7,10,14,18}}, {"9b5", "Advanced chords", {0,4,6,10,14}},
        {"11", "Advanced chords", {0,4,7,10,14,17}}, {"13", "Advanced chords", {0,4,7,10,14,17,21}}, {"add9", "Advanced chords", {0,4,7,14}},
        {"m6", "Advanced chords", {0,3,7,9}}, {"m6add9", "Advanced chords", {0,3,7,9,14}}, {"m7b5", "Advanced chords", {0,3,6,10}},
        {"m7b9", "Advanced chords", {0,3,7,10,13}}, {"m9b5", "Advanced chords", {0,3,6,10,14}}, {"m11", "Advanced chords", {0,3,7,10,14,17}},
        {"m13", "Advanced chords", {0,3,7,10,14,17,21}}, {"madd9", "Advanced chords", {0,3,7,14}}, {"m-Maj7", "Advanced chords", {0,3,7,11}},
        {"Maj7#5", "Advanced chords", {0,4,8,11}}, {"Maj7#11", "Advanced chords", {0,4,7,11,18}},
        {"Maj9", "Advanced chords", {0,4,7,11,14}}, {"Maj11", "Advanced chords", {0,4,7,11,14,17}}, {"Maj13", "Advanced chords", {0,4,7,11,14,17,21}},
        {"Major (Ionian)", "Scales", {0,2,4,5,7,9,11,12}}, {"Minor Natural (Aeolian)", "Scales", {0,2,3,5,7,8,10,12}},
        {"Minor Harmonic", "Scales", {0,2,3,5,7,8,11,12}}, {"Minor Melodic", "Scales", {0,2,3,5,7,9,11,12}},
        {"Major Pentatonic", "Scales", {0,2,4,7,9,12}}, {"Minor Pentatonic", "Scales", {0,3,5,7,10,12}},
        {"Dorian", "Scales", {0,2,3,5,7,9,10,12}}, {"Phrygian", "Scales", {0,1,3,5,7,8,10,12}},
        {"Lydian", "Scales", {0,2,4,6,7,9,11,12}}, {"Mixolydian", "Scales", {0,2,4,5,7,9,10,12}},
        {"Locrian", "Scales", {0,1,3,5,6,8,10,12}}, {"Blues", "Scales", {0,3,5,6,7,10,12}},
        {"Whole Tone", "Scales", {0,2,4,6,8,10,12}}, {"Diminished", "Scales", {0,2,3,5,6,8,9,11,12}},
        {"Phrygian Dominant", "Scales", {0,1,4,5,7,8,10,12}}
    };
    return catalog;
}
// Atomic placement: out-of-range pitches or insufficient capacity reject the whole stamp.
inline bool placeStamp(Pattern& p, const Stamp& stamp, double start, double length, int pitch, int channel, Selection& selection) {
    if (stamp.intervals.empty() || p.count + static_cast<int>(stamp.intervals.size()) > capacity || start < 0 || start >= p.loop || !std::isfinite(start) || !std::isfinite(length) || length <= 0 || !std::isfinite(p.loop) || p.loop < 4 || p.loop > 128) return false;
    for (int interval : stamp.intervals) if (pitch + interval < 0 || pitch + interval > 127) return false;
    selection.clear();
    for (int interval : stamp.intervals) { uint32_t id = p.nextId; p.add({0, start, length, pitch + interval, 100, channel, false}); selection.insert(id); }
    return true;
}
}
