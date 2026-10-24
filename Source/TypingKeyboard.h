#pragma once
#include <array>
#include <algorithm>

namespace pr {
// Independently implemented from the installed default Piano layout's key/note assignments.
class TypingKeyboard {
public:
    struct Key { int code, pitch; };
    inline static constexpr std::array<Key, 44> layout {{{'Z',48},{'X',50},{'C',52},{'V',53},{'B',55},{'N',57},{'M',59},{',',60},{'.',62},{'/',64},
        {'S',49},{'D',51},{'G',54},{'H',56},{'J',58},{'L',61},{';',63},
        {'Q',60},{'W',62},{'E',64},{'R',65},{'T',67},{'Y',69},{'U',71},{'I',72},{'O',74},{'P',76},{'[',77},{']',79},
        {'2',61},{'3',63},{'5',66},{'6',68},{'7',70},{'9',73},{'0',75},{'=',78},
        {'1',-1},{'4',-1},{'8',-1},{'-',-1},{'F',-1},{'K',-1},{39,-1}}};
    static int pitchFor(int code) {
        if (code >= 'a' && code <= 'z') code -= 'a' - 'A';
        for (auto key : layout) if (key.code == code) return key.pitch;
        return -2; // Free key; -1 is locked (no note or editor shortcut).
    }
    template<class Emit> bool press(int code, int octaveOffset, int channel, int velocity, Emit emit) {
        if (code >= 'a' && code <= 'z') code -= 'a' - 'A';
        const int mapped = pitchFor(code);
        if (mapped == -2) return false;
        if (mapped < 0) return true;
        for (auto key : held) if (key.code == code) return true; // OS repeat.
        const int pitch = std::clamp(mapped + octaveOffset * 12, 0, 127);
        for (auto& key : held) if (key.code == 0) {
            key = {code, pitch, channel};
            if (references[static_cast<size_t>((channel - 1) * 128 + pitch)]++ == 0) emit(pitch, velocity, true, channel);
            break;
        }
        return true;
    }
    template<class IsDown, class Emit> void releaseMissing(IsDown isDown, Emit emit) {
        for (auto& key : held) if (key.code && !isDown(key.code)) {
            if (--references[static_cast<size_t>((key.channel - 1) * 128 + key.pitch)] == 0) emit(key.pitch, 0, false, key.channel);
            key = {};
        }
    }
    template<class Emit> void releaseAll(Emit emit) { releaseMissing([](int) { return false; }, emit); }
private:
    struct Held { int code = 0, pitch = 0, channel = 1; };
    std::array<Held, 44> held {};
    std::array<int, 16 * 128> references {};
};
}
