#include "Editor.h"

class ClipEditorApplication final : public juce::JUCEApplication {
public:
    const juce::String getApplicationName() override { return "Piano Roll Clip Editor"; }
    const juce::String getApplicationVersion() override { return JucePlugin_VersionString; }
    bool moreThanOneInstanceAllowed() override { return true; }
    void initialise(const juce::String& arguments) override {
        const auto args=juce::StringArray::fromTokens(arguments,true);
        const int index=args.indexOf("--live-bridge");
        if(index<0 || index+1>=args.size()) { setApplicationReturnValue(1); quit(); return; }
        processor=std::make_unique<PianoRollProcessor>();
        auto editor=std::make_unique<PianoRollEditor>(*processor);
        const auto result=editor->connectLiveBridge(juce::File(args[index+1].unquoted()));
        if(result.failed()) { juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,"Live clip editor",result.getErrorMessage(),"Close",nullptr,juce::ModalCallbackFunction::create([](int){juce::JUCEApplication::quit();})); return; }
        window=std::make_unique<Window>(editor.release());
    }
    void shutdown() override { window.reset(); processor.reset(); }
private:
    class Window final : public juce::DocumentWindow {
    public:
        explicit Window(PianoRollEditor* editor) : DocumentWindow("Piano Roll - Live Clip",juce::Colour(0xff202220),allButtons) {
            setUsingNativeTitleBar(true); setContentOwned(editor,true); setResizable(true,false); centreWithSize(getWidth(),getHeight()); setVisible(true);
        }
        void closeButtonPressed() override {
            if(auto* editor=dynamic_cast<PianoRollEditor*>(getContentComponent()); editor && editor->hasUnappliedClipEdits())
                juce::AlertWindow::showOkCancelBox(juce::MessageBoxIconType::QuestionIcon,"Close clip editor","Discard unapplied clip edits?","Discard","Cancel",this,juce::ModalCallbackFunction::create([](int result){if(result)juce::JUCEApplication::quit();}));
            else juce::JUCEApplication::quit();
        }
    };
    std::unique_ptr<PianoRollProcessor> processor;
    std::unique_ptr<Window> window;
};
START_JUCE_APPLICATION(ClipEditorApplication)
// Shared plugin code supplies the factory; this audio-device-free GUI supplies its own entry point.
JUCE_MAIN_FUNCTION_DEFINITION
