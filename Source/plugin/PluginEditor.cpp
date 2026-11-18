#include "PluginEditor.h"

using namespace juce;

namespace
{
constexpr int kEar = 30, kHeaderH = 58, kGap = 8, kBottom = 12;
constexpr float kStart = MathConstants<float>::pi * -0.75f, kEnd = MathConstants<float>::pi * 0.75f;

Font titleFont (float size) { return Font (FontOptions (size, Font::bold)).withExtraKerningFactor (0.16f); }

int countChoices (const char* c) { int n = 1; for (; *c; ++c) if (*c == '|') ++n; return n; }
}

// ============================================================================ content
IronBusContent::IronBusContent (IronBusProcessor& p) : proc (p)
{
    setLookAndFeel (&lnf);
    using namespace ib;

    addSection ("Input", -1, -1, 1, 110, &Theme::capGlobal, { { InGain, "Input" }, { Link, "Link" } });
    addSection ("Iron", IronOn, IronType, 2, 210, &Theme::capIron, { { IronDrive, "Drive" }, { IronLow, "Low Weight" } });
    glue = &addSection ("Glue", GlueOn, -1, 4, 520, &Theme::capGlue, { { GlueThresh, "Threshold" }, { GlueRatio, "Ratio" }, { GlueAttack, "Attack" }, { GlueRelease, "Release" },
                                                                       { GlueScHpf, "SC HPF" }, { GlueMakeup, "Make-up" }, { GlueMix, "Mix" } });
    addSection ("Tape", TapeOn, -1, 2, 180, &Theme::capTape, { { TapeSat, "Saturation" }, { TapeBump, "Head Bump" }, { TapeHf, "HF Loss" } });
    clip = &addSection ("Clip", ClipOn, -1, 1, 190, &Theme::capClip, { { ClipCeiling, "Ceiling" }, { ClipSoft, "Softness" } });
    addSection ("Output", -1, -1, 1, 110, &Theme::capGlobal, { { OutGain, "Output" } });

    for (auto& k : knobs)
        if (k->param == InGain || k->param == OutGain) k->slider.getProperties().set ("bipolar", true);

    presetButton.onClick = [this] { showPresetMenu(); };
    prevButton.onClick = [this] { proc.stepPreset (-1); };
    nextButton.onClick = [this] { proc.stepPreset (1); };
    saveButton.onClick = [this] { savePresetDialog(); };
    presetButton.setTooltip ("Browse presets");
    saveButton.setTooltip ("Save the current settings as a user preset");
    for (auto* b : { &presetButton, &prevButton, &nextButton, &saveButton }) addAndMakeVisible (*b);

    setupHeaderBox (osBox, Oversample, osAttach);
    setupHeaderBox (orderBox, Order, orderAttach);
    osBox.setTooltip ("Oversampling for the saturation and clipper. Off adds no latency (best for tracking); 2x and 4x add a little latency, which is reported to the DAW.");
    orderBox.setTooltip ("Saturate then compress (Iron > Glue), or compress then saturate (Glue > Iron)");

    for (int i = 0; i < (int) themes().size(); ++i) themeBox.addItem (themes()[(size_t) i].name, i + 1);
    themeBox.onChange = [this] { applyTheme (themeBox.getSelectedId() - 1); };
    themeBox.setTooltip ("Theme");
    addAndMakeVisible (themeBox);

    setSize (W, H);
    applyTheme (jlimit (0, (int) themes().size() - 1, proc.themeIndex));
    themeBox.setSelectedId (proc.themeIndex + 1, dontSendNotification);
    startTimerHz (30);
}

IronBusContent::~IronBusContent()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void IronBusContent::setupHeaderBox (ComboBox& box, int param, std::unique_ptr<AudioProcessorValueTreeState::ComboBoxAttachment>& att)
{
    const auto& d = ib::paramDefs()[param];
    box.addItemList (StringArray::fromTokens (d.choices, "|", ""), 1);
    box.setJustificationType (Justification::centred);
    addAndMakeVisible (box);
    att = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, d.id, box);
}

IronBusContent::Section& IronBusContent::addSection (const String& title, int onParam, int typeParam, int cols, float natural,
                                                     Colour Theme::* cap, std::initializer_list<std::pair<int, const char*>> list)
{
    auto s = std::make_unique<Section>();
    s->title = title; s->onParam = onParam; s->typeParam = typeParam; s->cols = cols; s->natural = natural; s->cap = cap;
    const auto* defs = ib::paramDefs();

    if (onParam >= 0)
    {
        s->power.setTooltip ("Switch " + title + " in or out");
        addAndMakeVisible (s->power);
        s->powerAttach = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, defs[onParam].id, s->power);
    }
    if (typeParam >= 0)
    {
        s->type.addItemList (StringArray::fromTokens (defs[typeParam].choices, "|", ""), 1);
        s->type.setJustificationType (Justification::centred);
        s->type.setTooltip (defs[typeParam].name);
        addAndMakeVisible (s->type);
        s->typeAttach = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, defs[typeParam].id, s->type);
    }

    for (const auto& kv : list)
    {
        auto k = std::make_unique<Knob>();
        const auto& d = defs[kv.first];
        k->param = kv.first;
        k->label = kv.second;
        k->slider.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
        k->slider.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
        k->slider.setRotaryParameters (MathConstants<float>::twoPi + kStart, MathConstants<float>::twoPi + kEnd, true);
        k->slider.setPopupDisplayEnabled (true, true, this, 1200);
        k->slider.setTooltip (d.name);
        if (d.kind == ib::Kind::Choice) k->slider.getProperties().set ("steps", countChoices (d.choices));
        addAndMakeVisible (k->slider);
        k->attach = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, d.id, k->slider);
        if (auto* prm = proc.apvts.getParameter (d.id))
            k->slider.setDoubleClickReturnValue (true, prm->convertFrom0to1 (prm->getDefaultValue()));
        s->knobs.push_back (k.get());
        knobs.push_back (std::move (k));
    }
    sections.push_back (std::move (s));
    return *sections.back();
}

void IronBusContent::applyTheme (int index)
{
    index = jlimit (0, (int) themes().size() - 1, index);
    proc.themeIndex = index;
    const auto& t = themes()[(size_t) index];
    lnf.setTheme (t);
    for (auto& s : sections)
        for (auto* k : s->knobs) k->slider.getProperties().set ("cap", (t.*(s->cap)).toString());
    for (auto* b : { &osBox, &orderBox, &themeBox }) b->setColour (ComboBox::textColourId, t.ledMid);
    for (auto& s : sections) s->type.setColour (ComboBox::textColourId, t.ledMid);
    sendLookAndFeelChange();
    repaint();
}

// ---------------------------------------------------------------- layout
void IronBusContent::resized()
{
    // header
    const int hy = 14, bh = 30;
    prevButton.setBounds (372, hy, 28, bh);
    presetButton.setBounds (404, hy, 290, bh);
    nextButton.setBounds (698, hy, 28, bh);
    saveButton.setBounds (732, hy, 58, bh);
    osBox.setBounds (904, hy + 2, 74, bh - 4);
    orderBox.setBounds (1030, hy + 2, 132, bh - 4);
    themeBox.setBounds (W - kEar - 14 - 140, hy + 2, 140, bh - 4);

    // one row of sections across the strip, widths scaled to fill it
    float sum = 0;
    for (auto& s : sections) sum += s->natural;
    const float avail = (float) (W - 2 * kEar - 16 - kGap * ((int) sections.size() - 1));
    float x = (float) (kEar + 8);
    const int y = kHeaderH + 4, h = H - y - kBottom;
    for (size_t i = 0; i < sections.size(); ++i)
    {
        auto& s = *sections[i];
        const int w = i + 1 == sections.size() ? (W - kEar - 8 - (int) x) : (int) std::round (s.natural * avail / sum);
        s.bounds = { (int) x, y, w, h };
        x += (float) (w + kGap);

        auto head = s.bounds.withHeight (34).reduced (8, 7);
        auto body = s.bounds.withTrimmedTop (36).reduced (6, 4);
        if (s.onParam >= 0) s.power.setBounds (head.removeFromLeft (30).withSizeKeepingCentre (26, 18));
        if (s.typeParam >= 0) s.type.setBounds (head.removeFromRight (96));
        if (&s == glue) s.extra = body.removeFromLeft (176).reduced (6, 10);
        if (&s == clip) s.extra = body.removeFromRight (60).reduced (4, 6);

        const int rows = ((int) s.knobs.size() + s.cols - 1) / s.cols;
        const int cw = body.getWidth() / s.cols, ch = body.getHeight() / jmax (1, rows);
        const int knobSize = jmin (rows == 1 ? 96 : 78, cw - 6, ch - 30);
        for (size_t k = 0; k < s.knobs.size(); ++k)
        {
            const int c = (int) k % s.cols, r = (int) k / s.cols;
            auto cell = Rectangle<int> (body.getX() + c * cw, body.getY() + r * ch, cw, ch);
            if (rows == 1) cell = cell.withSizeKeepingCentre (cw, jmin (ch, knobSize + 60));
            s.knobs[k]->slider.setBounds (cell.withTrimmedBottom (28).withSizeKeepingCentre (knobSize, knobSize));
        }
    }
}

// ---------------------------------------------------------------- painting
void IronBusContent::drawRack (Graphics& g)
{
    const auto& t = lnf.theme();
    const auto all = getLocalBounds().toFloat();
    g.setGradientFill (ColourGradient (t.rack.brighter (0.06f), 0, 0, t.rack.darker (0.12f), 0, (float) H, false));
    g.fillAll();

    // brushed-metal streaks
    Random rnd (11);
    for (int i = 0; i < 160; ++i)
    {
        const float yy = rnd.nextFloat() * H;
        g.setColour ((rnd.nextBool() ? Colours::white : Colours::black).withAlpha (0.025f));
        g.drawHorizontalLine ((int) yy, 0.0f, (float) W);
    }

    // rack ears with slotted holes
    for (auto ear : { all.withWidth ((float) kEar), all.withLeft ((float) (W - kEar)) })
    {
        g.setColour (t.rack.darker (t.bevel ? 0.25f : 0.08f));
        g.fillRect (ear);
        g.setColour (t.rackEdge);
        g.drawVerticalLine ((int) (ear.getX() < 1 ? ear.getRight() - 1 : ear.getX()), 0.0f, (float) H);
        for (float yy : { 40.0f, (float) H - 40.0f })
        {
            auto slot = Rectangle<float> (12.0f, 20.0f).withCentre ({ ear.getCentreX(), yy });
            g.setColour (Colours::black.withAlpha (0.75f)); g.fillRoundedRectangle (slot, 6.0f);
            if (t.bevel)
            {
                auto screw = Rectangle<float> (13, 13).withCentre (slot.getCentre());
                g.setGradientFill (ColourGradient (Colour (0xffd8dadc), screw.getX(), screw.getY(), Colour (0xff6c7074), screw.getRight(), screw.getBottom(), false));
                g.fillEllipse (screw);
                g.setColour (Colour (0xff3a3d40));
                g.drawLine (screw.getX() + 3, screw.getCentreY(), screw.getRight() - 3, screw.getCentreY(), 1.6f);
                g.drawLine (screw.getCentreX(), screw.getY() + 3, screw.getCentreX(), screw.getBottom() - 3, 1.6f);
            }
        }
    }
}

void IronBusContent::paint (Graphics& g)
{
    const auto& t = lnf.theme();
    drawRack (g);

    // name plate
    g.setColour (t.text);
    g.setFont (titleFont (26.0f));
    g.drawText ("IRON BUS", kEar + 16, 10, 220, 30, Justification::centredLeft);
    g.setColour (t.textDim);
    g.setFont (titleFont (9.5f));
    g.drawText ("CONSOLE TRANSFORMER  /  TAPE GLUE", kEar + 18, 38, 300, 12, Justification::centredLeft);

    // header labels
    g.setColour (t.textDim);
    g.setFont (titleFont (10.0f));
    g.drawText ("OVERSAMPLE", osBox.getX() - 100, osBox.getY(), 96, osBox.getHeight(), Justification::centredRight);
    g.drawText ("ORDER", orderBox.getX() - 56, orderBox.getY(), 52, orderBox.getHeight(), Justification::centredRight);
    const int lat = proc.getLatencySamples();
    g.setFont (Font (FontOptions (10.0f)));
    g.drawText (lat > 0 ? "latency " + String (lat) + " smp" : "zero latency", osBox.getX(), osBox.getBottom() + 2, osBox.getWidth(), 12, Justification::centred);

    for (auto& sp : sections)
    {
        auto& s = *sp;
        const auto r = s.bounds.toFloat();
        const bool on = s.onParam < 0 || s.power.getToggleState();
        if (t.bevel)
        {
            g.setColour (Colours::black.withAlpha (0.25f)); g.fillRoundedRectangle (r.translated (0, 2), 4.0f);
            g.setGradientFill (ColourGradient (t.section.brighter (0.05f), 0, r.getY(), t.section.darker (0.06f), 0, r.getBottom(), false));
        }
        else g.setColour (t.section);
        g.fillRoundedRectangle (r, 4.0f);
        g.setColour (t.sectionEdge);
        g.drawRoundedRectangle (r.reduced (0.5f), 4.0f, 1.0f);

        // section title with a stripe in the section's cap colour
        auto head = s.bounds.withHeight (34).reduced (10, 7);
        if (s.onParam >= 0) head.removeFromLeft (32);
        g.setColour (t.*(s.cap));
        g.fillRect (s.bounds.getX() + 1, s.bounds.getY() + 1, s.bounds.getWidth() - 2, 3);
        g.setColour (on ? t.text : t.textDim.withAlpha (0.6f));
        g.setFont (titleFont (13.0f));
        g.drawText (s.title.toUpperCase(), head, Justification::centredLeft);
        g.setColour (t.sectionEdge.withAlpha (0.6f));
        g.drawHorizontalLine (s.bounds.getY() + 34, r.getX() + 8, r.getRight() - 8);

        for (auto* k : s.knobs)
        {
            const auto sb = k->slider.getBounds();
            g.setColour (on ? t.text : t.textDim.withAlpha (0.6f));
            g.setFont (titleFont (10.5f));
            g.drawText (k->label.toUpperCase(), sb.getX() - 20, sb.getBottom() + 1, sb.getWidth() + 40, 13, Justification::centred);
            g.setColour (t.textDim.withAlpha (on ? 1.0f : 0.5f));
            g.setFont (Font (FontOptions (11.0f)));
            g.drawText (k->slider.getTextFromValue (k->slider.getValue()), sb.getX() - 20, sb.getBottom() + 14, sb.getWidth() + 40, 13, Justification::centred);
        }
    }
    if (glue != nullptr) drawVu (g, glue->extra.toFloat());
    if (clip != nullptr) drawLadder (g, clip->extra.toFloat());
}

void IronBusContent::drawVu (Graphics& g, Rectangle<float> area)
{
    // gain reduction VU: rests at 0 on the right, swings left as the glue compresses
    const auto& t = lnf.theme();
    auto face = area.withHeight (jmin (area.getHeight() - 26, area.getWidth() * 0.78f));
    g.setColour (Colours::black.withAlpha (0.5f)); g.fillRoundedRectangle (face.expanded (3), 5.0f);
    g.setGradientFill (ColourGradient (t.vuFace.brighter (0.1f), 0, face.getY(), t.vuFace.darker (0.12f), 0, face.getBottom(), false));
    g.fillRoundedRectangle (face, 3.0f);

    const auto pivot = Point<float> (face.getCentreX(), face.getBottom() + face.getHeight() * 0.32f);
    const float rad = face.getHeight() * 0.98f;
    const float a0 = -0.82f, a1 = 0.82f;   // left (-20 dB) .. right (0 dB), radians from vertical
    auto angleFor = [&] (float gr) { return a1 - (a1 - a0) * std::pow (jlimit (0.0f, 1.0f, gr / 20.0f), 0.62f); };

    Path arc; arc.addCentredArc (pivot.x, pivot.y, rad * 0.86f, rad * 0.86f, 0, a0, a1, true);
    g.setColour (t.vuInk.withAlpha (0.8f)); g.strokePath (arc, PathStrokeType (1.2f));
    Path red; red.addCentredArc (pivot.x, pivot.y, rad * 0.86f, rad * 0.86f, 0, a0, angleFor (10.0f), true);
    g.setColour (t.vuRed); g.strokePath (red, PathStrokeType (3.0f));
    g.setFont (Font (FontOptions (9.5f, Font::bold)));
    for (float db : { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 6.0f, 8.0f, 10.0f, 15.0f, 20.0f })
    {
        const float a = angleFor (db);
        const int idb = (int) db;
        const bool major = idb == 0 || idb == 2 || idb == 4 || idb == 6 || idb == 10 || idb == 20;
        g.setColour (db >= 10 ? t.vuRed : t.vuInk);
        g.drawLine (Line<float> (pivot.getPointOnCircumference (rad * 0.86f, a), pivot.getPointOnCircumference (rad * (major ? 0.76f : 0.80f), a)), 1.2f);
        if (major)
            g.drawText (String ((int) db), Rectangle<float> (22, 11).withCentre (pivot.getPointOnCircumference (rad * 0.67f, a)), Justification::centred);
    }
    g.setColour (t.vuInk.withAlpha (0.7f));
    g.setFont (titleFont (9.0f));
    g.drawText ("GAIN REDUCTION", face.withTrimmedTop (face.getHeight() * 0.80f).withHeight (12), Justification::centred);

    // needle, clipped to the face
    {
        Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (face.toNearestInt());
        const float a = angleFor (needle);
        g.setColour (t.vuNeedle);
        g.drawLine (Line<float> (pivot.getPointOnCircumference (rad * 0.32f, a), pivot.getPointOnCircumference (rad * 0.93f, a)), 1.8f);
    }
    g.setColour (t.rack.darker (0.3f));
    g.fillRect (face.withTop (face.getBottom() - 10));

    g.setColour (t.ledMid);
    g.setFont (Font (FontOptions (12.0f, Font::bold)));
    auto txt = area.withTop (face.getBottom() + 8).withHeight (16);
    g.setColour (t.screen); g.fillRoundedRectangle (txt.withSizeKeepingCentre (96, 18), 3.0f);
    g.setColour (t.ledMid);
    g.drawText ("GR " + String (grShown, 1) + " dB", txt, Justification::centred);
}

void IronBusContent::drawLadder (Graphics& g, Rectangle<float> area)
{
    // output LED ladder (true-peak style) with a CLIP LED on top
    const auto& t = lnf.theme();
    g.setColour (t.screen);
    g.fillRoundedRectangle (area, 3.0f);
    auto inner = area.reduced (6, 6);
    auto label = inner.removeFromBottom (12);
    g.setColour (t.textDim.brighter (0.3f));
    g.setFont (titleFont (8.5f));
    g.drawText ("dBTP", label, Justification::centred);

    const float steps[] = { -30, -24, -18, -12, -9, -6, -4, -2, -1, 0 };
    const int n = 11;   // ten level LEDs + CLIP
    const float lh = inner.getHeight() / (float) n;
    for (int i = 0; i < n; ++i)
    {
        auto led = Rectangle<float> (inner.getX(), inner.getBottom() - (i + 1) * lh, inner.getWidth(), lh).reduced (1.5f, 1.5f);
        bool lit; Colour col; String txt;
        if (i == n - 1) { lit = clipHold > 0.05f; col = t.ledHigh; txt = "CLIP"; }
        else
        {
            lit = outMeter >= steps[i] || (outHold >= steps[i] && outHold < (i + 1 < n - 1 ? steps[i + 1] : 99.0f));
            col = i >= 9 ? t.ledHigh : (i >= 6 ? t.ledMid : t.ledLow);
            txt = String ((int) steps[i]);
        }
        g.setColour (lit ? col : t.ledOff);
        g.fillRoundedRectangle (led, 2.0f);
        if (lit) { g.setColour (col.withAlpha (0.25f)); g.drawRoundedRectangle (led.expanded (1.0f), 2.5f, 1.5f); }
        g.setColour (lit ? Colours::black.withAlpha (0.7f) : t.textDim.withAlpha (0.8f));
        g.setFont (Font (FontOptions (8.5f, Font::bold)));
        g.drawText (txt, led, Justification::centred);
    }
}

// ---------------------------------------------------------------- presets
void IronBusContent::showPresetMenu()
{
    PopupMenu menu;
    const auto& ps = ib::factoryPresets();
    for (const auto& bank : ib::presetBanks())
    {
        PopupMenu sub;
        for (size_t i = 0; i < ps.size(); ++i)
            if (ps[i].bank == bank) sub.addItem ((int) i + 1, ps[i].name, true, (int) i == proc.getCurrentProgram() && proc.getPresetName() == String (ps[i].name));
        menu.addSubMenu (bank, sub);
    }
    const auto users = proc.userPresets();
    PopupMenu userMenu;
    for (int i = 0; i < users.size(); ++i) userMenu.addItem (10000 + i, users[i].getFileNameWithoutExtension());
    if (users.isEmpty()) userMenu.addItem (-1, "(none saved yet)", false);
    menu.addSubMenu ("User", userMenu);
    menu.addSeparator();
    menu.addItem (20000, "Save current as...");
    menu.addItem (20001, "Show preset folder");

    menu.showMenuAsync (PopupMenu::Options().withTargetComponent (&presetButton).withMinimumWidth (presetButton.getWidth()),
                        [this, users] (int r)
                        {
                            if (r <= 0) return;
                            if (r < 10000) proc.loadFactoryPreset (r - 1);
                            else if (r < 20000) proc.loadUserPreset (users[r - 10000]);
                            else if (r == 20000) savePresetDialog();
                            else if (r == 20001)
                            {
                                IronBusProcessor::userPresetFolder().createDirectory();
                                IronBusProcessor::userPresetFolder().revealToUser();
                            }
                        });
}

void IronBusContent::savePresetDialog()
{
    saveWindow = std::make_unique<AlertWindow> ("Save preset", "Name for this preset:", MessageBoxIconType::NoIcon, this);
    saveWindow->setLookAndFeel (&lnf);
    saveWindow->addTextEditor ("name", proc.getPresetName());
    saveWindow->addButton ("Save", 1, KeyPress (KeyPress::returnKey));
    saveWindow->addButton ("Cancel", 0, KeyPress (KeyPress::escapeKey));
    saveWindow->enterModalState (true, ModalCallbackFunction::create ([this] (int result)
    {
        if (result == 1 && saveWindow != nullptr) proc.saveUserPreset (saveWindow->getTextEditorContents ("name"));
        saveWindow.reset();
    }), false);
}

// ---------------------------------------------------------------- meters and live labels
void IronBusContent::setMeterPreview (float grDb, float outDb, bool clipping)
{
    frozen = true;
    needle = grShown = grDb;
    outMeter = outHold = outDb;
    clipHold = clipping ? 1.0f : 0.0f;
    timerCallback();
    repaint();
}

void IronBusContent::timerCallback()
{
    auto& e = proc.getEngine();
    if (! frozen)
    {
        const float gr = e.grDb.exchange (0.0f);
        const float out = (float) Decibels::gainToDecibels (e.outPeak.exchange (0.0f), -60.0f);
        const float clipDb = e.clipDb.exchange (0.0f);
        needle += (gr - needle) * (gr > needle ? 0.45f : 0.18f);   // VU-ish ballistics
        grShown = grShown * 0.8f + gr * 0.2f;
        outMeter = out > outMeter ? out : outMeter - 1.2f;
        if (out >= outHold) { outHold = out; holdFrames = 45; }
        else if (--holdFrames < 0) outHold = jmax (-60.0f, outHold - 1.5f);
        clipHold = clipDb > 0.05f ? 1.0f : jmax (0.0f, clipHold - 0.06f);
    }

    if (shownPreset != proc.getPresetName())
    {
        shownPreset = proc.getPresetName();
        const int idx = proc.getCurrentProgram();
        const auto& ps = ib::factoryPresets();
        String bank = "User";
        if (isPositiveAndBelow (idx, (int) ps.size()) && String (ps[(size_t) idx].name) == shownPreset) bank = ps[(size_t) idx].bank;
        presetButton.setButtonText (shownPreset + "   /   " + bank);
    }

    for (auto& s : sections)
    {
        const bool on = s->onParam < 0 || s->power.getToggleState();
        for (auto* k : s->knobs) k->slider.setAlpha (on ? 1.0f : 0.5f);
        s->type.setAlpha (on ? 1.0f : 0.5f);
    }
    repaint();
}

// ============================================================================ editor
IronBusEditor::IronBusEditor (IronBusProcessor& p) : AudioProcessorEditor (p), proc (p), content (p)
{
    addAndMakeVisible (content);
    const double ratio = (double) IronBusContent::W / IronBusContent::H;
    setResizable (true, true);
    setResizeLimits ((int) (IronBusContent::W * 0.6), (int) (IronBusContent::H * 0.6), IronBusContent::W * 2, IronBusContent::H * 2);
    getConstrainer()->setFixedAspectRatio (ratio);
    const float s = jlimit (0.6f, 2.0f, proc.uiScale);
    setSize (roundToInt (IronBusContent::W * s), roundToInt (IronBusContent::H * s));
}

void IronBusEditor::resized()
{
    // fit inside the editor even when a host ignores the aspect ratio
    const float s = jmin ((float) getWidth() / (float) IronBusContent::W, (float) getHeight() / (float) IronBusContent::H);
    content.setTransform (AffineTransform::scale (s).translated ((getWidth() - IronBusContent::W * s) * 0.5f, 0.0f));
    content.setBounds (0, 0, IronBusContent::W, IronBusContent::H);
    proc.uiScale = s;
}
