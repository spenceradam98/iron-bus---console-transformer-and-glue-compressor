// UI themes. They follow Faderhouse's console colour schemes (and Scope's rack panels), so Iron Bus
// looks at home in Faderhouse's master insert slot.
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

struct Theme
{
    juce::String name;
    juce::Colour rack, rackEdge, section, sectionEdge, text, textDim, accent;
    juce::Colour knobBody, knobPointer;
    juce::Colour capIron, capGlue, capTape, capClip, capGlobal;
    juce::Colour vuFace, vuInk, vuNeedle, vuRed;
    juce::Colour ledLow, ledMid, ledHigh, ledOff, screen;
    bool flatKnobs = false;   // modern flat knobs with value arcs instead of coloured caps
    bool bevel = true;        // raised section panels, rack ears and screws
};

const std::vector<Theme>& themes();

class IronLookAndFeel : public juce::LookAndFeel_V4
{
public:
    IronLookAndFeel();
    void setTheme (const Theme&);
    const Theme& theme() const { return t; }

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool highlighted, bool down) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getTextButtonFont (juce::TextButton&, int h) override;
    juce::Font getPopupMenuFont() override;
    void drawBubble (juce::Graphics&, juce::BubbleComponent&, const juce::Point<float>& tip, const juce::Rectangle<float>& body) override;
    juce::Font getSliderPopupFont (juce::Slider&) override;

private:
    Theme t;
};
