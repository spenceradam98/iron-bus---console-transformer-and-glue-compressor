// Renders the editor in every theme to PNG files (for the docs), plus a themes.png overview.
// Build with -DIRONBUS_BUILD_SNAPSHOTS=ON; on Linux run under a display (e.g. xvfb-run).
// Usage: ironbus_snapshots <out-dir> [preset-index]
#include "../Source/plugin/PluginProcessor.h"
#include "../Source/plugin/PluginEditor.h"

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    const juce::File dir (argc > 1 ? juce::File::getCurrentWorkingDirectory().getChildFile (argv[1]) : juce::File::getCurrentWorkingDirectory());
    dir.createDirectory();
    IronBusProcessor proc;
    proc.setCurrentProgram (argc > 2 ? juce::String (argv[2]).getIntValue() : 8);
    proc.prepareToPlay (48000, 512);
    juce::AudioBuffer<float> buf (2, 512);
    juce::MidiBuffer midi;
    for (int b = 0; b < 100; ++b)
    {
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < 512; ++i) buf.setSample (c, i, 0.5f * std::sin ((float) (b * 512 + i) * 0.02f));
        proc.processBlock (buf, midi);
    }

    auto write = [&] (const juce::Image& img, const juce::String& name)
    {
        juce::FileOutputStream out (dir.getChildFile (name));
        out.setPosition (0); out.truncate();
        juce::PNGImageFormat().writeImageToStream (img, out);
    };

    std::vector<juce::Image> shots;
    for (int t = 0; t < (int) themes().size(); ++t)
    {
        proc.themeIndex = t;
        proc.uiScale = 1.0f;
        std::unique_ptr<IronBusEditor> ed (dynamic_cast<IronBusEditor*> (proc.createEditor()));
        ed->setSize (IronBusContent::W, IronBusContent::H);
        ed->getContent().setMeterPreview (4.5f, -2.5f, t % 2 == 1);
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
        write (img, juce::String (t) + "_" + juce::String (themes()[(size_t) t].name).removeCharacters (" ") + ".png");
        shots.push_back (img);
    }

    // overview: two columns at half size
    const int w = IronBusContent::W / 2, h = IronBusContent::H / 2, gap = 8;
    const int rows = ((int) shots.size() + 1) / 2;
    juce::Image all (juce::Image::RGB, 2 * w + 3 * gap, rows * h + (rows + 1) * gap, true);
    juce::Graphics g (all);
    g.fillAll (juce::Colour (0xff101010));
    for (size_t i = 0; i < shots.size(); ++i)
        g.drawImage (shots[i], juce::Rectangle<float> ((float) (gap + (int) (i % 2) * (w + gap)), (float) (gap + (int) (i / 2) * (h + gap)), (float) w, (float) h));
    write (all, "themes.png");
    return 0;
}
