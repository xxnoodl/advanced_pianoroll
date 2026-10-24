#include "Core.h"
#include <iostream>
#include <stdexcept>
#include <vector>
#include <limits>
struct Event { int sample, pitch; bool on; };
static void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
int main() {
    try {
        pr::Pattern p;
        p.loop = 4;
        p.add({0, 0, 1, 60, 100, 1, false});
        std::vector<Event> events;
        auto collect = [&](int sample, const pr::Note& n, bool on) { events.push_back({sample, n.pitch, on}); };
        pr::schedule(p, 0, 0.001, 1000, collect);
        require(events.size() == 1 && events[0].on && events[0].sample == 0, "block start note-on");
        events.clear();
        pr::schedule(p, 1, 0.001, 1000, collect);
        require(events.size() == 1 && !events[0].on && events[0].sample == 0, "off exactly at next block boundary");
        events.clear();
        pr::schedule(p, 3.9, 0.001, 200, collect);
        require(events.size() == 1 && events[0].on && events[0].sample == 100, "loop crossing");
        events.clear();
        p.notes[0].length = 4;
        pr::schedule(p, 4, 0.001, 100, collect);
        require(events.size() == 2 && !events[0].on && events[1].on, "off precedes retrigger");
        events.clear(); p.notes[0].muted = true;
        pr::schedule(p, 0, 0.001, 5000, collect);
        require(events.empty(), "muted notes emit no events");
        require(pr::snap(0.37, 0.25) == 0.25, "quantization");
        require(std::abs(pr::wrap(-0.25, 4) - 3.75) < 1.e-9, "negative host pre-roll");
        pr::Pattern bounds;
        bounds.add({0, -2, 999, 999, 0, 999, false});
        require(bounds.notes[0].pitch == 127 && bounds.notes[0].velocity == 1 && bounds.notes[0].channel == 16, "input validation");
        require(bounds.notes[0].start == 0 && bounds.notes[0].length == 16, "time limits");
        require(!bounds.add({0, std::numeric_limits<double>::quiet_NaN(), 1}), "invalid time rejected");
        for (int i = 1; i < pr::capacity; ++i) require(bounds.add({}), "capacity fill");
        require(!bounds.add({}), "capacity rejects overflow");
        bounds.remove(0); require(bounds.count == pr::capacity - 1, "remove");
        // Block partition invariance across tempo values and fractional sample boundaries.
        for (double bpm : {60.0, 123.0, 300.0}) {
            pr::Pattern pattern; pattern.loop = 4;
            pattern.add({0, 0.125, 0.37, 72, 90, 1, false});
            const double step = bpm / (60 * 48000);
            std::vector<int> whole, chunks;
            pr::schedule(pattern, 0, step, 240000, [&](int s, const pr::Note&, bool on) { whole.push_back(s * 2 + (on ? 1 : 0)); });
            for (int base = 0; base < 240000; base += 257)
                pr::schedule(pattern, base * step, step, std::min(257, 240000 - base), [&](int s, const pr::Note&, bool on) { chunks.push_back((base + s) * 2 + (on ? 1 : 0)); });
            std::sort(whole.begin(), whole.end()); std::sort(chunks.begin(), chunks.end());
            require(whole == chunks, "partition invariant scheduling");
        }
        std::cout << "All sequencing tests passed\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
