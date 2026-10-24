#pragma once
#include "Core.h"
#include <memory>
#include <random>
#include <set>
#include <vector>

namespace pr {
using Selection = std::set<uint32_t>;
inline bool included(const Note& n, const Selection& s) { return s.empty() || s.count(n.id) != 0; }
inline void quantize(Pattern& p, const Selection& s, double step, bool quantizeEnds, double strength = 1) {
    if (!(step > 0) || !std::isfinite(step)) return;
    strength = std::clamp(strength, 0.0, 1.0);
    for (int i = 0; i < p.count; ++i) {
        auto& n = p.notes[static_cast<size_t>(i)]; if (!included(n, s)) continue;
        const double originalEnd = n.start + n.length;
        n.start = std::clamp(n.start + (snap(n.start, step) - n.start) * strength, 0.0, p.loop - 1.0 / 960);
        if (quantizeEnds) n.length = std::max(1.0 / 960, originalEnd + (snap(originalEnd, step) - originalEnd) * strength - n.start);
        n.length = std::min(n.length, p.loop - n.start);
    }
}
inline bool chop(Pattern& p, Selection& selection, double step, double gate = 1) {
    if (!(step >= 1.0 / 960) || !std::isfinite(step)) return false;
    int required = 0;
    for (int i = 0; i < p.count; ++i) {
        const auto& n = p.notes[static_cast<size_t>(i)];
        required += included(n, selection) ? static_cast<int>(std::ceil(n.length / step - 1.e-9)) : 1;
        if (required > capacity) return false;
    }
    auto result = std::make_unique<Pattern>(p); result->count = 0;
    auto newSelection = selection;
    for (int i = 0; i < p.count; ++i) {
        const auto& n = p.notes[static_cast<size_t>(i)];
        if (!included(n, selection)) { result->notes[static_cast<size_t>(result->count++)] = n; continue; }
        int pieces = std::max(1, static_cast<int>(std::ceil(n.length / step - 1.e-9)));
        for (int j = 0; j < pieces; ++j) {
            auto piece = n; piece.start = n.start + j * step;
            piece.length = std::max(1.0 / 960, std::min(step, n.start + n.length - piece.start) * std::clamp(gate, 0.01, 1.0));
            piece.length = std::min(piece.length, p.loop - piece.start);
            if (j > 0) piece.id = result->nextId++;
            result->notes[static_cast<size_t>(result->count++)] = piece;
            if (!selection.empty()) newSelection.insert(piece.id);
        }
    }
    p = *result; selection = std::move(newSelection); return true;
}
inline void glue(Pattern& p, Selection& selection) {
    const auto scope = selection;
    bool merged = true;
    while (merged) {
        merged = false;
        for (int i = 0; i < p.count && !merged; ++i) {
            auto& a = p.notes[static_cast<size_t>(i)]; if (!included(a, scope)) continue;
            for (int j = 0; j < p.count; ++j) {
                const auto b = p.notes[static_cast<size_t>(j)];
                if (i == j || !included(b, scope) || a.pitch != b.pitch || a.channel != b.channel || a.muted != b.muted) continue;
                if (std::abs(a.start + a.length - b.start) < 1.e-7) {
                    a.length += b.length; selection.erase(b.id); p.remove(j); merged = true; break;
                }
            }
        }
    }
}
inline void randomizeVelocity(Pattern& p, const Selection& s, int amount, uint32_t seed) {
    std::mt19937 random(seed); amount = std::clamp(amount, 0, 126);
    std::uniform_int_distribution<int> change(-amount, amount);
    for (int i = 0; i < p.count; ++i) { auto& n = p.notes[static_cast<size_t>(i)]; if (included(n, s)) n.velocity = std::clamp(n.velocity + change(random), 1, 127); }
}
inline void scaleVelocity(Pattern& p, const Selection& s, double gain, double offset) {
    for (int i = 0; i < p.count; ++i) { auto& n = p.notes[static_cast<size_t>(i)]; if (included(n, s)) n.velocity = std::clamp(static_cast<int>(std::round(n.velocity * gain + offset)), 1, 127); }
}
inline void lfoVelocity(Pattern& p, const Selection& s, double centre, double amplitude, double cycles, double phase, int shape) {
    double first = p.loop, last = 0;
    for (int i = 0; i < p.count; ++i) { const auto& n = p.notes[static_cast<size_t>(i)]; if (included(n, s)) { first = std::min(first, n.start); last = std::max(last, n.start + n.length); } }
    for (int i = 0; i < p.count; ++i) {
        auto& n = p.notes[static_cast<size_t>(i)]; if (!included(n, s)) continue;
        double t = wrap((n.start - first) / std::max(1.0 / 960, last - first) * cycles + phase / 360, 1);
        double wave = shape == 1 ? 1 - 4 * std::abs(t - 0.5) : shape == 2 ? 2 * t - 1 : shape == 3 ? (t < 0.5 ? 1.0 : -1.0) : std::sin(t * 6.283185307179586);
        n.velocity = std::clamp(static_cast<int>(std::round(centre + amplitude * wave)), 1, 127);
    }
}
inline void strum(Pattern& p, const Selection& s, double spread, int velocitySlope, bool reverse) {
    auto original = std::make_unique<Pattern>(p); std::array<bool, capacity> visited {};
    for (int i = 0; i < p.count; ++i) {
        if (visited[static_cast<size_t>(i)] || !included(original->notes[static_cast<size_t>(i)], s)) continue;
        std::vector<int> chord;
        const auto& base = original->notes[static_cast<size_t>(i)];
        for (int j = i; j < p.count; ++j) { const auto& n = original->notes[static_cast<size_t>(j)]; if (included(n, s) && n.channel == base.channel && std::abs(n.start - base.start) < 1.e-8) { chord.push_back(j); visited[static_cast<size_t>(j)] = true; } }
        std::sort(chord.begin(), chord.end(), [&](int a, int b) { return reverse ? p.notes[static_cast<size_t>(a)].pitch > p.notes[static_cast<size_t>(b)].pitch : p.notes[static_cast<size_t>(a)].pitch < p.notes[static_cast<size_t>(b)].pitch; });
        for (size_t k = 0; k < chord.size(); ++k) {
            auto& n = p.notes[static_cast<size_t>(chord[k])];
            double fraction = chord.size() < 2 ? 0 : k / static_cast<double>(chord.size() - 1);
            n.start = std::min(p.loop - 1.0 / 960, n.start + spread * fraction); n.length = std::min(n.length, p.loop - n.start);
            n.velocity = std::clamp(n.velocity + static_cast<int>(std::round(velocitySlope * fraction)), 1, 127);
        }
    }
}
}
