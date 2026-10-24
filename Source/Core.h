#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace pr {
constexpr int capacity = 2048;
struct Note {
    uint32_t id = 0;
    double start = 0, length = 1;
    int pitch = 60, velocity = 100, channel = 1;
    bool muted = false;
};
struct Pattern {
    std::array<Note, capacity> notes {};
    int count = 0;
    double loop = 16;
    uint32_t nextId = 1;
    bool add(Note n) {
        if (count >= capacity || !std::isfinite(n.start) || !std::isfinite(n.length)
            || !std::isfinite(loop) || loop < 4 || loop > 128) return false;
        n.id = nextId++;
        n.start = std::clamp(n.start, 0.0, loop - 1.0 / 960);
        n.length = std::clamp(n.length, 1.0 / 960, loop - n.start);
        n.pitch = std::clamp(n.pitch, 0, 127);
        n.velocity = std::clamp(n.velocity, 1, 127);
        n.channel = std::clamp(n.channel, 1, 16);
        notes[static_cast<size_t>(count++)] = n;
        return true;
    }
    void remove(int i) {
        if (i < 0 || i >= count) return;
        for (int j = i; j < count - 1; ++j) notes[static_cast<size_t>(j)] = notes[static_cast<size_t>(j + 1)];
        --count;
    }
};
inline double snap(double value, double step) { return step > 0 ? std::round(value / step) * step : value; }
inline double wrap(double value, double loop) { return value - std::floor(value / loop) * loop; }

// Half-open block intervals prevent duplicate events at adjacent block boundaries.
// Events at a loop end precede the next loop's note-ons after MIDI buffer sorting.
template <typename Emit>
void schedule(const Pattern& p, double beat, double beatsPerSample, int samples, Emit emit) {
    if (!(p.loop >= 4 && p.loop <= 128) || !(beatsPerSample > 0) || samples <= 0 || !std::isfinite(beat)) return;
    const auto end = beat + samples * beatsPerSample;
    const auto firstCycle = static_cast<int64_t>(std::floor(beat / p.loop)) - 1;
    const auto lastCycle = static_cast<int64_t>(std::floor(end / p.loop));
    if (lastCycle - firstCycle > 128) return;
    for (int pass = 0; pass < 2; ++pass) {
        for (int i = 0; i < p.count; ++i) {
            const auto& n = p.notes[static_cast<size_t>(i)];
            if (n.muted) continue;
            for (auto cycle = firstCycle; cycle <= lastCycle; ++cycle) {
                const auto t = cycle * p.loop + n.start + (pass == 0 ? n.length : 0);
                const auto frame = static_cast<int64_t>(std::ceil((t - beat) / beatsPerSample - 1.e-7));
                if (frame >= 0 && frame < samples)
                    emit(static_cast<int>(frame), n, pass == 1);
            }
        }
    }
}
}
