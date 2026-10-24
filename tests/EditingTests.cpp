#include "TypingKeyboard.h"
#include <tuple>
#include <vector>
#include "Stamps.h"
#include <iostream>
#include <stdexcept>
static void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
static bool near(double a, double b) { return std::abs(a - b) < 1.e-8; }
int main() {
    try {
        auto p = std::make_unique<pr::Pattern>();
        p->add({0, 0.13, 0.37, 60, 80, 1, false}); p->add({0, 2.13, 0.37, 67, 90, 2, false});
        pr::Selection selection {p->notes[0].id};
        pr::quantize(*p, selection, 0.25, false);
        require(near(p->notes[0].start, 0.25) && near(p->notes[0].length, 0.37), "start-only quantize preserves duration");
        require(near(p->notes[1].start, 2.13), "quantize respects selection");
        pr::quantize(*p, selection, 0.25, true);
        require(near(p->notes[0].length, 0.25), "quick quantize duration");
        *p = {}; p->add({0, 130.0 / 960, 610.0 / 960, 60, 80, 1, false}); selection.clear();
        pr::quantize(*p, selection, 0.25, true);
        require(near(p->notes[0].start, 0.25) && near(p->notes[0].length, 0.5), "FL reference regression: quantize original end, not duration");
        *p = {}; p->add({0, 130.0 / 960, 610.0 / 960, 60, 80, 1, false});
        pr::quantize(*p, selection, 0.25, false);
        require(near(p->notes[0].start, 0.25) && near(p->notes[0].length, 610.0 / 960), "start-only quantize still preserves off-grid duration");
        *p = {}; p->add({0, 130.0 / 960, 610.0 / 960, 60, 80, 1, false});
        pr::quantize(*p, selection, 0.25, true, 0.5);
        require(near(p->notes[0].start, (130.0 / 960 + 0.25) / 2) && near(p->notes[0].start + p->notes[0].length, (740.0 / 960 + 0.75) / 2), "partial-strength interpolation applies to original endpoints");
        *p = {}; p->add({0, 1, 0.6, 60, 84, 3, true}); selection = {p->notes[0].id};
        require(pr::chop(*p, selection, 0.25), "chop succeeds");
        require(p->count == 3 && selection.size() == 3 && near(p->notes[2].start, 1.5) && near(p->notes[2].length, 0.1), "chop retains remainder and selection");
        require(p->notes[1].muted && p->notes[1].velocity == 84 && p->notes[1].channel == 3, "chop preserves properties");
        pr::glue(*p, selection); require(p->count == 1 && near(p->notes[0].length, 0.6) && selection.size() == 1, "chop/glue round trip");
        p->add({0, 1.6, 0.4, 60, 100, 2, false}); selection.clear(); pr::glue(*p, selection);
        require(p->count == 2, "glue does not merge different channels");
        *p = {}; for (int i = 0; i < pr::capacity; ++i) p->add({0, 0, 1, 60, 100, 1, false});
        const auto nextId = p->nextId; selection = {p->notes[0].id};
        require(!pr::chop(*p, selection, 0.25) && p->count == pr::capacity && p->nextId == nextId && selection.size() == 1, "chop capacity rejection is atomic");
        *p = {}; selection.clear(); const pr::Stamp major {"Major", "Chords", {0,4,7}};
        require(pr::placeStamp(*p, major, 2, 1, 60, 4, selection), "major stamp");
        require(p->count == 3 && p->notes[0].pitch == 60 && p->notes[1].pitch == 64 && p->notes[2].pitch == 67 && selection.size() == 3, "stamp intervals and selected group");
        auto stampId = p->nextId;
        require(!pr::placeStamp(*p, major, 4, 1, 125, 1, selection) && p->count == 3 && p->nextId == stampId, "stamp rejects pitch overflow atomically");
        for (const auto& stamp : pr::stamps()) {
            require(!stamp.intervals.empty() && stamp.intervals.front() == 0, "stamp has root");
            require(std::is_sorted(stamp.intervals.begin(), stamp.intervals.end()), "stamp intervals ordered");
            require(std::adjacent_find(stamp.intervals.begin(), stamp.intervals.end()) == stamp.intervals.end(), "stamp intervals unique");
        }
        *p = {}; for (int i = 0; i < 16; ++i) p->add({0, i * 0.25, 0.25, 60, 90, 1, false}); selection.clear();
        auto a = std::make_unique<pr::Pattern>(*p), b = std::make_unique<pr::Pattern>(*p);
        pr::randomizeVelocity(*a, selection, 30, 123); pr::randomizeVelocity(*b, selection, 30, 123);
        bool changed = false;
        for (int i = 0; i < a->count; ++i) { require(a->notes[i].velocity == b->notes[i].velocity, "seeded randomization reproducible"); changed |= a->notes[i].velocity != 90; }
        require(changed, "randomization changes velocities");
        selection = {p->notes[0].id}; pr::scaleVelocity(*p, selection, 2, 0);
        require(p->notes[0].velocity == 127 && p->notes[1].velocity == 90, "velocity scaling clamps and respects selection");
        selection.clear(); pr::lfoVelocity(*p, selection, 64, 40, 1, 90, 0);
        require(p->notes[0].velocity == 104 && p->notes[8].velocity == 24, "LFO phase and cycle");
        *p = {}; p->add({0, 1, 1, 64, 100, 1, false}); p->add({0, 1, 1, 60, 100, 1, false}); p->add({0, 1, 1, 67, 100, 1, false}); p->add({0, 3, 0.5, 72, 80, 1, false});
        pr::strum(*p, {}, 0.2, -20, false);
        require(near(p->notes[1].start, 1) && near(p->notes[0].start, 1.1) && near(p->notes[2].start, 1.2), "strum pitch ordering");
        require(p->notes[2].velocity == 80 && near(p->notes[3].start, 3), "strum velocity slope and isolated note");
        {
            pr::TypingKeyboard keyboard;
            std::vector<std::tuple<int,int,bool,int>> events;
            auto emit = [&](int pitch, int velocity, bool on, int channel) { events.emplace_back(pitch, velocity, on, channel); };
            for (int pitch = 48; pitch <= 79; ++pitch) {
                bool found = false; for (auto key : pr::TypingKeyboard::layout) found |= key.pitch == pitch;
                require(found, "typing layout covers all semitones from MIDI 48 through 79");
            }
            require(pr::TypingKeyboard::pitchFor('Z') == 48 && pr::TypingKeyboard::pitchFor('Q') == 60
                && pr::TypingKeyboard::pitchFor('=') == 78 && pr::TypingKeyboard::pitchFor(']') == 79, "FL default typing layout endpoints");
            require(keyboard.press('Q', 0, 3, 100, emit), "mapped key consumed");
            keyboard.press('Q', 0, 3, 100, emit); keyboard.press(',', 0, 3, 100, emit);
            require(events.size() == 1, "repeat and overlapping physical keys do not retrigger note");
            keyboard.releaseMissing([](int code) { return code == ','; }, emit);
            require(events.size() == 1, "overlapping key remains held until both release");
            keyboard.releaseAll(emit);
            require(events.size() == 2 && !std::get<2>(events.back()) && std::get<3>(events.back()) == 3, "release uses original MIDI channel");
            require(keyboard.press('F', 0, 1, 100, emit) && !keyboard.press('A', 0, 1, 100, emit), "locked and free typing keys differ");
            keyboard.press('Z', -3, 16, 80, emit);
            require(std::get<0>(events.back()) == 12 && std::get<3>(events.back()) == 16, "typing octave and channel");
            keyboard.releaseAll(emit); const auto count = events.size(); keyboard.releaseAll(emit);
            require(events.size() == count, "releasing again emits no redundant offs");
        }
        std::cout << "All editing tests passed; " << pr::stamps().size() << " stamps validated\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
