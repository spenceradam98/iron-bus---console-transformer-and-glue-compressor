#include "Themes.h"

using namespace juce;

namespace
{
Colour hex (uint32 rgb, float a = 1.0f) { return Colour (0xff000000 | rgb).withAlpha (a); }
}

const std::vector<Theme>& themes()
{
    static const std::vector<Theme> list = []
    {
        std::vector<Theme> v;
        { // Faderhouse "Studio Grey": classic large-format console grey with coloured caps
            Theme t; t.name = "Studio Grey";
            t.rack = hex (0x8d969e); t.rackEdge = hex (0x5d656c); t.section = hex (0x9ea7af); t.sectionEdge = hex (0x5d656c);
            t.text = hex (0x15181b); t.textDim = hex (0x3c4248); t.accent = hex (0xd23b2a);
            t.knobBody = hex (0x2a2d30); t.knobPointer = hex (0xf4f4f0);
            t.capIron = hex (0x6b4a2e); t.capGlue = hex (0xf0f0ea); t.capTape = hex (0x2d5fa8); t.capClip = hex (0xc8372b); t.capGlobal = hex (0xe8e2d0);
            t.vuFace = hex (0xf3e6c0); t.vuInk = hex (0x1d1a14); t.vuNeedle = hex (0x161310); t.vuRed = hex (0xc8372b);
            t.ledLow = hex (0x3ee05a); t.ledMid = hex (0xf4d03a); t.ledHigh = hex (0xff3b2a); t.ledOff = hex (0x1a1f22); t.screen = hex (0x10171c);
            v.push_back (t);
        }
        { // Faderhouse "Black Session": black-finish console
            Theme t; t.name = "Black Session";
            t.rack = hex (0x1d1f22); t.rackEdge = hex (0x0c0d0e); t.section = hex (0x26292d); t.sectionEdge = hex (0x0c0d0e);
            t.text = hex (0xe6e6e2); t.textDim = hex (0x8e949a); t.accent = hex (0xff5a3c);
            t.knobBody = hex (0x0f1011); t.knobPointer = hex (0xffffff);
            t.capIron = hex (0x9a7148); t.capGlue = hex (0xe9e9e9); t.capTape = hex (0x3772c8); t.capClip = hex (0xd8402f); t.capGlobal = hex (0xbfc3c7);
            t.vuFace = hex (0xe9dcb4); t.vuInk = hex (0x1a1712); t.vuNeedle = hex (0x111111); t.vuRed = hex (0xd8402f);
            t.ledLow = hex (0x37e05a); t.ledMid = hex (0xffd23a); t.ledHigh = hex (0xff3b2a); t.ledOff = hex (0x121416); t.screen = hex (0x050607);
            v.push_back (t);
        }
        { // Faderhouse "Cream Classic": vintage cream and brown
            Theme t; t.name = "Cream Classic";
            t.rack = hex (0xd9cfb8); t.rackEdge = hex (0x8f8167); t.section = hex (0xe6dcc5); t.sectionEdge = hex (0x8f8167);
            t.text = hex (0x2a2118); t.textDim = hex (0x5f5243); t.accent = hex (0xa8321f);
            t.knobBody = hex (0x3a2a1e); t.knobPointer = hex (0xf6eedb);
            t.capIron = hex (0x5a3f28); t.capGlue = hex (0x6b6b6b); t.capTape = hex (0x3e5f8a); t.capClip = hex (0xb0402e); t.capGlobal = hex (0xb8442c);
            t.vuFace = hex (0xf7ecc8); t.vuInk = hex (0x2a2118); t.vuNeedle = hex (0x1e1812); t.vuRed = hex (0xa8321f);
            t.ledLow = hex (0xffb03a); t.ledMid = hex (0xffd27a); t.ledHigh = hex (0xff5533); t.ledOff = hex (0x2a2118); t.screen = hex (0x1e1812);
            v.push_back (t);
        }
        { // Faderhouse "Midnight": blue broadcast desk, flat knobs
            Theme t; t.name = "Midnight";
            t.rack = hex (0x18223a); t.rackEdge = hex (0x0b1120); t.section = hex (0x1f2b48); t.sectionEdge = hex (0x0b1120);
            t.text = hex (0xdfe8ff); t.textDim = hex (0x8394bd); t.accent = hex (0x6fd3ff);
            t.knobBody = hex (0x0c1222); t.knobPointer = hex (0x9fe6ff);
            t.capIron = hex (0xc49a6c); t.capGlue = hex (0x6fd3ff); t.capTape = hex (0x5aa0ff); t.capClip = hex (0xff6a7a); t.capGlobal = hex (0xaebde0);
            t.vuFace = hex (0x0a1020); t.vuInk = hex (0x8394bd); t.vuNeedle = hex (0x6fd3ff); t.vuRed = hex (0xff5a6a);
            t.ledLow = hex (0x3fe0c5); t.ledMid = hex (0x9fe6ff); t.ledHigh = hex (0xff5a6a); t.ledOff = hex (0x0d1426); t.screen = hex (0x050914);
            t.flatKnobs = true;
            v.push_back (t);
        }
        { // Faderhouse "Daylight": modern flat, light
            Theme t; t.name = "Daylight";
            t.rack = hex (0xeceef0); t.rackEdge = hex (0xd0d4d8); t.section = hex (0xffffff); t.sectionEdge = hex (0xd0d4d8);
            t.text = hex (0x1d2329); t.textDim = hex (0x6b7580); t.accent = hex (0xff6b2c);
            t.knobBody = hex (0xdfe3e7); t.knobPointer = hex (0x1d2329);
            t.capIron = hex (0xad7f58); t.capGlue = hex (0x5a6570); t.capTape = hex (0x3e7bfa); t.capClip = hex (0xe5484d); t.capGlobal = hex (0x8a94a0);
            t.vuFace = hex (0xf7f8f9); t.vuInk = hex (0x1d2329); t.vuNeedle = hex (0xff6b2c); t.vuRed = hex (0xe5484d);
            t.ledLow = hex (0x30a46c); t.ledMid = hex (0xf5a524); t.ledHigh = hex (0xe5484d); t.ledOff = hex (0xd9dde1); t.screen = hex (0x1d2329);
            t.flatKnobs = true; t.bevel = false;
            v.push_back (t);
        }
        { // Scope "LED Rack": black rack panel, amber LEDs, silver knobs
            Theme t; t.name = "LED Rack";
            t.rack = hex (0x141414); t.rackEdge = hex (0x050505); t.section = hex (0x1c1c1c); t.sectionEdge = hex (0x333333);
            t.text = hex (0xffb347); t.textDim = hex (0x9a7a4a); t.accent = hex (0xffb347);
            t.knobBody = hex (0xb8bcc0); t.knobPointer = hex (0x111111);
            t.capIron = hex (0xc9cdd1); t.capGlue = hex (0xc9cdd1); t.capTape = hex (0xc9cdd1); t.capClip = hex (0xc9cdd1); t.capGlobal = hex (0xc9cdd1);
            t.vuFace = hex (0x0b0b0b); t.vuInk = hex (0xffb347); t.vuNeedle = hex (0xffd27a); t.vuRed = hex (0xff4a2a);
            t.ledLow = hex (0xffb347); t.ledMid = hex (0xffd27a); t.ledHigh = hex (0xff4a2a); t.ledOff = hex (0x2a1d0e); t.screen = hex (0x050505);
            v.push_back (t);
        }
        return v;
    }();
    return list;
}

IronLookAndFeel::IronLookAndFeel() { setTheme (themes().front()); }

void IronLookAndFeel::setTheme (const Theme& th)
{
    t = th;
    const bool light = t.section.getPerceivedBrightness() > 0.55f;
    const auto popupBg = light ? t.section.brighter (0.1f) : t.rack.darker (0.4f);
    const auto popupText = light ? t.text : t.text.brighter (0.2f);
    setColour (ComboBox::backgroundColourId, Colours::transparentBlack);
    setColour (ComboBox::textColourId, t.text);
    setColour (ComboBox::outlineColourId, t.sectionEdge);
    setColour (ComboBox::arrowColourId, t.textDim);
    setColour (PopupMenu::backgroundColourId, popupBg);
    setColour (PopupMenu::textColourId, popupText);
    setColour (PopupMenu::headerTextColourId, t.accent);
    setColour (PopupMenu::highlightedBackgroundColourId, t.accent.withAlpha (0.35f));
    setColour (PopupMenu::highlightedTextColourId, light ? Colours::black : Colours::white);
    setColour (TextButton::textColourOffId, t.text);
    setColour (TextButton::textColourOnId, t.text);
    setColour (TextButton::buttonColourId, Colours::transparentBlack);
    setColour (Label::textColourId, t.text);
    setColour (TextEditor::backgroundColourId, popupBg);
    setColour (TextEditor::textColourId, popupText);
    setColour (TextEditor::outlineColourId, t.sectionEdge);
    setColour (AlertWindow::backgroundColourId, popupBg);
    setColour (AlertWindow::textColourId, popupText);
    setColour (TooltipWindow::backgroundColourId, popupBg);
    setColour (TooltipWindow::textColourId, popupText);
}

void IronLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos, float start, float end, Slider& s)
{
    auto b = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (2.0f);
    const float d = jmin (b.getWidth(), b.getHeight());
    const auto c = b.getCentre();
    const float r = d * 0.5f;
    const float angle = start + pos * (end - start);
    const int steps = (int) s.getProperties().getWithDefault ("steps", 0);
    const auto cap = Colour::fromString (s.getProperties().getWithDefault ("cap", t.capGlobal.toString()).toString());
    const bool bipolar = s.getProperties()["bipolar"];

    // scale: one tick per step on stepped knobs (chunky detents), eleven ticks on continuous ones
    const int ticks = steps > 1 ? steps : 11;
    for (int i = 0; i < ticks; ++i)
    {
        const float a = start + (end - start) * (float) i / (float) (ticks - 1);
        const bool major = steps > 1 || i % 5 == 0;
        g.setColour (t.text.withAlpha (major ? 0.85f : 0.45f));
        g.drawLine (Line<float> (c.getPointOnCircumference (r - 0.5f, a), c.getPointOnCircumference (r - (major ? 5.5f : 3.5f), a)), steps > 1 ? 2.0f : 1.2f);
    }

    const float body = d * (steps > 1 ? 0.74f : 0.70f);
    auto knob = Rectangle<float> (body, body).withCentre (c);

    if (t.flatKnobs)
    {
        const float ar = r - 8.0f;
        Path track; track.addCentredArc (c.x, c.y, ar, ar, 0, start, end, true);
        g.setColour (t.textDim.withAlpha (0.3f)); g.strokePath (track, PathStrokeType (3.0f, PathStrokeType::curved, PathStrokeType::rounded));
        const float from = bipolar ? (start + end) * 0.5f : start;
        Path val; val.addCentredArc (c.x, c.y, ar, ar, 0, jmin (from, angle), jmax (from, angle), true);
        g.setColour (cap); g.strokePath (val, PathStrokeType (3.0f, PathStrokeType::curved, PathStrokeType::rounded));
        knob = knob.reduced (body * 0.12f);
        g.setColour (t.knobBody); g.fillEllipse (knob);
        g.setColour (t.sectionEdge); g.drawEllipse (knob, 1.0f);
        g.setColour (t.knobPointer);
        g.drawLine (Line<float> (c.getPointOnCircumference (knob.getWidth() * 0.12f, angle), c.getPointOnCircumference (knob.getWidth() * 0.42f, angle)), 2.4f);
        return;
    }

    // chunky console knob: drop shadow, ribbed skirt, coloured cap, white pointer line
    g.setColour (Colours::black.withAlpha (0.35f)); g.fillEllipse (knob.translated (1.5f, 3.0f));
    g.setGradientFill (ColourGradient (t.knobBody.brighter (0.35f), knob.getX(), knob.getY(), t.knobBody.darker (0.4f), knob.getRight(), knob.getBottom(), false));
    g.fillEllipse (knob);
    const int ribs = steps > 1 ? 12 : 24;
    g.setColour (Colours::black.withAlpha (0.35f));
    for (int i = 0; i < ribs; ++i)
    {
        const float a = angle + MathConstants<float>::twoPi * (float) i / (float) ribs;
        g.drawLine (Line<float> (c.getPointOnCircumference (body * 0.5f - 0.5f, a), c.getPointOnCircumference (body * 0.42f, a)), 1.0f);
    }
    auto top = knob.reduced (body * 0.16f);
    g.setGradientFill (ColourGradient (cap.brighter (0.25f), top.getX(), top.getY(), cap.darker (0.35f), top.getRight(), top.getBottom(), false));
    g.fillEllipse (top);
    g.setColour (Colours::black.withAlpha (0.25f)); g.drawEllipse (top, 1.0f);
    const bool lightCap = cap.getPerceivedBrightness() > 0.6f;
    g.setColour (lightCap ? Colour (0xff15181b) : t.knobPointer);
    g.drawLine (Line<float> (c.getPointOnCircumference (body * 0.08f, angle), c.getPointOnCircumference (body * 0.48f, angle)), steps > 1 ? 3.0f : 2.4f);
}

void IronLookAndFeel::drawToggleButton (Graphics& g, ToggleButton& b, bool highlighted, bool)
{
    // console push switch with an LED
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    const bool on = b.getToggleState();
    g.setColour (t.knobBody.withAlpha (0.9f));
    g.fillRoundedRectangle (r, 3.0f);
    g.setColour (highlighted ? t.accent : t.sectionEdge);
    g.drawRoundedRectangle (r, 3.0f, 1.0f);
    auto led = Rectangle<float> (r.getHeight() * 0.45f, r.getHeight() * 0.45f).withCentre (r.getCentre());
    if (on) { g.setColour (t.ledMid.withAlpha (0.35f)); g.fillEllipse (led.expanded (3.0f)); }
    g.setColour (on ? t.ledMid : t.ledOff.brighter (0.15f));
    g.fillEllipse (led);
}

void IronLookAndFeel::drawButtonBackground (Graphics& g, Button& b, const Colour&, bool highlighted, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (t.section.darker (down ? 0.25f : (highlighted ? 0.12f : 0.05f)));
    g.fillRoundedRectangle (r, 3.0f);
    g.setColour (highlighted ? t.accent : t.sectionEdge);
    g.drawRoundedRectangle (r, 3.0f, 1.0f);
}

void IronLookAndFeel::drawComboBox (Graphics& g, int w, int h, bool, int, int, int, int, ComboBox& box)
{
    auto r = Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
    g.setColour (t.screen);
    g.fillRoundedRectangle (r, 3.0f);
    g.setColour (box.isMouseOver() ? t.accent : t.sectionEdge);
    g.drawRoundedRectangle (r, 3.0f, 1.0f);
    Path arrow;
    const float ax = (float) w - 10.0f, ay = (float) h * 0.5f;
    arrow.addTriangle (ax - 4, ay - 2, ax + 4, ay - 2, ax, ay + 3);
    g.setColour (t.ledMid);
    g.fillPath (arrow);
}

Font IronLookAndFeel::getComboBoxFont (ComboBox&) { return Font (FontOptions (13.0f, Font::bold)); }
Font IronLookAndFeel::getTextButtonFont (TextButton&, int h) { return Font (FontOptions (jmin (14.0f, (float) h * 0.55f), Font::bold)); }
Font IronLookAndFeel::getPopupMenuFont() { return Font (FontOptions (14.0f)); }
Font IronLookAndFeel::getSliderPopupFont (Slider&) { return Font (FontOptions (13.0f, Font::bold)); }

void IronLookAndFeel::drawBubble (Graphics& g, BubbleComponent&, const Point<float>&, const Rectangle<float>& body)
{
    g.setColour (findColour (TooltipWindow::backgroundColourId));
    g.fillRoundedRectangle (body, 4.0f);
    g.setColour (t.accent);
    g.drawRoundedRectangle (body, 4.0f, 1.0f);
}
