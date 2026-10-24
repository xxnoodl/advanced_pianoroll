#include "Editor.h"

namespace {
const juce::Colour background(0xff202220), panel(0xff2c2832), accent(0xffacd67c), orange(0xffffa642), selectedColour(0xffd88c98), playColour(0xffeb4b73);
bool black(int pitch) { const int n = pitch % 12; return n == 1 || n == 3 || n == 6 || n == 8 || n == 10; }
juce::Colour colourFor(int channel) { return juce::Colour::fromHSV(std::fmod(0.28f + (channel - 1) * 0.097f, 1.f), 0.38f, 0.84f, 1.f); }
}
RollCanvas::RollCanvas(PianoRollProcessor& p) : processor(p), pattern(p.readPattern()), before(pattern) {
    setWantsKeyboardFocus(true);
    seenRevision = processor.revision.load();
    for (auto* bar : {&horizontal, &vertical}) {
        addAndMakeVisible(bar); bar->addListener(this); bar->setAutoHide(false);
        bar->setColour(juce::ScrollBar::backgroundColourId, panel.darker());
        bar->setColour(juce::ScrollBar::thumbColourId, juce::Colour(0xff62616b));
    }
}
RollCanvas::~RollCanvas() { releaseTyping(); cancelProcessing(); if (processingWindow) processingWindow->exitModalState(0); if (audition >= 0) processor.preview(audition, 0, false); }
void RollCanvas::releaseTyping() {
    typingState.releaseAll([this](int pitch, int velocity, bool on, int ch) { processor.preview(pitch, velocity, on, ch); });
}
void RollCanvas::setTypingEnabled(bool enabled) {
    releaseTyping(); typingEnabled = enabled;
    status = enabled ? "Typing piano ON  /  Z = C4, Q = C5  /  Ctrl+T to disable" : "Typing piano OFF  /  editor letter shortcuts enabled";
    if (onTypingChanged) onTypingChanged(); repaint();
}
void RollCanvas::pollTyping() {
    if (!hasKeyboardFocus(true)) { releaseTyping(); return; }
    typingState.releaseMissing([](int code) { return juce::KeyPress::isKeyCurrentlyDown(code); },
        [this](int pitch, int velocity, bool on, int ch) { processor.preview(pitch, velocity, on, ch); });
}
bool RollCanvas::keyStateChanged(bool) { pollTyping(); return false; }
void RollCanvas::focusLost(FocusChangeType) { releaseTyping(); }
juce::Rectangle<float> RollCanvas::noteRect(const pr::Note& n) const {
    return {static_cast<float>(68 + (n.start - offset) * pxBeat), static_cast<float>(gridTop + (topPitch - n.pitch) * row + 1),
            static_cast<float>(std::max(3.0, n.length * pxBeat - 1)), static_cast<float>(row - 2)};
}
void RollCanvas::resized() {
    velocityHeight = std::clamp(velocityHeight, 60, std::max(60, getHeight() - gridTop - 100));
    horizontal.setBounds(68, getHeight() - 14, getWidth() - 82, 14);
    vertical.setBounds(getWidth() - 14, gridTop, 14, laneTop() - gridTop);
    updateScrollbars();
}
void RollCanvas::updateScrollbars() {
    const double view = std::min(pattern.loop, std::max(1.0, (getWidth() - 82) / pxBeat));
    offset = std::clamp(offset, 0.0, pattern.loop - view);
    horizontal.setRangeLimits(0, pattern.loop, juce::dontSendNotification);
    horizontal.setCurrentRange(offset, view, juce::dontSendNotification);
    const double rows = std::clamp((laneTop() - gridTop) / static_cast<double>(row), 1.0, 128.0);
    topPitch = std::clamp(topPitch, static_cast<int>(std::ceil(rows)) - 1, 127);
    vertical.setRangeLimits(0, 128, juce::dontSendNotification);
    vertical.setCurrentRange(127 - topPitch, rows, juce::dontSendNotification);
}
void RollCanvas::scrollBarMoved(juce::ScrollBar* bar, double value) {
    if (bar == &horizontal) offset = value; else topPitch = 127 - static_cast<int>(std::round(value));
    repaint();
}
void RollCanvas::overviewAt(float x) {
    double view = (getWidth() - 82) / pxBeat;
    offset = std::clamp((x - 68) / std::max(1, getWidth() - 82) * pattern.loop - view / 2, 0.0, std::max(0.0, pattern.loop - view));
    updateScrollbars(); repaint();
}
int RollCanvas::hit(juce::Point<float> point) const {
    for (int i = pattern.count - 1; i >= 0; --i) if (noteRect(pattern.notes[static_cast<size_t>(i)]).contains(point)) return i;
    return -1;
}
#include "EditorPainting.inc"
void RollCanvas::checkpoint() {
    history.push_back(pattern); if (history.size() > 64) history.erase(history.begin()); future.clear(); before = pattern;
}
void RollCanvas::publish() { processor.writePattern(pattern); seenRevision = processor.revision.load(); repaint(); }
void RollCanvas::refresh(bool retainEditingState) {
    if(seenRevision!=processor.revision.load()) {
        processingBaseline.reset();setEnabled(true);pattern=processor.readPattern();seenRevision=processor.revision.load();
        if(!retainEditingState){selected.clear();history.clear();future.clear();rangeStart=rangeEnd=0;}
        else {for(auto it=selected.begin();it!=selected.end();){bool found=false;for(int i=0;i<pattern.count;++i)found|=pattern.notes[static_cast<size_t>(i)].id==*it;if(found)++it;else it=selected.erase(it);}}
        updateScrollbars();
    }
    repaint();
}
void RollCanvas::undo(bool redo) {
    if (processingBaseline) return;
    auto& from = redo ? future : history; auto& to = redo ? history : future;
    if (from.empty()) return; to.push_back(pattern); pattern = from.back(); from.pop_back(); selected.clear(); publish();
}
void RollCanvas::deleteAt(juce::Point<float> point) { int i = hit(point); if (i >= 0) { selected.erase(pattern.notes[static_cast<size_t>(i)].id); pattern.remove(i); publish(); } }
void RollCanvas::addAt(const juce::MouseEvent& e) {
    if(externalClipLimit>0 && beatAt(e.position.x)>=noteLimit())return;
    double beat = std::clamp(snapped(beatAt(e.position.x), e.mods), 0.0, noteLimit() - 1.0 / 960);
    int pitch = pitchAt(e.position.y);
    if (stampIndex >= 0 && stampIndex < static_cast<int>(pr::stamps().size())) {
        if (!pr::placeStamp(pattern, pr::stamps()[static_cast<size_t>(stampIndex)], beat, std::min(lastLength,noteLimit()-beat), pitch, channel, selected)) { status = "Stamp exceeds pitch or note capacity limits."; return; }
        status = "Stamped " + juce::String(pr::stamps()[static_cast<size_t>(stampIndex)].name);
        if (stampOnlyOnce) stampIndex = -1;
        if (onStampChanged) onStampChanged(); publish(); return;
    }
    for (int i = 0; i < pattern.count; ++i) { const auto& n = pattern.notes[static_cast<size_t>(i)]; if (n.pitch == pitch && std::abs(n.start - beat) < 0.001 && n.channel == channel) return; }
    uint32_t id = pattern.nextId;
    if (pattern.add({0, beat, std::min(lastLength, noteLimit() - beat), pitch, 100, channel, false})) { selected.clear(); selected.insert(id); publish(); }
    else status = "Pattern is full (2048 notes).";
}
void RollCanvas::velocityAt(juce::Point<float> point) {
    double beat = beatAt(point.x); int closest = -1; double distance = 8.0 / pxBeat;
    for (int i = 0; i < pattern.count; ++i) {
        double d = std::abs(pattern.notes[static_cast<size_t>(i)].start - beat);
        if (d <= distance) { closest = i; distance = d; }
    }
    if (closest < 0) return;
    auto id = pattern.notes[static_cast<size_t>(closest)].id;
    int value = std::clamp(static_cast<int>((getHeight() - 19 - point.y) / (getHeight() - laneTop() - 30) * 127), 1, 127);
    for (int i = 0; i < pattern.count; ++i) {
        auto& n = pattern.notes[static_cast<size_t>(i)];
        if (n.id == id || (selected.count(id) && selected.count(n.id))) n.velocity = value;
    }
    status = "Velocity " + juce::String(value); publish();
}
void RollCanvas::mouseMove(const juce::MouseEvent& e) {
    if (std::abs(e.position.y - laneTop()) <= 5) setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
    else if (e.position.y < overviewHeight && e.position.x >= 68) setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    else if (e.position.y >= overviewHeight && e.position.y < gridTop && e.position.x >= 68) setMouseCursor(juce::MouseCursor::CrosshairCursor);
    else if (e.position.y >= gridTop && e.position.y < laneTop() && e.position.x >= 68) {
        int index = hit(e.position);
        setMouseCursor(index >= 0 && noteRect(pattern.notes[static_cast<size_t>(index)]).getRight() - e.position.x < std::min(7.f, noteRect(pattern.notes[static_cast<size_t>(index)]).getWidth() * 0.3f)
            ? juce::MouseCursor::LeftRightResizeCursor : juce::MouseCursor::NormalCursor);
    } else setMouseCursor(juce::MouseCursor::NormalCursor);
}
void RollCanvas::mouseDown(const juce::MouseEvent& e) {
    grabKeyboardFocus(); down = current = e.position; gesture = none;
    if (e.position.y < overviewHeight && e.position.x >= 68) { gesture = overview; overviewAt(e.position.x); return; }
    if (e.position.y < gridTop) {
        if (e.position.x >= 68) {
            if (e.mods.isRightButtonDown()) { rangeStart = rangeEnd = 0; }
            else { rangeAnchor = std::clamp(snapped(beatAt(e.position.x), e.mods), 0.0, pattern.loop); rangeStart = rangeEnd = rangeAnchor; gesture = timeRange; }
            repaint();
        }
        return;
    }
    if (std::abs(e.position.y - laneTop()) <= 5) { gesture = laneResize; return; }
    if (e.position.y >= getHeight() - 14) return;
    if (e.position.y >= laneTop()) { checkpoint(); gesture = velocity; velocityAt(e.position); return; }
    if (e.position.x < 68) { audition = pitchAt(e.position.y); processor.preview(audition, 100, true); gesture = keys; return; }
    if (e.mods.isRightButtonDown()) { checkpoint(); gesture = erase; deleteAt(e.position); return; }
    int index = hit(e.position);
    if (tool == select || e.mods.isCtrlDown() || e.mods.isCommandDown()) {
        if (!e.mods.isShiftDown()) selected.clear();
        if (index >= 0) { auto id = pattern.notes[static_cast<size_t>(index)].id; if (selected.count(id)) selected.erase(id); else selected.insert(id); }
        else gesture = marquee;
        repaint(); return;
    }
    checkpoint();
    if (index >= 0 && tool == mute) { auto& n = pattern.notes[static_cast<size_t>(index)]; n.muted = !n.muted; publish(); return; }
    if (index >= 0 && tool == slice) {
        const auto n = pattern.notes[static_cast<size_t>(index)]; double cut = snapped(beatAt(e.position.x), e.mods);
        if (cut > n.start && cut < n.start + n.length && pattern.count < pr::capacity) {
            auto right = n; right.start = cut; right.length = n.start + n.length - cut;
            pattern.notes[static_cast<size_t>(index)].length = cut - n.start; pattern.add(right); publish();
        }
        return;
    }
    const bool existingNote = index >= 0;
    if (index < 0) {
        if (tool == mute || tool == slice) return;
        addAt(e); index = hit(e.position);
        if (tool == paintNotes) { gesture = painting; return; }
    }
    if (index < 0) return;
    auto n = pattern.notes[static_cast<size_t>(index)];
    lastLength = n.length;
    if (!selected.count(n.id)) { selected.clear(); selected.insert(n.id); }
    if (existingNote && e.mods.isShiftDown() && pattern.count + static_cast<int>(selected.size()) <= pr::capacity) {
        const auto originalSelection = selected; selected.clear(); int count = pattern.count;
        for (int i = 0; i < count; ++i) if (originalSelection.count(pattern.notes[static_cast<size_t>(i)].id)) {
            auto clone = pattern.notes[static_cast<size_t>(i)]; auto id = pattern.nextId; pattern.add(clone); selected.insert(id);
        }
    }
    before = pattern; dragBeat = beatAt(e.position.x); dragPitch = pitchAt(e.position.y);
    gesture = noteRect(n).getRight() - e.position.x < std::min(7.f, noteRect(n).getWidth() * 0.3f) ? resizeNote : move;
    audition = n.pitch; processor.preview(audition, n.velocity, true); repaint();
}
void RollCanvas::mouseDrag(const juce::MouseEvent& e) {
    current = e.position;
    if (gesture == overview) { overviewAt(e.position.x); return; }
    if (gesture == timeRange) {
        double beat = std::clamp(snapped(beatAt(e.position.x), e.mods), 0.0, pattern.loop);
        rangeStart = std::min(rangeAnchor, beat); rangeEnd = std::max(rangeAnchor, beat); repaint(); return;
    }
    if (gesture == laneResize) {
        velocityHeight = std::clamp(getHeight() - 14 - static_cast<int>(e.position.y), 60, std::max(60, getHeight() - gridTop - 100));
        resized(); repaint(); return;
    }
    if (gesture == keys) {
        int pitch = pitchAt(e.position.y);
        if (pitch != audition) { if (audition >= 0) processor.preview(audition, 0, false); audition = pitch; processor.preview(audition, 100, true); }
        return;
    }
    if (gesture == erase) { deleteAt(e.position); return; }
    if (gesture == painting) { if (e.position.y >= gridTop && e.position.y < laneTop() && e.position.x >= 68) addAt(e); return; }
    if (gesture == velocity) { velocityAt(e.position); return; }
    if (gesture == marquee) { repaint(); return; }
    if (gesture != move && gesture != resizeNote) return;
    double delta = snapped(beatAt(e.position.x) - dragBeat, e.mods);
    int semitones = pitchAt(e.position.y) - dragPitch;
    if (gesture == move) {
        double min = -pattern.loop, max = pattern.loop; int minPitch = -127, maxPitch = 127;
        for (int i = 0; i < before.count; ++i) { const auto& n = before.notes[static_cast<size_t>(i)]; if (!selected.count(n.id)) continue;
            min = std::max(min, -n.start); max = std::min(max, noteLimit() - n.start - n.length);
            minPitch = std::max(minPitch, -n.pitch); maxPitch = std::min(maxPitch, 127 - n.pitch);
        }
        delta = std::clamp(delta, min, max); semitones = std::clamp(semitones, minPitch, maxPitch);
    }
    for (int i = 0; i < pattern.count; ++i) {
        auto& n = pattern.notes[static_cast<size_t>(i)]; const auto& old = before.notes[static_cast<size_t>(i)]; if (!selected.count(n.id)) continue;
        if (gesture == move) { n.start = old.start + delta; n.pitch = old.pitch + semitones; }
        else { n.length = std::clamp(old.length + delta, 1.0 / 960, noteLimit() - old.start); lastLength = n.length; }
    }
    if (gesture == move && audition >= 0 && dragPitch + semitones != audition) {
        processor.preview(audition, 0, false); audition = dragPitch + semitones; processor.preview(audition, 100, true);
    }
    publish();
}
void RollCanvas::mouseUp(const juce::MouseEvent&) {
    if (gesture == marquee) { juce::Rectangle<float> rectangle(down, current); for (int i = 0; i < pattern.count; ++i) { const auto& n = pattern.notes[static_cast<size_t>(i)]; if (rectangle.intersects(noteRect(n))) selected.insert(n.id); } }
    if (audition >= 0) { processor.preview(audition, 0, false); audition = -1; }
    gesture = none; repaint();
}
void RollCanvas::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) {
    if (e.mods.isAltDown()) {
        const int index = e.position.x >= 68 && e.position.y >= gridTop && e.position.y < laneTop() ? hit(e.position) : -1;
        if (index >= 0 && selected.count(pattern.notes[static_cast<size_t>(index)].id)) {
            if (wheel.deltaY == 0) return;
            const bool fine = e.mods.isCtrlDown() || e.mods.isCommandDown();
            const int amount = (wheel.deltaY > 0 ? 1 : -1) * (fine ? 1 : 4);
            bool changed = false;
            for (int i = 0; i < pattern.count; ++i) {
                const auto& n = pattern.notes[static_cast<size_t>(i)];
                if (selected.count(n.id) && std::clamp(n.velocity + amount, 1, 127) != n.velocity) changed = true;
            }
            if (!changed) return;
            checkpoint();
            for (int i = 0; i < pattern.count; ++i) {
                auto& n = pattern.notes[static_cast<size_t>(i)];
                if (selected.count(n.id)) n.velocity = std::clamp(n.velocity + amount, 1, 127);
            }
            status = "Selected velocity " + juce::String(amount > 0 ? "+" : "") + juce::String(amount);
            publish(); return;
        }
        if (wheel.deltaY == 0) return;
        double pitch = topPitch - (e.position.y - gridTop) / row;
        row = std::clamp(static_cast<int>(std::round(row * std::pow(1.5, -wheel.deltaY * 3))), 4, 32);
        topPitch = static_cast<int>(std::round(pitch + (e.position.y - gridTop) / row));
    } else if (e.mods.isCtrlDown() || e.mods.isCommandDown()) {
        double anchor = beatAt(e.position.x); pxBeat = std::clamp(pxBeat * std::pow(1.5, wheel.deltaY * 3), 6.0, 400.0);
        offset = std::clamp(anchor - (e.position.x - 68) / pxBeat, 0.0, pattern.loop - 1);
    } else if (e.mods.isShiftDown() || std::abs(wheel.deltaX) > std::abs(wheel.deltaY)) {
        offset = std::clamp(offset - (wheel.deltaX + wheel.deltaY) * 12, 0.0, std::max(0.0, pattern.loop - (getWidth() - 68) / pxBeat));
    } else topPitch += static_cast<int>(std::round(wheel.deltaY * 24));
    updateScrollbars(); repaint();
}
void RollCanvas::quantize(bool ends) {
    checkpoint(); pr::quantize(pattern, selected, step > 0 ? step : 0.25, ends); publish();
}
void RollCanvas::legato() {
    checkpoint(); for (int i = 0; i < pattern.count; ++i) { auto& n = pattern.notes[static_cast<size_t>(i)]; if (!selected.empty() && !selected.count(n.id)) continue;
        double end = noteLimit();
        for (int j = 0; j < pattern.count; ++j) { const auto& next = pattern.notes[static_cast<size_t>(j)]; if (next.channel == n.channel && next.start > n.start + 0.001) end = std::min(end, next.start); }
        n.length = end - n.start;
    } publish();
}
void RollCanvas::clear() { checkpoint(); pattern.count = 0; selected.clear(); publish(); }
void RollCanvas::fit() {
    pxBeat = std::clamp((getWidth() - 82) / pattern.loop, 6.0, 400.0); offset = 0;
    int high = 72, low = 60; if (pattern.count) { high = 0; low = 127; }
    for (int i = 0; i < pattern.count; ++i) { high = std::max(high, pattern.notes[static_cast<size_t>(i)].pitch); low = std::min(low, pattern.notes[static_cast<size_t>(i)].pitch); }
    topPitch = std::clamp((high + low) / 2 + (laneTop() - gridTop) / row / 2, 24, 127); updateScrollbars(); repaint();
}
void RollCanvas::copy() { clipboard.clear(); for (int i = 0; i < pattern.count; ++i) if (selected.count(pattern.notes[static_cast<size_t>(i)].id)) clipboard.push_back(pattern.notes[static_cast<size_t>(i)]); }
void RollCanvas::paste(bool duplicate) {
    if (duplicate) copy(); if (clipboard.empty()) return;
    if (pattern.count + static_cast<int>(clipboard.size()) > pr::capacity) { status = "Pattern is full."; return; }
    double first = pattern.loop, end = 0;
    for (const auto& n : clipboard) { first = std::min(first, n.start); end = std::max(end, n.start + n.length); }
    double shift = duplicate ? end - first : std::max(0.0, pr::snap(processor.playhead.load(), step)) - first;
    if (end + shift > noteLimit()) { status = "Extend the loop before duplicating past its end."; return; }
    checkpoint(); selected.clear();
    for (auto n : clipboard) { n.start += shift; uint32_t id = pattern.nextId; pattern.add(n); selected.insert(id); } publish();
}
bool RollCanvas::keyPressed(const juce::KeyPress& key) {
    const int code = key.getKeyCode(); bool command = key.getModifiers().isCommandDown() || key.getModifiers().isCtrlDown();
    if (processingBaseline) return true;
    if(processor.externalClipMode.load() && code==juce::KeyPress::spaceKey){if(onExternalTransport)onExternalTransport();return true;}
    if (command && code == 'T') { setTypingEnabled(!typingEnabled); return true; }
    if (typingEnabled && !command && !key.getModifiers().isAltDown()
        && typingState.press(code, typingOctave, channel, typingVelocity,
            [this](int pitch, int velocity, bool on, int ch) { processor.preview(pitch, velocity, on, ch); })) return true;
    if (key.getModifiers().isAltDown() && !command) {
        if (code == 'Q') { openProcessing(ProcessTool::quantize); return true; }
        if (code == 'U') { openProcessing(ProcessTool::chop); return true; }
        if (code == 'S') { openProcessing(ProcessTool::strum); return true; }
        if (code == 'R') { openProcessing(ProcessTool::randomVelocity); return true; }
        if (code == 'X') { openProcessing(ProcessTool::scaleVelocity); return true; }
        if (code == 'O') { openProcessing(ProcessTool::lfoVelocity); return true; }
        if (code == 'M') { checkpoint(); for (int i = 0; i < pattern.count; ++i) if (selected.count(pattern.notes[static_cast<size_t>(i)].id)) pattern.notes[static_cast<size_t>(i)].muted = !key.getModifiers().isShiftDown(); publish(); return true; }
    }
    if (command) {
        if (code == 'Q') { quantize(true); return true; }
        if (code == 'L') { legato(); return true; }
        if (code == 'U') { quickChop(); return true; }
        if (code == 'G') { glueNotes(); return true; }
        if (code == 'D') { selected.clear(); repaint(); return true; }
        if (code == 'Z') { undo(key.getModifiers().isShiftDown()); return true; }
        if (code == 'Y') { undo(true); return true; }
        if (code == 'A') {
            selected.clear(); rangeStart = pattern.loop; rangeEnd = 0;
            for (int i = 0; i < pattern.count; ++i) { const auto& n = pattern.notes[static_cast<size_t>(i)]; selected.insert(n.id); rangeStart = std::min(rangeStart, n.start); rangeEnd = std::max(rangeEnd, n.start + n.length); }
            if (pattern.count == 0) rangeStart = rangeEnd = 0;
            repaint(); return true;
        }
        if (code == 'C') { copy(); return true; }
        if (code == 'V') { paste(false); return true; }
        if (code == 'B') { paste(true); return true; }
        if (code == 'X') { copy(); checkpoint(); for (int i = pattern.count - 1; i >= 0; --i) if (selected.count(pattern.notes[static_cast<size_t>(i)].id)) pattern.remove(i); selected.clear(); publish(); return true; }
    }
    if (code == juce::KeyPress::spaceKey) { if (!processor.hostSync.load()) processor.playing.store(!processor.playing.load()); return true; }
    if (code == juce::KeyPress::escapeKey) { selected.clear(); repaint(); return true; }
    if (code == 'Q' && key.getModifiers().isShiftDown()) { quantize(false); return true; }
    if (!command && !key.getModifiers().isAltDown()) {
        if (code == 'P') { tool = draw; return true; }
        if (code == 'B') { tool = paintNotes; return true; }
        if (code == 'C') { tool = slice; return true; }
        if (code == 'E') { tool = select; return true; }
        if (code == 'T') { tool = mute; return true; }
        if (code == 'N' && key.getModifiers().isShiftDown()) { stampIndex = -1; if (onStampChanged) onStampChanged(); return true; }
    }
    if (code == juce::KeyPress::deleteKey || code == juce::KeyPress::backspaceKey) {
        checkpoint(); for (int i = pattern.count - 1; i >= 0; --i) if (selected.count(pattern.notes[static_cast<size_t>(i)].id)) {
            pattern.remove(i);
        } publish(); return true;
    }
    if (code == juce::KeyPress::upKey || code == juce::KeyPress::downKey || code == juce::KeyPress::leftKey || code == juce::KeyPress::rightKey) {
        checkpoint(); for (int i = 0; i < pattern.count; ++i) { auto& n = pattern.notes[static_cast<size_t>(i)]; if (!selected.count(n.id)) continue;
            if (code == juce::KeyPress::upKey || code == juce::KeyPress::downKey) n.pitch = std::clamp(n.pitch + (code == juce::KeyPress::upKey ? 1 : -1) * (command ? 12 : 1), 0, 127);
            else n.start = std::clamp(n.start + (code == juce::KeyPress::rightKey ? 1 : -1) * (step > 0 ? step : 1.0 / 960), 0.0, noteLimit() - n.length);
        } publish(); return true;
    }
    return false;
}

#include "EditorProcessing.inc"

PianoRollEditor::PianoRollEditor(PianoRollProcessor& p) : AudioProcessorEditor(p), processor(p), roll(p), liveBridge(p) {
    look.setColour(juce::ResizableWindow::backgroundColourId, background);
    look.setColour(juce::TextButton::buttonColourId, panel);
    look.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff547a4b));
    look.setColour(juce::ComboBox::backgroundColourId, panel);
    look.setColour(juce::Slider::thumbColourId, accent);
    look.setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff28232f));
    look.setColour(juce::PopupMenu::textColourId, juce::Colour(0xffded7e5));
    look.setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff55415f));
    look.setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    setLookAndFeel(&look); addAndMakeVisible(roll);
    for (auto* b : {&play, &host, &monitor, &undo, &redo, &stamp, &processing, &fit, &importButton, &exportButton, &save, &load, &help, &typing, &routing}) { addAndMakeVisible(b); b->setWantsKeyboardFocus(false); }
    const std::array<juce::String,15> iconNames {"play","host","monitor","undo","redo","stamp","processing","fit","import","export","save","load","help","typing","routing"};
    const std::array<juce::TextButton*,15> iconButtons {&play,&host,&monitor,&undo,&redo,&stamp,&processing,&fit,&importButton,&exportButton,&save,&load,&help,&typing,&routing};
    for (size_t i = 0; i < iconButtons.size(); ++i) {
        auto& b = *iconButtons[i]; b.getProperties().set("icon",iconNames[i]); b.setName(b.getButtonText()); b.setTooltip(b.getButtonText());
        b.setComponentID("action-" + iconNames[i]);
    }
    const std::array<juce::String,5> toolNames {"draw","paint","select","mute","slice"};
    for (size_t i = 0; i < toolButtons.size(); ++i) {
        auto& b = toolButtons[i]; addAndMakeVisible(b); b.setWantsKeyboardFocus(false);
        b.getProperties().set("icon",toolNames[i]); b.setName(toolNames[i]); b.setTooltip(toolNames[i]); b.setComponentID("tool-"+toolNames[i]);
        b.onClick = [this,i] { roll.tool = static_cast<RollCanvas::Tool>(i); roll.grabKeyboardFocus(); };
    }
    for (auto* b : {&liveReload,&liveApply}) { addChildComponent(b); b->setWantsKeyboardFocus(false); b->setTooltip(b->getButtonText()); b->setName(b->getButtonText()); }
    addChildComponent(liveTarget); liveTarget.setComponentID("live-target"); liveTarget.setFont(juce::Font(juce::FontOptions("Segoe UI",13.f,juce::Font::plain)));
    liveTarget.setColour(juce::Label::textColourId,accent); liveTarget.setInterceptsMouseClicks(false,false);
    liveReload.getProperties().set("icon","import"); liveApply.getProperties().set("icon","export");
    liveReload.setComponentID("live-reload"); liveApply.setComponentID("live-apply");
    liveApply.onClick = [this] { auto result=liveBridge.apply(); if(result.failed()) liveBridge.message=result.getErrorMessage(); refreshControls(); };
    liveReload.onClick = [this] {
        if (liveBridge.waiting()) return;
        juce::Component::SafePointer<PianoRollEditor> safeEditor(this);
        auto reload=[safeEditor](int accepted) { if(safeEditor && accepted) { safeEditor->liveBridge.poll(true); safeEditor->roll.refresh(); safeEditor->roll.fit(); } };
        if (liveBridge.dirty()) juce::AlertWindow::showOkCancelBox(juce::MessageBoxIconType::QuestionIcon,"Reload Live clip","Discard unapplied editor changes and load the selected Live clip?","Reload","Cancel",this,juce::ModalCallbackFunction::create(reload));
        else reload(1);
    };
    toolButtons[0].setToggleState(true, juce::dontSendNotification);
    for (auto* c : {&tools, &snap, &bars, &channels, &scales, &roots, &typingOctave, &typingVelocity, &uiZoom}) { addAndMakeVisible(c); c->setWantsKeyboardFocus(false); }
    typing.setComponentID("typing-piano"); typing.setClickingTogglesState(true);
    typing.onClick = [this] { roll.setTypingEnabled(typing.getToggleState()); roll.grabKeyboardFocus(); };
    roll.onTypingChanged = [this] { typing.setToggleState(roll.typingEnabled, juce::dontSendNotification); };
    typing.setTooltip("Ctrl+T: FL-style full QWERTY piano layout. Letter tool shortcuts are suspended while enabled.");
    for (int oct = -3; oct <= 4; ++oct) typingOctave.addItem("Z: C" + juce::String(4 + oct) + " / Q: C" + juce::String(5 + oct), oct + 4);
    typingOctave.setSelectedId(4); typingOctave.onChange = [this] { roll.releaseTyping(); roll.typingOctave = typingOctave.getSelectedId() - 4; roll.grabKeyboardFocus(); };
    for (int vel : {32, 64, 80, 100, 112, 127}) typingVelocity.addItem("Velocity " + juce::String(vel), vel);
    typingVelocity.setSelectedId(100); typingVelocity.onChange = [this] { roll.typingVelocity = typingVelocity.getSelectedId(); roll.grabKeyboardFocus(); };
    uiZoom.setComponentID("interface-zoom"); uiZoom.addItem("UI 100%", 100); uiZoom.addItem("UI 125%", 125); uiZoom.addItem("UI 150%", 150); uiZoom.setSelectedId(100);
    uiZoom.onChange = [this] { interfaceScale = uiZoom.getSelectedId() / 100.f; resized(); repaint(); };
    addAndMakeVisible(bpm); bpm.setRange(20, 300, 0.1); bpm.setValue(processor.tempo.load(), juce::dontSendNotification);
    bpm.setComponentID("tempo"); bpm.setName("Local tempo");
    bpm.setSliderStyle(juce::Slider::LinearBarVertical); bpm.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 82, 24); bpm.setTextValueSuffix(" BPM");
    bpm.setSliderSnapsToMousePosition(false); bpm.setMouseDragSensitivity(560); bpm.setNumDecimalPlacesToDisplay(1);
    bpm.setTooltip("Local tempo: drag vertically or double-click to type. Host sync follows the DAW tempo.");
    bpm.onValueChange = [this] { processor.tempo.store(bpm.getValue()); };
    play.onClick = [this] { if(liveBridge.active()){auto result=liveBridge.toggleTransport();if(result.failed())liveBridge.message=result.getErrorMessage();}else processor.playing.store(!processor.playing.load());roll.grabKeyboardFocus(); };
    roll.onExternalTransport=[this]{play.onClick();};
    host.setClickingTogglesState(true); host.setToggleState(liveBridge.active()||processor.hostSync.load(), juce::dontSendNotification);
    host.onClick = [this] { processor.hostSync.store(host.getToggleState()); processor.playing.store(false); };
    monitor.setClickingTogglesState(true); monitor.setToggleState(liveBridge.active()?liveBridge.auditionEnabled:processor.monitor.load(), juce::dontSendNotification);
    monitor.onClick = [this] { if(liveBridge.active()){liveBridge.auditionEnabled=monitor.getToggleState();if(!liveBridge.auditionEnabled)liveBridge.panic();}else processor.monitor.store(monitor.getToggleState()); };
    routing.setTooltip("Route MIDI to another synth");
    routing.onClick = [this] {
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon, "Play another instrument",
            "Piano Roll sends MIDI through its plugin output. Use your DAW's MIDI routing to feed an instrument track, and turn off our speaker icon to silence the preview synth.\n\n"
            "Ableton Live:\n1. Put Piano Roll on one MIDI track and your synth on another.\n2. On the synth track, set MIDI From to the Piano Roll track, then choose Piano Roll in the lower chooser.\n3. Set the synth track's Monitor to In.\n4. Enable our chain icon (Host sync), disable our speaker icon, and start Live's transport.\n\n"
            "The typing piano also sends MIDI while our editor has focus. Arm and record the receiving track to capture a Live clip.\n\n"
            "This version edits its own pattern. Existing Live clips do not automatically appear here. Direct clip editing needs a separate Live integration.");
    };
    undo.onClick = [this] { roll.undo(); }; redo.onClick = [this] { roll.undo(true); };
    stamp.onClick = [this] { roll.showStampMenu(&stamp); }; processing.onClick = [this] { roll.showProcessingMenu(&processing); }; fit.onClick = [this] { roll.fit(); };
    roll.onStampChanged = [this] { stamp.setToggleState(roll.stampIndex >= 0, juce::dontSendNotification); stamp.setButtonText(roll.stampIndex >= 0 ? "Stamp *" : "Stamp"); stamp.setTooltip(roll.stampIndex >= 0 ? juce::String(pr::stamps()[static_cast<size_t>(roll.stampIndex)].name) : "Choose a chord or scale stamp"); };
    tools.setVisible(false);
    tools.addItemList({"Draw", "Paint", "Select", "Mute", "Slice"}, 1); tools.setSelectedId(1);
    tools.onChange = [this] { roll.tool = static_cast<RollCanvas::Tool>(tools.getSelectedId() - 1); };
    snap.setComponentID("snap-resolution");
    snap.addItemList({"Snap: none", "Beat (1/4 note)", "1/2 beat (1/8)", "1/4 beat (1/16)", "1/2 step (1/32)", "1/3 beat (triplet)", "1/6 beat (triplet)", "1/4 step (1/64)"}, 1); snap.setSelectedId(4);
    snap.setTooltip("One beat is a quarter note; one step is a sixteenth note.");
    snap.onChange = [this] { const int id = snap.getSelectedId(); if (id >= 1 && id <= 8) roll.step = std::array<double, 8>{0, 1, 0.5, 0.25, 0.125, 1.0 / 3, 1.0 / 6, 0.0625}[static_cast<size_t>(id - 1)]; roll.repaint(); };
    for (int i = 1; i <= 32; ++i) bars.addItem(juce::String(i) + (i == 1 ? " bar" : " bars"), i);
    bars.setSelectedId(static_cast<int>(p.readPattern().loop / 4), juce::dontSendNotification);
    bars.onChange = [this] {
        auto pattern = processor.readPattern(); double length = bars.getSelectedId() * 4.0;
        for (int i = 0; i < pattern.count; ++i) if (pattern.notes[static_cast<size_t>(i)].start + pattern.notes[static_cast<size_t>(i)].length > length + 0.00001) {
            bars.setSelectedId(static_cast<int>(pattern.loop / 4), juce::dontSendNotification); roll.status = "Remove notes beyond the new loop end before shortening."; return;
        }
        pattern.loop = length; processor.writePattern(pattern); roll.refresh();
    };
    for (int i = 1; i <= 16; ++i) channels.addItem("Channel " + juce::String(i), i); channels.setSelectedId(1);
    channels.onChange = [this] { roll.releaseTyping(); roll.channel = channels.getSelectedId(); };
    scales.addItemList({"Scale: off", "Major", "Minor"}, 1); scales.setSelectedId(1);
    scales.onChange = [this] { roll.scale = scales.getSelectedId() - 1; roll.repaint(); };
    roots.addItemList({"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"}, 1); roots.setSelectedId(1);
    roots.onChange = [this] { roll.root = roots.getSelectedId() - 1; roll.repaint(); };
    importButton.onClick = [this] { chooseFile(0); }; exportButton.onClick = [this] { chooseFile(1); };
    save.onClick = [this] { chooseFile(2); }; load.onClick = [this] { chooseFile(3); };
    help.onClick = [this] { juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon, "Piano Roll controls",
        "Draw: click to add, drag notes to move, drag the right edge to resize.\nRight-click/drag: erase. Shift-drag: clone. Alt: bypass snap.\nSelect tool or Ctrl-drag: box select. Ctrl-click: select. Shift: add selection.\nVelocity lane: drag stems; selecting notes links their velocity edits.\nWheel: scroll pitch. Shift-wheel: scroll time. Ctrl-wheel: zoom time. Alt-wheel: zoom pitch; over selected notes: velocity. Ctrl+Alt-wheel: fine velocity.\nOverview/scrollbars navigate. Drag the velocity divider to resize.\nDrag the ruler for a visual time range; right-click the ruler to clear it. The range does not change playback-loop bounds yet.\nCtrl+A/C/X/V: select/copy/cut/paste. Ctrl+B: duplicate after selection.\nCtrl+Z/Y: undo/redo. Arrows: move; Ctrl+Up/Down: octave.\nCtrl+Q: quantize. Shift+Q: quantize starts. Ctrl+L: legato. Ctrl+U: chop. Ctrl+G: glue.\nAlt+M: mute selected. Alt+Shift+M: unmute. P/B/C/E/T: draw/paint/slice/select/mute (typing piano OFF). Ctrl+T: toggle full typing piano.\nStamp chooses a chord/scale preset. Tools opens processing previews with Accept/Cancel.\nDelete: remove. Space: local playback.\nHost sync follows the DAW transport. Preview synth can be muted while MIDI is routed to another instrument.\nThis version supports up to 32 bars in 4/4 and 2048 notes."); };
    setResizable(true, true); setResizeLimits(1080, 700, 3200, 2200); setSize(1280, 860); roll.setReadableRows(); roll.fit(); startTimerHz(30);
}
PianoRollEditor::~PianoRollEditor() { stopTimer(); roll.releaseTyping();liveBridge.panic();processor.externalPreview={};setLookAndFeel(nullptr); }
void PianoRollEditor::paint(juce::Graphics& g) {
    g.fillAll(background);
    g.addTransform(juce::AffineTransform::scale(interfaceScale));
    const int width = static_cast<int>(getWidth() / interfaceScale), height = static_cast<int>(getHeight() / interfaceScale);
    g.setColour(panel.darker(0.25f)); g.fillRect(0, 0, width, toolbarBottom - 8);

    g.setColour(juce::Colour(0xff49414f));
    for (auto* group : {&undo, &toolButtons[0], &stamp, &importButton}) {
        const auto b = group->getBounds(); g.drawVerticalLine(b.getX() - 4, static_cast<float>(b.getY() + 6), static_cast<float>(b.getBottom() - 6));
    }
    g.setColour(juce::Colour(0xffbdc5cc)); g.setFont(juce::FontOptions("Segoe UI", 11.f, juce::Font::plain));
    g.drawText(roll.status.isEmpty() ? "Ctrl+T typing piano   /   Alt+wheel zoom pitch   /   selected note + Alt+wheel velocity" : roll.status,
               16, height - 26, width - 32, 22, juce::Justification::centredLeft);
}
void PianoRollEditor::resized() {
    const int width = static_cast<int>(getWidth() / interfaceScale), height = static_cast<int>(getHeight() / interfaceScale);
    int x = 8, y = 4;
    auto place = [&](juce::Component& c, int w) {
        if (x + w > width - 8) { x = 8; y += 30; }
        c.setTransform(juce::AffineTransform::scale(interfaceScale)); c.setBounds(x, y, w, 26); x += w + 3;
    };
    place(play,26); place(host,26); place(monitor,26); place(typing,26); place(routing,26); x += 6;
    place(undo,26); place(redo,26); x += 6;
    for (auto& b : toolButtons) place(b,26);
    x += 6; place(stamp,26); place(processing,26); place(fit,26); x += 6;
    place(importButton,26); place(exportButton,26); place(save,26); place(load,26); place(help,26);
    x += 6; place(bpm,90); place(snap,146); place(bars,70); place(channels,94);
    x = 8; y += 30;
    if (liveBridge.active()) { place(liveReload,26); place(liveApply,26); x+=6; }
    place(scales,104); place(roots,48); place(typingOctave,132); place(typingVelocity,106); place(uiZoom,96);
    if(liveBridge.active()) place(liveTarget,std::min(300,std::max(100,width-x-8)));
    toolbarBottom = y + 32;
    roll.setTransform(juce::AffineTransform::scale(interfaceScale));
    roll.setViewportBounds({4, toolbarBottom, width - 8, std::max(120, height - toolbarBottom - 25)});
}
juce::Result PianoRollEditor::connectLiveBridge(const juce::File& folder) {
    auto result=liveBridge.connect(folder);
    if(result.wasOk()) {
        liveReload.setVisible(true); liveApply.setVisible(false); liveTarget.setVisible(true); roll.releaseTyping(); roll.setTypingEnabled(false);
        roll.externalClipLimit=liveBridge.loopLength();
        roll.channel=1; roll.refresh(); roll.fit(); resized();
        processor.externalPreview=[this](int pitch,int velocity,bool on,int channel){liveBridge.preview(pitch,velocity,on,channel);};
        typing.setTooltip("Ctrl+T: play the selected Live instrument with the FL-style typing keyboard.");
        play.setTooltip("Play or stop Live (Space)");monitor.setTooltip("Audition notes through the selected Live instrument");
        roll.status=liveBridge.message; refreshControls();
    } else roll.status=result.getErrorMessage();
    return result;
}
void PianoRollEditor::refreshControls() {
    if(liveBridge.active()) {
        liveBridge.updatePlayback();
        if(++bridgePoll%3==0 && !roll.isGestureActive() && !roll.isProcessing()) {
            const auto previous=liveBridge.targetId();const bool changed=liveBridge.poll();
            if(changed){roll.externalClipLimit=liveBridge.loopLength();const bool same=previous==liveBridge.targetId();if(!same)roll.releaseTyping();roll.refresh(same&&liveBridge.lastPollWasAck());if(!same)roll.fit();}
        }
        const auto revision=processor.revision.load();const auto now=juce::Time::getMillisecondCounterHiRes();
        if(revision!=autoRevision){autoRevision=revision;autoChangedAt=now;}
        if(liveBridge.dirty()&&!liveBridge.waiting()&&!liveBridge.autoSyncPaused()&&!roll.isGestureActive()&&!roll.isProcessing()&&now-autoChangedAt>180) {
            auto result=liveBridge.apply();if(result.failed())liveBridge.message=result.getErrorMessage();
        }
        roll.setEnabled(liveBridge.hasClip() && !roll.isProcessing());
        liveApply.setVisible(liveBridge.autoSyncPaused());liveApply.setTooltip("Retry note sync");
        liveApply.setEnabled(liveBridge.dirty()&&!liveBridge.waiting()&&!roll.isProcessing());liveReload.setEnabled(!liveBridge.waiting());
        for(auto* action:{&undo,&redo,&stamp,&processing})action->setEnabled(liveBridge.hasClip()&&!roll.isProcessing());
        for(auto* c:std::initializer_list<juce::Component*>{&host,&bars,&channels,&bpm,&importButton,&load})c->setEnabled(false);
        typing.setEnabled(liveBridge.hasClip());typingOctave.setEnabled(liveBridge.hasClip());typingVelocity.setEnabled(liveBridge.hasClip());monitor.setEnabled(liveBridge.hasClip());
        liveTarget.setText(liveBridge.clipName.isEmpty()?"Select a MIDI track in Live":"Live: "+liveBridge.clipName,juce::dontSendNotification);
        roll.status=liveBridge.message+(liveBridge.dirty()?" / syncing edits":"");
    }

    if (!displayDiagnosticsWritten && getPeer() != nullptr) {
        auto data = std::make_unique<juce::DynamicObject>();
        data->setProperty("version", JucePlugin_VersionString); data->setProperty("peerScale",getPeer()->getPlatformScaleFactor());
        data->setProperty("componentScale",juce::Component::getApproximateScaleFactorForComponent(this));
        data->setProperty("desktopScale",juce::Desktop::getInstance().getGlobalScaleFactor());
        data->setProperty("logicalWidth",getWidth()); data->setProperty("logicalHeight",getHeight());
        const auto file = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("IndependentAudio/PianoRoll/display-diagnostics.json");
        file.getParentDirectory().createDirectory(); file.replaceWithText(juce::JSON::toString(juce::var(data.release())));
        displayDiagnosticsWritten = true;
    }
    roll.pollTyping(); roll.refresh(); play.setButtonText(processor.playing.load() ? "Stop" : "Play"); play.setEnabled(liveBridge.active()?(liveBridge.hasClip()&&liveBridge.connected()):!processor.hostSync.load());
    for (size_t i = 0; i < toolButtons.size(); ++i) toolButtons[i].setToggleState(static_cast<int>(roll.tool) == static_cast<int>(i), juce::dontSendNotification);
    play.setToggleState(processor.playing.load(), juce::dontSendNotification);
    tools.setSelectedId(static_cast<int>(roll.tool) + 1, juce::dontSendNotification);
    host.setToggleState(liveBridge.active()||processor.hostSync.load(), juce::dontSendNotification); monitor.setToggleState(liveBridge.active()?liveBridge.auditionEnabled:processor.monitor.load(), juce::dontSendNotification);
    const bool followsHost = liveBridge.active()||processor.hostSync.load();
    bpm.setEnabled(!followsHost && !liveBridge.active());
    const double hostTempo = processor.reportedHostTempo.load();
    bool tempoEditing = false;
    for (int i = 0; i < bpm.getNumChildComponents(); ++i)
        if (auto* label = dynamic_cast<juce::Label*>(bpm.getChildComponent(i))) tempoEditing |= label->isBeingEdited();
    if (!bpm.isMouseButtonDown() && !tempoEditing)
        bpm.setValue(followsHost && hostTempo > 0 ? hostTempo : processor.tempo.load(), juce::dontSendNotification);
    bars.setSelectedId(static_cast<int>(processor.readPattern().loop / 4), juce::dontSendNotification); repaint();
}
void PianoRollEditor::chooseFile(int action) {
    const bool saving = action == 1 || action == 2, midi = action < 2;
    chooser = std::make_unique<juce::FileChooser>(saving ? "Save" : "Open", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), midi ? "*.mid;*.midi" : "*.pianoroll");
    int chooserFlags = (saving ? juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting : juce::FileBrowserComponent::openMode) | juce::FileBrowserComponent::canSelectFiles;
    juce::Component::SafePointer<PianoRollEditor> safe(this);
    chooser->launchAsync(chooserFlags, [safe, action](const juce::FileChooser& dialog) {
        if (!safe) return; auto file = dialog.getResult(); if (file == juce::File()) return;
        const auto chosenFile = file;
        if (action == 1) file = file.withFileExtension("mid"); if (action == 2) file = file.withFileExtension("pianoroll");
        if (file != chosenFile && file.existsAsFile()) { safe->roll.status = "That filename exists. Select it with its extension to confirm overwrite."; return; }
        auto result = action == 0 ? safe->processor.importMidi(file) : action == 1 ? safe->processor.exportMidi(file) : action == 2 ? safe->processor.savePattern(file) : safe->processor.loadPattern(file);
        safe->roll.status = result.wasOk() ? file.getFileName() : result.getErrorMessage(); safe->roll.refresh(); if (action == 0 || action == 3) safe->roll.fit();
    });
}
