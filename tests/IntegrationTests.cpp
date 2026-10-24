#include "Processor.h"
#include "Editor.h"
#include <iostream>
#include <stdexcept>
static void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
struct TestPlayHead final : juce::AudioPlayHead {
    double beat = 0, bpm = 120;
    bool running = true;
    juce::Optional<PositionInfo> getPosition() const override {
        PositionInfo p; p.setPpqPosition(beat); p.setBpm(bpm); p.setIsPlaying(running); return p;
    }
};
static juce::MouseEvent mouse(juce::Component& component, float x, float y, int modifiers, juce::Point<float> down, bool dragged = false) {
    return {juce::Desktop::getInstance().getMainMouseSource(), {x, y}, juce::ModifierKeys(modifiers), 1, 0, 0, 0, 0,
            &component, &component, juce::Time::getCurrentTime(), down, juce::Time::getCurrentTime(), 1, dragged};
}
int main(int argc, char** argv) {
    juce::ScopedJuceInitialiser_GUI initialise;
    if (argc == 2 || argc == 3) {
        try {
            const juce::File folder(juce::String::fromUTF8(argv[1]));
            const double fixtureStep = argc == 3 ? juce::String::fromUTF8(argv[2]).getDoubleValue() : 0.25;
            require(fixtureStep >= 1.0 / 960 && fixtureStep <= 4, "fixture step is out of range");
            const juce::String suffix = argc == 3 ? ".step-" + juce::String(fixtureStep, 6) + ".prototype.mid" : ".prototype.mid";
            for (const auto* name : {"quick-quantize", "quantize-starts", "quick-chop", "glue", "legato"}) {
                auto processor = std::make_unique<PianoRollProcessor>(); processor->prepareToPlay(48000, 256);
                require(processor->importMidi(folder.getChildFile(juce::String(name) + ".input.mid")).wasOk(), "fixture import failed");
                auto canvas = std::make_unique<RollCanvas>(*processor); canvas->setSize(1100, 700);
                canvas->step = fixtureStep;
                canvas->keyPressed(juce::KeyPress('A', juce::ModifierKeys::ctrlModifier, 0));
                const juce::String action(name);
                if (action == "quantize-starts") canvas->keyPressed(juce::KeyPress('Q', juce::ModifierKeys::shiftModifier, 0));
                else canvas->keyPressed(juce::KeyPress(action == "quick-quantize" ? 'Q' : action == "quick-chop" ? 'U' : action == "glue" ? 'G' : 'L', juce::ModifierKeys::ctrlModifier, 0));
                require(processor->exportMidi(folder.getChildFile(action + suffix)).wasOk(), "fixture export failed");
                std::cout << "EXPORTED: " << action << std::endl;
            }
            return 0;
        } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
    }
    const auto temporary = juce::File::getSpecialLocation(juce::File::tempDirectory).getNonexistentChildFile("PianoRoll-test", "", false);
    temporary.createDirectory();
    try {
        auto processorOwner = std::make_unique<PianoRollProcessor>(); auto& p = *processorOwner;
        pr::Pattern pattern; pattern.loop = 4;
        pattern.add({0, 0, 0.25, 60, 100, 2, false}); p.writePattern(pattern);
        p.prepareToPlay(48000, 256); p.playing.store(true);
        juce::AudioBuffer<float> audio(2, 256); juce::MidiBuffer midi;
        int noteOns = 0, noteOffs = 0; float amplitude = 0;
        for (int block = 0; block < 30; ++block) {
            midi.clear(); p.processBlock(audio, midi); amplitude = std::max(amplitude, audio.getMagnitude(0, 256));
            if (block == 0) require(p.keyActivity[60].load() && p.transportRunning.load(), "active key and transport indication");
            for (auto event : midi) {
                auto m = event.getMessage();
                if (m.isNoteOn()) { ++noteOns; require(block == 0 && event.samplePosition == 0 && m.getChannel() == 2, "initial MIDI onset/channel"); }
                if (m.isNoteOff()) { ++noteOffs; require(block * 256 + event.samplePosition == 6000, "sample-accurate duration"); }
            }
        }
        require(noteOns == 1 && noteOffs == 1 && amplitude > 0.001f, "sequencer and preview audio");
        require(!p.keyActivity[60].load(), "key indication releases on note-off");
        p.playing.store(false); midi.clear(); p.processBlock(audio, midi);
        int panic = 0; for (auto e : midi) if (e.getMessage().isAllNotesOff()) ++panic;
        require(panic == 16, "transport stop releases channels");
        p.monitor.store(false); midi.clear(); midi.addEvent(juce::MidiMessage::noteOn(3, 72, static_cast<juce::uint8>(90)), 12);
        p.processBlock(audio, midi); require(audio.getMagnitude(0, 256) == 0 && midi.getNumEvents() == 1, "silent preview retains incoming MIDI");

        p.tempo.store(137); juce::MemoryBlock state; p.getStateInformation(state);
        auto restoredOwner = std::make_unique<PianoRollProcessor>(); auto& restored = *restoredOwner;
        restored.setStateInformation(state.getData(), static_cast<int>(state.getSize()));
        auto restoredPattern = restored.readPattern();
        require(restoredPattern.count == 1 && restoredPattern.notes[0].channel == 2 && restoredPattern.notes[0].length == 0.25 && restoredPattern.loop == 4, "state notes round trip");
        require(restored.tempo.load() == 137 && !restored.monitor.load() && !restored.playing.load(), "state settings round trip");
        auto patternFile = temporary.getChildFile("test.pianoroll");
        require(p.savePattern(patternFile).wasOk() && restored.loadPattern(patternFile).wasOk(), "pattern file round trip");
        auto corrupt = temporary.getChildFile("invalid.pianoroll"); corrupt.replaceWithText("invalid");
        require(restored.loadPattern(corrupt).failed() && restored.readPattern().count == 1, "invalid pattern rejected without mutation");
        auto midiFile = temporary.getChildFile("test.mid");
        require(p.exportMidi(midiFile).wasOk() && restored.importMidi(midiFile).wasOk(), "MIDI file round trip");
        restoredPattern = restored.readPattern();
        require(restoredPattern.count == 1 && restoredPattern.notes[0].pitch == 60 && restoredPattern.notes[0].channel == 2 && std::abs(restoredPattern.notes[0].length - 0.25) < 1.e-8, "MIDI note properties preserved");

        TestPlayHead host; p.setPlayHead(&host); p.hostSync.store(true);
        pattern.notes[0].length = 2; p.writePattern(pattern);
        host.beat = 1; midi.clear(); p.processBlock(audio, midi);
        require(p.reportedHostTempo.load() == 120 && p.tempo.load() == 137, "host tempo is reported without overwriting local tempo");
        bool chased = false; for (auto e : midi) chased |= e.getMessage().isNoteOn(); require(chased, "host seek chases held notes");
        host.running = false; midi.clear(); p.processBlock(audio, midi);
        panic = 0; for (auto e : midi) if (e.getMessage().isAllNotesOff()) ++panic; require(panic == 16, "host stop panic");
        p.setPlayHead(nullptr);
        pattern.count = 0; pattern.loop = 16; pattern.add({0, 0.13, 0.5, 60, 100, 1, false}); p.writePattern(pattern);
        auto canvas = std::make_unique<RollCanvas>(p); canvas->setSize(1100, 700);
        canvas->keyPressed(juce::KeyPress('A', juce::ModifierKeys::ctrlModifier, 0));
        canvas->quantize(); require(p.readPattern().notes[0].start == 0.25, "editor quantize");
        canvas->undo(); require(p.readPattern().notes[0].start == 0.13, "editor undo");
        canvas->undo(true); require(p.readPattern().notes[0].start == 0.25, "editor redo");
        canvas->keyPressed(juce::KeyPress('A', juce::ModifierKeys::ctrlModifier, 0));
        canvas->keyPressed(juce::KeyPress('B', juce::ModifierKeys::ctrlModifier, 0));
        require(p.readPattern().count == 2 && p.readPattern().notes[1].start == 0.75, "editor duplicate span");
        canvas->keyPressed(juce::KeyPress('M', juce::ModifierKeys::altModifier, 0)); require(p.readPattern().notes[1].muted, "editor mute selection");
        const auto noteCountBeforeNavigation = p.readPattern().count;
        canvas->mouseDown(mouse(*canvas, 142, 40, juce::ModifierKeys::leftButtonModifier, {142, 40}));
        canvas->mouseDrag(mouse(*canvas, 290, 40, juce::ModifierKeys::leftButtonModifier, {142, 40}, true));
        canvas->mouseUp(mouse(*canvas, 290, 40, 0, {142, 40}, true));
        require(canvas->timeSelection() == juce::Range<double>(1, 3), "snapped timeline range");
        canvas->keyPressed(juce::KeyPress('M', juce::ModifierKeys::altModifier | juce::ModifierKeys::shiftModifier, 0));
        require(!p.readPattern().notes[1].muted && !p.readPattern().notes[0].muted, "timeline range preserves note selection");
        canvas->mouseDown(mouse(*canvas, 438, 40, juce::ModifierKeys::leftButtonModifier, {438, 40}));
        canvas->mouseDrag(mouse(*canvas, 216, 40, juce::ModifierKeys::leftButtonModifier, {438, 40}, true));
        canvas->mouseUp(mouse(*canvas, 216, 40, 0, {438, 40}, true));
        require(canvas->timeSelection() == juce::Range<double>(2, 5), "backwards timeline drag");
        canvas->mouseDown(mouse(*canvas, 100, 40, juce::ModifierKeys::rightButtonModifier, {100, 40}));
        require(canvas->timeSelection().isEmpty(), "right click clears timeline range");
        const int oldLaneHeight = canvas->velocityLaneHeight();
        const float divider = static_cast<float>(700 - oldLaneHeight - 14);
        canvas->mouseDown(mouse(*canvas, 500, divider, juce::ModifierKeys::leftButtonModifier, {500, divider}));
        canvas->mouseDrag(mouse(*canvas, 500, divider - 50, juce::ModifierKeys::leftButtonModifier, {500, divider}, true));
        canvas->mouseUp(mouse(*canvas, 500, divider - 50, 0, {500, divider}, true));
        require(canvas->velocityLaneHeight() == oldLaneHeight + 50, "velocity divider resizes lane");
        require(p.readPattern().count == noteCountBeforeNavigation, "navigation does not mutate score");
        canvas.reset();
        {
            auto wheelProcessor = std::make_unique<PianoRollProcessor>();
            pr::Pattern score; score.loop = 16;
            score.add({0, 1, 1, 60, 100, 1, false});
            score.add({0, 3, 1, 64, 126, 1, false});
            wheelProcessor->writePattern(score);
            auto wheelCanvas = std::make_unique<RollCanvas>(*wheelProcessor); wheelCanvas->setSize(1100, 700);
            juce::MouseWheelDetails wheel; wheel.deltaY = 0.2f;
            auto turn = [&](float x, float y, int mods = juce::ModifierKeys::altModifier) {
                wheelCanvas->mouseWheelMove(mouse(*wheelCanvas, x, y, mods, {x, y}), wheel);
            };
            turn(150, 348);
            require(wheelCanvas->pitchRowHeight() < 12 && wheelProcessor->readPattern().notes[0].velocity == 100, "Alt-wheel over unselected note zooms without editing");
            // Fresh view gives deterministic note coordinates for the property-edit checks.
            wheelCanvas = std::make_unique<RollCanvas>(*wheelProcessor); wheelCanvas->setSize(1100, 700);
            wheelCanvas->keyPressed(juce::KeyPress('A', juce::ModifierKeys::ctrlModifier, 0));
            turn(150, 348);
            require(wheelCanvas->pitchRowHeight() == 12 && wheelProcessor->readPattern().notes[0].velocity == 104 && wheelProcessor->readPattern().notes[1].velocity == 127, "selected-note wheel edits selection with clamping and no zoom");
            wheelCanvas->undo();
            require(wheelProcessor->readPattern().notes[0].velocity == 100 && wheelProcessor->readPattern().notes[1].velocity == 126, "wheel velocity edit is undoable");
            wheelCanvas->keyPressed(juce::KeyPress('A', juce::ModifierKeys::ctrlModifier, 0));
            turn(150, 348, juce::ModifierKeys::altModifier | juce::ModifierKeys::ctrlModifier);
            require(wheelProcessor->readPattern().notes[0].velocity == 101, "Ctrl-Alt wheel fine velocity edit");
            turn(30, 348);
            require(wheelCanvas->pitchRowHeight() < 12 && wheelProcessor->readPattern().notes[0].velocity == 101, "keyboard wheel zoom works with notes selected");
            const int previousRow = wheelCanvas->pitchRowHeight(); turn(800, 200);
            require(wheelCanvas->pitchRowHeight() < previousRow && wheelProcessor->readPattern().notes[0].start == 1, "empty-grid wheel zoom preserves notes");
            const int smallerRow = wheelCanvas->pitchRowHeight(); wheel.deltaY = -0.2f; turn(800,200);
            require(wheelCanvas->pitchRowHeight() > smallerRow, "Alt-wheel down enlarges pitch rows");
        }
        pattern.count = 0; p.writePattern(pattern); canvas = std::make_unique<RollCanvas>(p); canvas->setSize(1100, 700);
        for (size_t i = 0; i < pr::stamps().size(); ++i) if (pr::stamps()[i].name == "Major" && pr::stamps()[i].category == "Chords") canvas->stampIndex = static_cast<int>(i);
        canvas->mouseDown(mouse(*canvas, 147, 348, juce::ModifierKeys::leftButtonModifier, {147, 348}));
        canvas->mouseUp(mouse(*canvas, 147, 348, 0, {147, 348}));
        require(p.readPattern().count == 3 && p.readPattern().notes[0].pitch == 60 && p.readPattern().notes[2].pitch == 67 && canvas->stampIndex == -1, "one-shot stamp mouse placement");
        canvas->undo(); require(p.readPattern().count == 0, "stamp is one undo step"); canvas->undo(true);
        canvas->keyPressed(juce::KeyPress('A', juce::ModifierKeys::ctrlModifier, 0));
        canvas->keyPressed(juce::KeyPress('U', juce::ModifierKeys::ctrlModifier, 0));
        require(p.readPattern().count == 12, "Ctrl+U quick chop uses snap");
        canvas->keyPressed(juce::KeyPress('G', juce::ModifierKeys::ctrlModifier, 0));
        require(p.readPattern().count == 3, "Ctrl+G glues chopped chord");
        require(canvas->beginProcessing(), "begin tool transaction");
        RollCanvas::ProcessSettings settings; settings.a = 50; settings.b = 0;
        require(canvas->previewProcessing(RollCanvas::ProcessTool::scaleVelocity, settings) && p.readPattern().notes[0].velocity == 50, "live velocity scaling preview");
        settings.a = 25; canvas->previewProcessing(RollCanvas::ProcessTool::scaleVelocity, settings);
        require(p.readPattern().notes[0].velocity == 25, "preview does not compound");
        canvas->cancelProcessing(); require(p.readPattern().notes[0].velocity == 100, "cancel restores original values");
        canvas->beginProcessing(); canvas->previewProcessing(RollCanvas::ProcessTool::scaleVelocity, settings); canvas->acceptProcessing();
        require(p.readPattern().notes[0].velocity == 25, "accept commits preview");
        canvas->undo(); require(p.readPattern().notes[0].velocity == 100, "accepted preview has single undo step");
        canvas->beginProcessing(); auto externalPattern = std::make_unique<pr::Pattern>(p.readPattern()); externalPattern->notes[0].velocity = 77; p.writePattern(*externalPattern);
        canvas->cancelProcessing(); require(p.readPattern().notes[0].velocity == 77, "cancel never overwrites external state changes");
        auto toolContent = canvas->createProcessingContent(RollCanvas::ProcessTool::lfoVelocity);
        require(toolContent != nullptr, "LFO panel opens"); toolContent->setVisible(true);
        juce::Image toolImage(juce::Image::ARGB, toolContent->getWidth(), toolContent->getHeight(), true, juce::SoftwareImageType());
        { juce::Graphics graphics(toolImage); toolContent->paintEntireComponent(graphics, true); }
        auto toolFile = juce::File::getCurrentWorkingDirectory().getChildFile("pianoroll-lfo-preview.png");
        juce::FileOutputStream toolStream(toolFile); toolStream.setPosition(0); toolStream.truncate(); juce::PNGImageFormat toolPng;
        require(toolPng.writeImageToStream(toolImage, toolStream), "LFO panel renders");
        toolContent.reset(); require(p.readPattern().notes[0].velocity == 77, "closing tool panel cancels preview");
        pattern.count = 0; pattern.add({0, 1, 0.0625, 60, 100, 1, false}); p.writePattern(pattern); canvas->refresh();
        canvas->mouseDown(mouse(*canvas, 143, 348, juce::ModifierKeys::leftButtonModifier, {143, 348}));
        canvas->mouseDrag(mouse(*canvas, 161.5f, 348, juce::ModifierKeys::leftButtonModifier, {143, 348}, true));
        canvas->mouseUp(mouse(*canvas, 161.5f, 348, 0, {143, 348}, true));
        require(std::abs(p.readPattern().notes[0].start - 1.25) < 1.e-8 && p.readPattern().notes[0].length == 0.0625, "short notes remain draggable rather than always resizing");
        canvas.reset();
        {
            auto uiProcessor = std::make_unique<PianoRollProcessor>(); pr::Pattern uiPattern; uiPattern.loop = 4;
            uiPattern.add({0,130.0/960,610.0/960,60,80,1,false}); uiPattern.add({0,1070.0/960,390.0/960,64,96,1,false});
            uiPattern.add({0,2010.0/960,730.0/960,67,110,1,false}); uiPattern.add({0,3,1,48,90,1,false}); uiProcessor->writePattern(uiPattern);
            std::unique_ptr<juce::AudioProcessorEditor> uiEditor(uiProcessor->createEditor());
            auto* snapSelector = dynamic_cast<juce::ComboBox*>(uiEditor->findChildWithID("snap-resolution"));
            RollCanvas* uiRoll = nullptr; for (int i = 0; i < uiEditor->getNumChildComponents(); ++i) if (auto* c = dynamic_cast<RollCanvas*>(uiEditor->getChildComponent(i))) uiRoll = c;
            require(snapSelector != nullptr && uiRoll != nullptr, "snap selector and roll are exposed by editor");
            auto* tempoControl = dynamic_cast<juce::Slider*>(uiEditor->findChildWithID("tempo"));
            require(tempoControl != nullptr, "tempo control exposed");
            tempoControl->setValue(143.2, juce::sendNotificationSync);
            require(std::abs(uiProcessor->tempo.load() - 143.2) < 1.e-8, "tempo entry updates local processor tempo");
            uiProcessor->hostSync.store(true); uiProcessor->reportedHostTempo.store(98.5);
            static_cast<PianoRollEditor*>(uiEditor.get())->refreshControls();
            require(!tempoControl->isEnabled() && tempoControl->getValue() == 98.5 && std::abs(uiProcessor->tempo.load() - 143.2) < 1.e-8, "host tempo is shown read-only while local setting is retained");
            uiProcessor->hostSync.store(false); static_cast<PianoRollEditor*>(uiEditor.get())->refreshControls();
            require(tempoControl->isEnabled() && std::abs(tempoControl->getValue() - 143.2) < 1.e-8, "local tempo resumes when host sync disabled");
            auto* routingIcon = dynamic_cast<juce::TextButton*>(uiEditor->findChildWithID("action-routing"));
            require(routingIcon != nullptr && routingIcon->getTooltip().isNotEmpty(), "MIDI routing help is accessible");
            auto* paintIcon = dynamic_cast<juce::TextButton*>(uiEditor->findChildWithID("tool-paint"));
            require(paintIcon != nullptr && paintIcon->getTooltip().isNotEmpty() && paintIcon->getName().isNotEmpty(), "tool icon has tooltip and accessible name");
            paintIcon->onClick(); require(uiRoll->tool == RollCanvas::paintNotes, "paint icon selects paint tool");
            auto* drawIcon = dynamic_cast<juce::TextButton*>(uiEditor->findChildWithID("tool-draw"));
            require(drawIcon != nullptr, "draw icon available"); drawIcon->onClick(); require(uiRoll->tool == RollCanvas::draw, "draw icon restores draw tool");
            auto selectSnap = [&](const juce::String& name) {
                int id = 0; for (int i = 0; i < snapSelector->getNumItems(); ++i) if (snapSelector->getItemText(i).startsWith(name)) id = snapSelector->getItemId(i);
                require(id != 0, ("snap label is available: " + name).toRawUTF8()); snapSelector->setSelectedId(id, juce::sendNotificationSync);
            };
            selectSnap("1/4 step"); uiRoll->keyPressed(juce::KeyPress('A', juce::ModifierKeys::ctrlModifier, 0)); uiRoll->keyPressed(juce::KeyPress('U', juce::ModifierKeys::ctrlModifier, 0));
            require(uiProcessor->readPattern().count == 47, "UI 1/4-step chop matches FL note count");
            uiRoll->undo(); selectSnap("1/4 beat"); uiRoll->keyPressed(juce::KeyPress('A', juce::ModifierKeys::ctrlModifier, 0)); uiRoll->keyPressed(juce::KeyPress('U', juce::ModifierKeys::ctrlModifier, 0));
            require(uiProcessor->readPattern().count == 13, "UI 1/4-beat chop matches FL note count");
            uiRoll->undo(); uiRoll->keyPressed(juce::KeyPress('A', juce::ModifierKeys::ctrlModifier, 0)); uiRoll->keyPressed(juce::KeyPress('Q', juce::ModifierKeys::ctrlModifier, 0));
            require(uiProcessor->readPattern().notes[0].start == 0.25 && uiProcessor->readPattern().notes[0].length == 0.5, "UI Ctrl+Q matches observed FL endpoints");
        }
        {
            auto typedProcessor = std::make_unique<PianoRollProcessor>(); typedProcessor->prepareToPlay(48000, 256);
            auto typedCanvas = std::make_unique<RollCanvas>(*typedProcessor); typedCanvas->setSize(1100, 700); typedCanvas->channel = 7;
            typedCanvas->keyPressed(juce::KeyPress('T', juce::ModifierKeys::ctrlModifier, 0));
            require(typedCanvas->typingEnabled, "Ctrl-T enables typing piano");
            typedCanvas->keyPressed(juce::KeyPress('Z')); typedCanvas->keyPressed(juce::KeyPress('Q'));
            typedCanvas->keyPressed(juce::KeyPress('B'));
            require(typedCanvas->tool == RollCanvas::draw, "typing letters do not switch editor tools");
            juce::AudioBuffer<float> typedAudio(2, 256); juce::MidiBuffer typedMidi;
            typedProcessor->processBlock(typedAudio, typedMidi);
            int ons = 0; bool lower = false, upper = false;
            for (auto event : typedMidi) if (event.getMessage().isNoteOn()) {
                ++ons; require(event.getMessage().getChannel() == 7, "typing MIDI uses selected channel");
                lower |= event.getMessage().getNoteNumber() == 48; upper |= event.getMessage().getNoteNumber() == 60;
            }
            require(ons == 3 && lower && upper, "both FL typing rows generate MIDI");
            typedCanvas->focusLost(juce::Component::focusChangedDirectly); typedMidi.clear(); typedProcessor->processBlock(typedAudio, typedMidi);
            int offs = 0; for (auto event : typedMidi) offs += event.getMessage().isNoteOff() ? 1 : 0;
            require(offs == 3, "focus loss releases every typing note");
            typedCanvas->keyPressed(juce::KeyPress('Z')); typedCanvas->setTypingEnabled(false);
            typedMidi.clear(); typedProcessor->processBlock(typedAudio, typedMidi);
            bool released = false; for (auto event : typedMidi) released |= event.getMessage().isNoteOff();
            require(released, "disabling typing releases held notes");
            typedCanvas->keyPressed(juce::KeyPress('B')); require(typedCanvas->tool == RollCanvas::paintNotes, "letter shortcuts return when typing disabled");
        }
        auto screenshotProcessor = std::make_unique<PianoRollProcessor>();
        std::unique_ptr<juce::AudioProcessorEditor> editor(screenshotProcessor->createEditor());
        editor->setVisible(true);
        juce::Image image(juce::Image::ARGB, editor->getWidth(), editor->getHeight(), true, juce::SoftwareImageType());
        { juce::Graphics graphics(image); editor->paintEntireComponent(graphics, true); }
        require(image.getPixelAt(20, 20).getAlpha() > 0, "editor paints visible content");
        auto previewFile = juce::File::getCurrentWorkingDirectory().getChildFile("pianoroll-preview.png");
        juce::FileOutputStream previewStream(previewFile); juce::PNGImageFormat png;
        previewStream.setPosition(0); previewStream.truncate();
        require(previewStream.openedOk() && png.writeImageToStream(image, previewStream), "editor renders screenshot");
        for (int i = 0; i < editor->getNumChildComponents(); ++i)
            if (auto* editorRoll = dynamic_cast<RollCanvas*>(editor->getChildComponent(i)))
                editorRoll->keyPressed(juce::KeyPress('A', juce::ModifierKeys::ctrlModifier, 0));
        { juce::Graphics selectedGraphics(image); editor->paintEntireComponent(selectedGraphics, true); }
        auto selectedPreviewFile = juce::File::getCurrentWorkingDirectory().getChildFile("pianoroll-selection-preview.png");
        juce::FileOutputStream selectedStream(selectedPreviewFile); selectedStream.setPosition(0); selectedStream.truncate();
        require(selectedStream.openedOk() && png.writeImageToStream(image, selectedStream), "selected editor renders screenshot");
        // Render vector/text content directly at 250% device scale, never enlarge a 1x bitmap.
        for (int zoom : {100, 125, 150}) {
            auto* zoomBox = dynamic_cast<juce::ComboBox*>(editor->findChildWithID("interface-zoom"));
            require(zoomBox != nullptr, "UI scale selector available"); zoomBox->setSelectedId(zoom, juce::sendNotificationSync);
            for (int i = 0; i < editor->getNumChildComponents(); ++i) {
                auto* child = editor->getChildComponent(i);
                require(editor->getLocalBounds().toFloat().expanded(1.f).contains(child->getBoundsInParent().toFloat()), "scaled controls fit in editor");
            }
            const float dpi = 2.5f;
            juce::Image highDpi(juce::Image::ARGB, juce::roundToInt(editor->getWidth() * dpi), juce::roundToInt(editor->getHeight() * dpi), true, juce::SoftwareImageType());
            { juce::Graphics highGraphics(highDpi); highGraphics.addTransform(juce::AffineTransform::scale(dpi)); editor->paintEntireComponent(highGraphics, true); }
            require(highDpi.getPixelAt(50, 50).getAlpha() == 255, "high DPI image contains painted content");
            auto highFile = juce::File::getCurrentWorkingDirectory().getChildFile("pianoroll-250pct-ui-" + juce::String(zoom) + ".png");
            juce::FileOutputStream highStream(highFile); highStream.setPosition(0); highStream.truncate();
            require(png.writeImageToStream(highDpi, highStream), "250 percent DPI preview renders");
        }
        for (int zoom : {100, 125, 150}) {
            auto* zoomBox = dynamic_cast<juce::ComboBox*>(editor->findChildWithID("interface-zoom"));
            zoomBox->setSelectedId(zoom, juce::sendNotificationSync); editor->setSize(1080, 700);
            for (int i = 0; i < editor->getNumChildComponents(); ++i)
                require(editor->getLocalBounds().toFloat().expanded(1.f).contains(editor->getChildComponent(i)->getBoundsInParent().toFloat()), "minimum window fits all UI scales");
        }
        editor.reset();
        // Only files created in the uniquely named test directory are removed.
        patternFile.deleteFile(); corrupt.deleteFile(); midiFile.deleteFile(); temporary.deleteFile();
        std::cout << "All processor integration tests passed\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
