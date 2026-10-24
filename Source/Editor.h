#pragma once
#include "Processor.h"
#include "Stamps.h"
#include "TypingKeyboard.h"
#include "Icons.h"
#include "LiveClipBridge.h"
#include <set>
#include <vector>

class RollCanvas final : public juce::Component, private juce::ScrollBar::Listener {
public:
    explicit RollCanvas(PianoRollProcessor&);
    ~RollCanvas() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseMove(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    bool keyPressed(const juce::KeyPress&) override;
    void refresh();
    bool keyStateChanged(bool) override;
    void focusLost(FocusChangeType) override;
    void setTypingEnabled(bool);
    void releaseTyping();
    void pollTyping();
    bool typingEnabled = false;
    int typingOctave = 0, typingVelocity = 100;
    std::function<void()> onTypingChanged;
    void setViewportBounds(juce::Rectangle<int> bounds) {
        const bool existing = getHeight() > 0;
        const double centre = topPitch - (laneTop() - gridTop) / (2.0 * row);
        setBounds(bounds);
        if (existing) { topPitch = juce::roundToInt(centre + (laneTop() - gridTop) / (2.0 * row)); updateScrollbars(); repaint(); }
    }
    void setReadableRows() { row = 16; resized(); repaint(); }
    void undo(bool redo = false);
    void quantize(bool ends = true);
    void legato();
    void clear();
    void fit();
    void quickChop();
    void glueNotes();
    void showStampMenu(juce::Component* anchor = nullptr);
    void showProcessingMenu(juce::Component* anchor = nullptr);
    enum class ProcessTool { quantize, chop, strum, randomVelocity, scaleVelocity, lfoVelocity };
    struct ProcessSettings { double a = 0, b = 0, c = 0, d = 0; int choice = 0; bool toggle = false; };
    bool isProcessing() const { return processingBaseline != nullptr; }
    bool beginProcessing();
    bool previewProcessing(ProcessTool, const ProcessSettings&);
    void acceptProcessing();
    void cancelProcessing();
    void openProcessing(ProcessTool);
    std::unique_ptr<juce::Component> createProcessingContent(ProcessTool);
    int stampIndex = -1;
    bool stampOnlyOnce = true;
    std::function<void()> onStampChanged;
    enum Tool { draw, paintNotes, select, mute, slice };
    Tool tool = draw;
    double step = 0.25;
    int channel = 1, scale = 0, root = 0;
    juce::String status;
    juce::Range<double> timeSelection() const { return {rangeStart, rangeEnd}; }
    int velocityLaneHeight() const { return velocityHeight; }
    int pitchRowHeight() const { return row; }
private:
    pr::TypingKeyboard typingState;
    PianoRollProcessor& processor;
    pr::Pattern pattern, before;
    std::unique_ptr<pr::Pattern> processingBaseline;
    pr::Selection processingSelection;
    juce::Component::SafePointer<juce::DialogWindow> processingWindow;
    std::vector<pr::Pattern> history, future;
    std::vector<pr::Note> clipboard;
    std::set<uint32_t> selected;
    double pxBeat = 74, offset = 0, lastLength = 1, dragBeat = 0;
    int topPitch = 84, row = 12, dragPitch = 60, audition = -1;
    int velocityHeight = 125;
    static constexpr int overviewHeight = 30, gridTop = 54;
    double rangeStart = 0, rangeEnd = 0, rangeAnchor = 0;
    juce::ScrollBar horizontal {false}, vertical {true};
    uint64_t seenRevision = 0;
    juce::Point<float> down, current;
    enum Gesture { none, move, resizeNote, marquee, erase, velocity, painting, keys, overview, timeRange, laneResize } gesture = none;
    int laneTop() const { return std::max(gridTop + 48, getHeight() - velocityHeight - 14); }
    double beatAt(float x) const { return offset + (x - 68) / pxBeat; }
    int pitchAt(float y) const { return std::clamp(topPitch - static_cast<int>(std::floor((y - gridTop) / row)), 0, 127); }
    juce::Rectangle<float> noteRect(const pr::Note&) const;
    int hit(juce::Point<float>) const;
    void checkpoint();
    void publish();
    void addAt(const juce::MouseEvent&);
    void deleteAt(juce::Point<float>);
    void velocityAt(juce::Point<float>);
    void copy();
    void paste(bool duplicate);
    void updateScrollbars();
    void scrollBarMoved(juce::ScrollBar*, double) override;
    void overviewAt(float x);
    double snapped(double b, const juce::ModifierKeys& mods) const { return mods.isAltDown() ? b : pr::snap(b, step); }
};

class PianoRollLookAndFeel final : public juce::LookAndFeel_V4 {
public:
    juce::Font getTextButtonFont(juce::TextButton&, int) override { return juce::Font(juce::FontOptions("Segoe UI", 13.f, juce::Font::plain)); }
    juce::Font getComboBoxFont(juce::ComboBox&) override { return juce::Font(juce::FontOptions("Segoe UI", 13.f, juce::Font::plain)); }
    void drawButtonBackground(juce::Graphics& g, juce::Button& b, const juce::Colour& colour, bool over, bool down) override {
        if (!b.getProperties().contains("icon")) { juce::LookAndFeel_V4::drawButtonBackground(g,b,colour,over,down); return; }
        if (over || down || b.getToggleState()) {
            g.setColour(b.getToggleState() ? juce::Colour(0xff495344) : juce::Colour(down ? 0xff45414b : 0xff36323e));
            g.fillRoundedRectangle(b.getLocalBounds().toFloat().reduced(1),3.f);
        }
    }
    void drawButtonText(juce::Graphics& g, juce::TextButton& b, bool over, bool down) override {
        const auto icon = b.getProperties()["icon"].toString();
        if (icon.isEmpty()) { juce::LookAndFeel_V4::drawButtonText(g,b,over,down); return; }
        g.setColour((b.getToggleState() ? juce::Colour(0xffb0d982) : juce::Colour(0xffe78399)).withAlpha(b.isEnabled() ? 1.f : 0.35f));
        drawRollIcon(g,b.getLocalBounds().toFloat().reduced(5),icon,b.getToggleState());
    }
    juce::Font getPopupMenuFont() override { return juce::Font(juce::FontOptions("Segoe UI", 13.f, juce::Font::plain)); }
    void positionComboBoxText(juce::ComboBox& box, juce::Label& label) override {
        label.setBounds(7, 0, std::max(0, box.getWidth() - 27), box.getHeight());
        label.setFont(getComboBoxFont(box)); label.setJustificationType(juce::Justification::centredLeft);
    }
    void getIdealPopupMenuItemSize(const juce::String& text, bool separator, int, int& w, int& h) override {
        w = juce::GlyphArrangement::getStringWidthInt(getPopupMenuFont(), text) + 44; h = separator ? 8 : 26;
    }
    juce::Slider::SliderLayout getSliderLayout(juce::Slider& slider) override {
        if (slider.getComponentID() != "tempo") return juce::LookAndFeel_V4::getSliderLayout(slider);
        juce::Slider::SliderLayout result; result.sliderBounds = slider.getLocalBounds(); result.textBoxBounds = slider.getLocalBounds().reduced(5,1).withTrimmedRight(9); return result;
    }
    juce::Label* createSliderTextBox(juce::Slider& slider) override {
        auto* label = juce::LookAndFeel_V4::createSliderTextBox(slider);
        if (slider.getComponentID() == "tempo") {
            label->setFont(juce::Font(juce::FontOptions("Segoe UI", 13.f, juce::Font::plain)));
            label->setColour(juce::Label::outlineColourId, juce::Colours::transparentBlack);
            label->setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
            label->setColour(juce::Label::textColourId, juce::Colour(0xffd5cedd));
        }
        return label;
    }
    void drawLinearSlider(juce::Graphics& g, int x, int y, int w, int h, float value, float min, float max, const juce::Slider::SliderStyle style, juce::Slider& slider) override {
        if (slider.getComponentID() != "tempo") { juce::LookAndFeel_V4::drawLinearSlider(g,x,y,w,h,value,min,max,style,slider); return; }
        auto bounds = slider.getLocalBounds().toFloat().reduced(0.5f);
        g.setColour(juce::Colour(0xff29262e)); g.fillRoundedRectangle(bounds, 3.f);
        g.setColour(juce::Colour(slider.isMouseOverOrDragging() ? 0xff8c7b94 : 0xff49414f)); g.drawRoundedRectangle(bounds,3.f,1.f);
        const float right = static_cast<float>(slider.getWidth() - 9);
        g.setColour(juce::Colour(slider.isEnabled() ? 0xffb8acbf : 0xff736b79));
        juce::Path arrows; arrows.addTriangle(right-2,10,right,7,right+2,10); arrows.addTriangle(right-2,16,right,19,right+2,16); g.fillPath(arrows);
    }
    void drawComboBox(juce::Graphics& g, int w, int h, bool down, int, int, int, int, juce::ComboBox& box) override {
        g.setColour(juce::Colour(down ? 0xff38303e : 0xff29262e)); g.fillRoundedRectangle(0,0,static_cast<float>(w),static_cast<float>(h),3);
        g.setColour(juce::Colour(box.isMouseOverOrDragging() ? 0xff8c7b94 : 0xff49414f)); g.drawRoundedRectangle(0.5f,0.5f,w-1.f,h-1.f,3.f,1.f);
        g.setColour(juce::Colour(0xff928c9a)); juce::Path p; p.startNewSubPath(static_cast<float>(w-13),h*0.45f); p.lineTo(static_cast<float>(w-9),h*0.6f); p.lineTo(static_cast<float>(w-5),h*0.45f); g.strokePath(p,juce::PathStrokeType(1.f));
    }
};

class PianoRollEditor final : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit PianoRollEditor(PianoRollProcessor&);
    ~PianoRollEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    void refreshControls();
    juce::Result connectLiveBridge(const juce::File&);
    bool hasUnappliedClipEdits() const { return liveBridge.dirty(); }
private:
    PianoRollProcessor& processor;
    RollCanvas roll;
    LiveClipBridge liveBridge;
    juce::TextButton liveReload {"Reload selected Live clip"}, liveApply {"Apply edits to Live clip"};
    int bridgePoll = 0;
    juce::Label liveTarget;
    juce::TextButton play {"Play"}, host {"Host sync"}, monitor {"Preview synth"};
    juce::TextButton undo {"Undo"}, redo {"Redo"}, stamp {"Stamp"}, processing {"Tools"}, fit {"Fit"};
    juce::TextButton importButton {"Import MIDI"}, exportButton {"Export MIDI"}, save {"Save"}, load {"Open"}, help {"?"};
    juce::TextButton typing {"Typing piano"}, routing {"MIDI routing"};
    std::array<juce::TextButton, 5> toolButtons;
    juce::TooltipWindow tooltips {this, 500};
    bool displayDiagnosticsWritten = false;
    juce::ComboBox tools, snap, bars, channels, scales, roots, typingOctave, typingVelocity, uiZoom;
    float interfaceScale = 1.f;
    int toolbarBottom = 128;
    juce::Slider bpm;
    std::unique_ptr<juce::FileChooser> chooser;
    PianoRollLookAndFeel look;
    void timerCallback() override { refreshControls(); }
    void chooseFile(int action);
};
