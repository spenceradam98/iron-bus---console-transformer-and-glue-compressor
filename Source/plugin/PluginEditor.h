#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "Themes.h"

// A 1U rack strip. All controls live in a fixed-size content component that is scaled to the window.
class IronBusContent : public juce::Component, private juce::Timer
{
public:
    static constexpr int W = 1440, H = 380;

    explicit IronBusContent (IronBusProcessor&);
    ~IronBusContent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    // the snapshot tool uses these to show a moving meter in still images
    void setMeterPreview (float grDb, float outDb, bool clipping);

private:
    struct Knob
    {
        int param = 0;
        juce::String label;
        juce::Slider slider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach;
    };
    struct Section
    {
        juce::String title;
        int onParam = -1, typeParam = -1, cols = 1;
        float natural = 120;               // natural width before scaling to the strip
        juce::Colour Theme::* cap = &Theme::capGlobal;
        std::vector<Knob*> knobs;
        juce::Rectangle<int> bounds, extra;   // extra: VU or LED ladder area
        juce::ToggleButton power;
        juce::ComboBox type;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> powerAttach;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> typeAttach;
    };

    Section& addSection (const juce::String& title, int onParam, int typeParam, int cols, float natural,
                         juce::Colour Theme::* cap, std::initializer_list<std::pair<int, const char*>> knobs);
    void setupHeaderBox (juce::ComboBox&, int param, std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>&);
    void applyTheme (int index);
    void showPresetMenu();
    void savePresetDialog();
    void timerCallback() override;
    void drawRack (juce::Graphics&);
    void drawVu (juce::Graphics&, juce::Rectangle<float>);
    void drawLadder (juce::Graphics&, juce::Rectangle<float>);

    IronBusProcessor& proc;
    IronLookAndFeel lnf;
    std::vector<std::unique_ptr<Knob>> knobs;
    std::vector<std::unique_ptr<Section>> sections;
    Section* glue = nullptr;
    Section* clip = nullptr;

    juce::TextButton presetButton, prevButton { "<" }, nextButton { ">" }, saveButton { "SAVE" };
    juce::ComboBox themeBox, osBox, orderBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> osAttach, orderAttach;
    float needle = 0, grShown = 0, outMeter = -60, outHold = -60, clipHold = 0;
    int holdFrames = 0;
    bool frozen = false;
    juce::String shownPreset;
    std::unique_ptr<juce::AlertWindow> saveWindow;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (IronBusContent)
};

class IronBusEditor : public juce::AudioProcessorEditor
{
public:
    explicit IronBusEditor (IronBusProcessor&);
    void resized() override;
    void paint (juce::Graphics& g) override { g.fillAll (juce::Colours::black); }
    IronBusContent& getContent() { return content; }

private:
    IronBusProcessor& proc;
    IronBusContent content;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (IronBusEditor)
};
