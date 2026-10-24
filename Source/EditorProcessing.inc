void RollCanvas::quickChop() {
    auto candidate = std::make_unique<pr::Pattern>(pattern); auto selection = selected;
    if (!pr::chop(*candidate, selection, step > 0 ? step : 0.25)) { status = "Chop would exceed the 2048-note limit."; return; }
    checkpoint(); pattern = *candidate; selected = std::move(selection); publish();
}
void RollCanvas::glueNotes() { checkpoint(); pr::glue(pattern, selected); publish(); }
bool RollCanvas::beginProcessing() {
    if (processingBaseline) { status = "Finish the open processing tool first."; return false; }
    refresh();
    processingBaseline = std::make_unique<pr::Pattern>(pattern); processingSelection = selected; setEnabled(false); return true;
}
bool RollCanvas::previewProcessing(ProcessTool kind, const ProcessSettings& settings) {
    if (!processingBaseline) return false;
    if (processor.revision.load() != seenRevision) { processingBaseline.reset(); setEnabled(true); refresh(); return false; }
    auto result = std::make_unique<pr::Pattern>(*processingBaseline); auto selection = processingSelection;
    switch (kind) {
        case ProcessTool::quantize: pr::quantize(*result, selection, settings.a, settings.toggle, settings.b / 100); break;
        case ProcessTool::chop:
            if (!pr::chop(*result, selection, settings.a, settings.b / 100)) {
                pattern = *processingBaseline; selected = processingSelection; publish(); status = "Chop exceeds note capacity; reduce the slice count."; return false;
            }
            break;
        case ProcessTool::strum: pr::strum(*result, selection, settings.a, static_cast<int>(settings.b), settings.toggle); break;
        case ProcessTool::randomVelocity: pr::randomizeVelocity(*result, selection, static_cast<int>(settings.a), static_cast<uint32_t>(settings.b)); break;
        case ProcessTool::scaleVelocity: pr::scaleVelocity(*result, selection, settings.a / 100, settings.b); break;
        case ProcessTool::lfoVelocity: pr::lfoVelocity(*result, selection, settings.a, settings.b, settings.c, settings.d, settings.choice); break;
    }
    pattern = *result; selected = std::move(selection); publish(); return true;
}
void RollCanvas::acceptProcessing() {
    if (!processingBaseline) return;
    if (processor.revision.load() == seenRevision) { history.push_back(*processingBaseline); if (history.size() > 64) history.erase(history.begin()); future.clear(); }
    processingBaseline.reset(); setEnabled(true); refresh();
}
void RollCanvas::cancelProcessing() {
    if (!processingBaseline) return;
    const bool ownPreview = processor.revision.load() == seenRevision;
    if (ownPreview) { pattern = *processingBaseline; selected = processingSelection; }
    processingBaseline.reset(); setEnabled(true);
    if (ownPreview) publish(); else refresh();
}

namespace {
class ProcessingPanel final : public juce::Component {
public:
    ProcessingPanel(RollCanvas& roll, RollCanvas::ProcessTool kind) : canvas(&roll), tool(kind) {
        using Tool = RollCanvas::ProcessTool;
        switch (tool) {
            case Tool::quantize: add("Step (beats)", 1.0 / 960, 4, 1.0 / 960, roll.step > 0 ? roll.step : 0.25); add("Strength (%)", 0, 100, 1, 100); toggle.setButtonText("Quantize note ends too"); toggle.setToggleState(true, juce::dontSendNotification); break;
            case Tool::chop: add("Slice (beats)", 1.0 / 16, 4, 1.0 / 16, roll.step > 0 ? roll.step : 0.25); add("Gate (%)", 10, 100, 1, 100); break;
            case Tool::strum: add("Spread (beats)", 0, 0.5, 0.001, 0.08); add("Velocity slope", -64, 64, 1, 0); toggle.setButtonText("High notes first"); break;
            case Tool::randomVelocity: add("Velocity variation", 0, 64, 1, 15); add("Seed", 1, 9999, 1, 1); break;
            case Tool::scaleVelocity: add("Gain (%)", 0, 200, 1, 100); add("Offset", -64, 64, 1, 0); break;
            case Tool::lfoVelocity:
                add("Centre velocity", 1, 127, 1, 90); add("Amplitude", 0, 63, 1, 25); add("Cycles across span", 0.125, 16, 0.125, 2); add("Phase (degrees)", 0, 360, 1, 0);
                shape.addItemList({"Sine", "Triangle", "Saw", "Square"}, 1); shape.setSelectedId(1); addAndMakeVisible(shape); shape.onChange = [this] { update(); }; break;
        }
        if (toggle.getButtonText().isNotEmpty()) { addAndMakeVisible(toggle); toggle.onClick = [this] { update(); }; }
        addAndMakeVisible(description); description.setText("Live preview: selected notes, or all notes when none are selected", juce::dontSendNotification); description.setFont(juce::Font(juce::FontOptions(12.f)));
        addAndMakeVisible(accept); addAndMakeVisible(cancel);
        accept.onClick = [this] { if (canvas) canvas->acceptProcessing(); close(); };
        cancel.onClick = [this] { if (canvas) canvas->cancelProcessing(); close(); };
        setSize(480, 115 + static_cast<int>(sliders.size()) * 46 + (toggle.isVisible() ? 30 : 0) + (shape.isVisible() ? 36 : 0)); update();
    }
    ~ProcessingPanel() override { if (canvas) canvas->cancelProcessing(); }
    void paint(juce::Graphics& g) override { g.fillAll(juce::Colour(0xff242a2d)); }
    void resized() override {
        description.setBounds(14, 8, getWidth() - 28, 32); int y = 46;
        for (size_t i = 0; i < sliders.size(); ++i) { labels[i]->setBounds(14, y, 155, 32); sliders[i]->setBounds(172, y, getWidth() - 186, 32); y += 46; }
        if (toggle.isVisible()) { toggle.setBounds(14, y, getWidth() - 28, 26); y += 30; }
        if (shape.isVisible()) { shape.setBounds(172, y, getWidth() - 186, 28); }
        cancel.setBounds(getWidth() - 200, getHeight() - 42, 86, 28); accept.setBounds(getWidth() - 104, getHeight() - 42, 90, 28);
    }
private:
    juce::Component::SafePointer<RollCanvas> canvas;
    RollCanvas::ProcessTool tool;
    std::vector<std::unique_ptr<juce::Slider>> sliders;
    std::vector<std::unique_ptr<juce::Label>> labels;
    juce::Label description;
    juce::ComboBox shape;
    juce::ToggleButton toggle;
    juce::TextButton accept {"Accept"}, cancel {"Cancel"};
    void add(const juce::String& name, double low, double high, double step, double value) {
        auto slider = std::make_unique<juce::Slider>(); slider->setRange(low, high, step); slider->setValue(value, juce::dontSendNotification);
        slider->setSliderStyle(juce::Slider::LinearHorizontal); slider->setTextBoxStyle(juce::Slider::TextBoxRight, false, 65, 24);
        slider->onValueChange = [this] { update(); }; addAndMakeVisible(*slider); sliders.push_back(std::move(slider));
        auto label = std::make_unique<juce::Label>(); label->setText(name, juce::dontSendNotification); addAndMakeVisible(*label); labels.push_back(std::move(label));
    }
    void update() {
        if (!canvas) return; RollCanvas::ProcessSettings settings;
        double* values[] {&settings.a, &settings.b, &settings.c, &settings.d};
        for (size_t i = 0; i < sliders.size(); ++i) *values[i] = sliders[i]->getValue();
        settings.toggle = toggle.getToggleState(); settings.choice = std::max(0, shape.getSelectedId() - 1);
        bool valid = canvas->previewProcessing(tool, settings); accept.setEnabled(valid);
        description.setText(valid ? "Live preview: selected notes, or all notes when none are selected" : "Preview unavailable: check note capacity or reopen after a score change.", juce::dontSendNotification);
    }
    void close() { if (auto* dialog = findParentComponentOfClass<juce::DialogWindow>()) dialog->exitModalState(0); }
};
}

std::unique_ptr<juce::Component> RollCanvas::createProcessingContent(ProcessTool kind) {
    if (!beginProcessing()) return {};
    return std::make_unique<ProcessingPanel>(*this, kind);
}
void RollCanvas::openProcessing(ProcessTool kind) {
    auto content = createProcessingContent(kind); if (!content) return;
    const std::array<juce::String, 6> titles {"Quantize", "Chop", "Strum", "Randomize velocity", "Scale velocity levels", "Velocity LFO"};
    juce::DialogWindow::LaunchOptions options; options.dialogTitle = titles[static_cast<size_t>(kind)];
    options.content.setOwned(content.release()); options.componentToCentreAround = this;
    options.dialogBackgroundColour = juce::Colour(0xff242a2d); options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true; options.resizable = false; processingWindow = options.launchAsync();
}
void RollCanvas::showProcessingMenu(juce::Component* anchor) {
    if (processingBaseline) { status = "Finish the open processing tool first."; return; }
    juce::PopupMenu menu;
    auto add = [&menu](int id, const char* label, const char* shortcut) { juce::PopupMenu::Item item(label); item.itemID = id; item.shortcutKeyDescription = shortcut; menu.addItem(std::move(item)); };
    add(1, "Quick legato", "Ctrl+L"); add(2, "Quick quantize", "Ctrl+Q"); add(3, "Quantize start times", "Shift+Q");
    add(4, "Quick chop", "Ctrl+U"); add(5, "Glue", "Ctrl+G"); menu.addSeparator();
    add(10, "Quantize...", "Alt+Q"); add(11, "Chop...", "Alt+U"); add(12, "Strum...", "Alt+S");
    add(13, "Randomize velocity...", "Alt+R"); add(14, "Scale velocity levels...", "Alt+X"); add(15, "Velocity LFO...", "Alt+O");
    juce::Component::SafePointer<RollCanvas> safe(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(anchor ? anchor : this).withPreferredPopupDirection(juce::PopupMenu::Options::PopupDirection::downwards), [safe](int result) {
        if (!safe) return;
        if (result == 1) safe->legato(); if (result == 2) safe->quantize(true); if (result == 3) safe->quantize(false);
        if (result == 4) safe->quickChop(); if (result == 5) safe->glueNotes();
        if (result >= 10 && result <= 15) safe->openProcessing(static_cast<ProcessTool>(result - 10));
    });
}
void RollCanvas::showStampMenu(juce::Component* anchor) {
    if (processingBaseline) { status = "Finish the open processing tool first."; return; }
    juce::PopupMenu menu; menu.addItem(1, "None\tShift+N", true, stampIndex < 0); menu.addItem(2, "Only one", true, stampOnlyOnce); menu.addSeparator();
    for (const auto* category : {"Chords", "Advanced chords", "Scales"}) {
        juce::PopupMenu submenu;
        for (size_t i = 0; i < pr::stamps().size(); ++i) if (pr::stamps()[i].category == category)
            submenu.addItem(100 + static_cast<int>(i), juce::String(pr::stamps()[i].name), true, stampIndex == static_cast<int>(i));
        menu.addSubMenu(category, submenu);
    }
    juce::Component::SafePointer<RollCanvas> safe(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(anchor ? anchor : this), [safe](int result) {
        if (!safe || result == 0) return;
        if (result == 1) safe->stampIndex = -1;
        if (result == 2) safe->stampOnlyOnce = !safe->stampOnlyOnce;
        if (result >= 100 && result < 100 + static_cast<int>(pr::stamps().size())) { safe->stampIndex = result - 100; safe->tool = draw; safe->status = "Stamp: " + juce::String(pr::stamps()[static_cast<size_t>(safe->stampIndex)].name); }
        if (safe->onStampChanged) safe->onStampChanged();
    });
}
