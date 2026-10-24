void RollCanvas::paint(juce::Graphics& g) {
    g.fillAll(background);
    const int lane = laneTop(), right = getWidth() - 14, bottom = getHeight() - 14;
    const auto gridClip = juce::Rectangle<int>(68, gridTop, std::max(0, right - 68), std::max(0, lane - gridTop));
    const int rows = (lane - gridTop) / row + 1;
    for (int r = 0; r < rows; ++r) {
        int pitch = topPitch - r; if (pitch < 0) break;
        int y = gridTop + r * row;
        auto shade = black(pitch) ? juce::Colour(0xff222422) : juce::Colour(0xff292b29);
        const int interval = (pitch - root + 120) % 12;
        const bool inScale = scale == 1 ? (interval == 0 || interval == 2 || interval == 4 || interval == 5 || interval == 7 || interval == 9 || interval == 11)
                                       : (interval == 0 || interval == 2 || interval == 3 || interval == 5 || interval == 7 || interval == 8 || interval == 10);
        if (scale > 0 && inScale) shade = shade.interpolatedWith(accent, 0.055f);
        g.setColour(shade); g.fillRect(68, y, right - 68, std::min(row, lane - y));
        g.setColour(juce::Colour(0xff1d1f1d)); g.drawHorizontalLine(y + row - 1, 68.f, static_cast<float>(right));
        const bool active = pitch == audition || processor.keyActivity[static_cast<size_t>(pitch)].load(std::memory_order_relaxed);
        g.setColour(active ? orange : black(pitch) ? juce::Colour(0xff555b63) : juce::Colour(0xffbfc4cd));
        g.fillRect(0, y, 66, std::min(row - 1, lane - y));
        if (row >= 10 || pitch % 12 == 0) {
            g.setColour(active || !black(pitch) ? juce::Colour(0xff4a4e54) : juce::Colour(0xffc6cbd1));
            g.setFont(juce::FontOptions("Segoe UI", static_cast<float>(std::min(row - 1, 12)), juce::Font::plain));
            g.drawText(juce::MidiMessage::getMidiNoteName(pitch, true, true, 5), 4, y, 59, row, juce::Justification::centredRight);
        }
    }

    g.setColour(panel.darker(0.25f)); g.fillRect(68, 0, right - 68, overviewHeight);
    g.setColour(panel); g.fillRect(0, 0, 66, gridTop);
    g.setColour(colourFor(channel)); g.setFont(12.f); g.drawText("MIDI " + juce::String(channel), 3, 2, 60, overviewHeight, juce::Justification::centred);
    int low = 127, high = 0;
    for (int i = 0; i < pattern.count; ++i) { low = std::min(low, pattern.notes[static_cast<size_t>(i)].pitch); high = std::max(high, pattern.notes[static_cast<size_t>(i)].pitch); }
    for (int i = 0; i < pattern.count; ++i) {
        const auto& n = pattern.notes[static_cast<size_t>(i)];
        float x = 68.f + static_cast<float>(n.start / pattern.loop * (right - 68));
        float y = 3.f + static_cast<float>((high - n.pitch) / static_cast<double>(std::max(1, high - low)) * (overviewHeight - 7));
        g.setColour((selected.count(n.id) ? selectedColour : colourFor(n.channel)).withAlpha(n.muted ? 0.2f : 0.85f));
        g.fillRect(x, y, static_cast<float>(std::max(2.0, n.length / pattern.loop * (right - 68))), 1.5f);
    }
    float viewX = 68.f + static_cast<float>(offset / pattern.loop * (right - 68));
    float viewWidth = static_cast<float>(std::min(1.0, (right - 68) / pxBeat / pattern.loop) * (right - 68));
    g.setColour(juce::Colours::white.withAlpha(0.09f)); g.fillRect(viewX, 0.f, viewWidth, static_cast<float>(overviewHeight));
    g.setColour(juce::Colours::white.withAlpha(0.3f)); g.drawRect(viewX, 0.f, viewWidth, static_cast<float>(overviewHeight), 1.f);

    g.setColour(juce::Colour(0xff171a18)); g.fillRect(68, overviewHeight, right - 68, gridTop - overviewHeight);
    g.setColour(juce::Colour(0xff252725)); g.fillRect(68, lane, right - 68, bottom - lane);
    g.setColour(panel); g.fillRect(0, lane, 66, bottom - lane);
    g.setColour(juce::Colours::lightgrey); g.setFont(12.f); g.drawText("Velocity", 3, lane + 6, 60, 18, juce::Justification::centredLeft);
    double grid = step > 0 ? step : 0.25; while (grid * pxBeat < 9) grid *= 2;
    for (double b = std::floor(offset / grid) * grid; b < offset + (right - 68) / pxBeat; b += grid) {
        int x = static_cast<int>(68 + (b - offset) * pxBeat); if (x < 68) continue;
        const bool bar = std::abs(pr::wrap(b, 4)) < 1.e-6;
        const bool beat = std::abs(b - std::round(b)) < 1.e-6;
        g.setColour(bar ? juce::Colour(0xff454943) : beat ? juce::Colour(0xff343833) : juce::Colour(0xff1e211e));
        g.drawVerticalLine(x, static_cast<float>(gridTop), static_cast<float>(bottom));
    }
    if (rangeEnd > rangeStart) {
        const float x1 = std::max(68.f, static_cast<float>(68 + (rangeStart - offset) * pxBeat));
        const float x2 = std::min(static_cast<float>(right), static_cast<float>(68 + (rangeEnd - offset) * pxBeat));
        if (x2 > x1) {
            g.setColour(selectedColour.withAlpha(0.12f)); g.fillRect(x1, static_cast<float>(gridTop), x2 - x1, static_cast<float>(bottom - gridTop));
            g.setColour(selectedColour.withAlpha(0.5f)); g.fillRect(x1, static_cast<float>(overviewHeight), x2 - x1, static_cast<float>(gridTop - overviewHeight));
        }
    }
    g.saveState(); g.reduceClipRegion(gridClip);
    int end = static_cast<int>(68 + (pattern.loop - offset) * pxBeat);
    g.setColour(juce::Colours::black.withAlpha(0.3f)); g.fillRect(end, gridTop, std::max(0, right - end), lane - gridTop);
    for (int i = 0; i < pattern.count; ++i) {
        const auto& n = pattern.notes[static_cast<size_t>(i)]; const auto rect = noteRect(n);
        auto c = selected.count(n.id) ? selectedColour : colourFor(n.channel); if (n.muted) c = c.withAlpha(0.25f);
        g.setGradientFill(juce::ColourGradient(c.brighter(0.12f), 0, rect.getY(), c.darker(0.07f), 0, rect.getBottom(), false));
        g.fillRect(rect);
        g.setColour(c.darker(0.4f)); g.drawRect(rect, 0.5f);
        if (rect.getWidth() > 24 && row >= 10) {
            g.setColour(background.withAlpha(n.muted ? 0.4f : 0.85f)); g.setFont(juce::FontOptions("Segoe UI", static_cast<float>(std::min(row - 2, 12)), juce::Font::plain));
            g.drawText(juce::MidiMessage::getMidiNoteName(n.pitch, true, true, 5), rect.reduced(3, 0), juce::Justification::centredLeft);
        }
    }
    if (gesture == marquee) {
        juce::Rectangle<float> rect(down, current); g.setColour(selectedColour.withAlpha(0.12f)); g.fillRect(rect); g.setColour(selectedColour); g.drawRect(rect);
    }
    g.restoreState();
    g.saveState(); g.reduceClipRegion(68, lane + 3, right - 68, bottom - lane - 3);
    for (int i = 0; i < pattern.count; ++i) {
        const auto& n = pattern.notes[static_cast<size_t>(i)]; const int x = static_cast<int>(68 + (n.start - offset) * pxBeat);
        const int height = static_cast<int>((bottom - lane - 16) * n.velocity / 127.f);
        auto c = selected.count(n.id) ? selectedColour : colourFor(n.channel); if (n.muted) c = c.withAlpha(0.2f);
        g.setColour(c); g.fillRect(x, bottom - height - 5, 2, height);
        g.fillEllipse(static_cast<float>(x - 2), static_cast<float>(bottom - height - 8), 6.f, 6.f);
        g.setColour(c.brighter(0.5f)); g.drawEllipse(static_cast<float>(x - 2), static_cast<float>(bottom - height - 8), 6.f, 6.f, 0.6f);
    }
    g.restoreState();
    for (double b = std::ceil(offset / 4) * 4; b < std::min(pattern.loop, offset + (right - 68) / pxBeat); b += 4) {
        int x = static_cast<int>(68 + (b - offset) * pxBeat);
        g.setColour(juce::Colours::lightgrey); g.setFont(12.f); g.drawText(juce::String(static_cast<int>(b / 4) + 1), x + 4, overviewHeight, 40, gridTop - overviewHeight, juce::Justification::centredLeft);
    }
    g.setColour(panel.brighter(0.15f)); g.fillRect(0, lane - 2, right, 4);
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    for (int i = -2; i <= 2; ++i) g.fillEllipse(getWidth() / 2.f + i * 5.f, lane - 1.f, 2.f, 2.f);
    if (processor.transportRunning.load()) {
        const float x = static_cast<float>(68 + (processor.playhead.load() - offset) * pxBeat);
        if (x >= 68 && x < right) { g.setColour(playColour); g.drawLine(x, static_cast<float>(overviewHeight), x, static_cast<float>(bottom), 1.4f); }
    }
}
