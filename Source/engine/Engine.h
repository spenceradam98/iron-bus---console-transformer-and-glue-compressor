// Iron Bus engine: transformer, glue compressor, tape and clipper in one bus strip.
// Plain C++17, no JUCE, so it can be unit tested.
#pragma once

#include "Dsp.h"
#include "Params.h"
#include <atomic>
#include <string>
#include <utility>
#include <vector>

namespace ib
{
// ---------------------------------------------------------------- Iron: transformer saturation
// The signal is split at ~200 Hz. Lows drive a biased tanh core harder (core flux rises as frequency
// falls), highs drive it lightly. A small "remanence" term feeds back how hard the core was pushed on
// the previous sample (hysteresis-lite). Then a gentle HF roll-off and a first-order LF coupling
// high-pass, which adds the slight low-end phase shift of a real output transformer.
// Small-signal gain is 1 at any drive, so Drive adds colour rather than level.
class Iron
{
public:
    struct Voice { double bias, driveScale, highAmount, hyst, hfCut, lfCut, lowBoost, headroom; };
    static const Voice& voice (int type);

    void prepare (double fs);
    void reset();
    void set (int type, double drive, double lowWeight);
    inline double process (int ch, double x);

private:
    double fs = 48000;
    int type = -1;
    double gLow = 1, gHigh = 1, hyst = 0;
    OnePoleLP split[2], hf[2];
    OnePoleHP lf[2];
    Adaa<BiasedTanh> low[2], high[2];
    double lastErr[2] {};
};

// ---------------------------------------------------------------- Glue: VCA bus compressor
// Faderhouse's mix-bus compressor (soft-knee peak detector in dB, stepped attack/release,
// program-dependent Auto release, parallel mix) plus a 1.5:1 ratio, a sidechain high-pass
// and variable stereo link.
class Glue
{
public:
    struct Settings { double threshDb = -12, ratio = 2, attackMs = 3, releaseSec = -1, scHpfHz = 0, makeupDb = 0, mix = 1, link = 1; };

    void prepare (double fs);
    void reset();
    void set (const Settings&);
    inline void process (double& l, double& r);

    // static gain reduction (dB, <= 0) for a detector level, without the time constants
    static double staticGr (double levelDb, double threshDb, double ratio);

    double deepestGrDb = 0;   // most negative gain reduction since the last read

private:
    double fs = 48000;
    Settings s;
    double atk = 0, fixedRel = 0, autoFast = 0, autoSlow = 0, slowC = 0, makeup = 1;
    double gr[2] {}, slowGr[2] {};
    Biquad sc[2];
    double hpfHz = -1;
};

// ---------------------------------------------------------------- Tape: subtle tape stage
// Tapehead Echo's record path at gentle settings: pre-emphasis, tanh saturation, de-emphasis,
// head bump (peak near 90 Hz) and HF loss (gap/spacing low-pass).
class Tape
{
public:
    void prepare (double fs);
    void reset();
    void set (double sat, double bump, double hfLoss);
    inline double process (int ch, double x);

private:
    double fs = 48000, sat = -1, bump = -1, hfLoss = -1;
    double g = 1;
    Biquad pre[2], de[2], headBump[2], gap[2];
    Adaa<BiasedTanh> shaper[2];
};

// ---------------------------------------------------------------- Clip: soft clipper
class Clipper
{
public:
    void reset();
    void set (double ceilingDb, double softness);
    inline double process (int ch, double x);
    double ceiling = 1;
    double peakOver = 0;   // highest |x| / ceiling seen since the last read

private:
    Adaa<KneeClip> shaper[2];
};

// ---------------------------------------------------------------- Engine
class Engine
{
public:
    Engine();

    void prepare (double sampleRate, int maxBlock);
    void reset();

    // Parameter values in their natural units (as in the ParamDef table).
    void setParam (int index, float value) { if (index >= 0 && index < NumParams) values[(size_t) index] = value; }
    float getParam (int index) const { return values[(size_t) index]; }

    // Safe in place (out may equal in).
    void process (const float* inL, const float* inR, float* outL, float* outR, int numSamples);

    // Latency in samples for the current oversampling setting (0 when Off).
    int latencySamples() const;
    static int latencyFor (int osIndex);

    // Metering for the UI (written on the audio thread, read and reset by the UI).
    std::atomic<float> inPeak { 0 }, outPeak { 0 }, grDb { 0 }, clipDb { 0 };

private:
    void update();
    inline void processSample (double& l, double& r);
    void setOversampling (int index);

    double fs = 48000, fsOs = 48000;
    int maxBlock = 512, osIndex = -1, osFactor = 1;
    std::array<float, NumParams> values {};

    Iron iron;
    Glue glue;
    Tape tape;
    Clipper clip;
    Oversampler2x os1[2], os2[2];   // os1: base <-> 2x, os2: 2x <-> 4x

    Smoothed inGain, outGain, onIron, onGlue, onTape, onClip;
    bool glueFirst = false;
    double lastOut[2][3] {};   // last output samples, for the inter-sample peak estimate
};

// ---------------------------------------------------------------- Presets
struct Preset
{
    std::string name;
    std::string bank;
    std::vector<std::pair<std::string, float>> values;   // overrides on top of the parameter defaults
};

const std::vector<Preset>& factoryPresets();
const std::vector<std::string>& presetBanks();

// Full parameter vector for a preset (defaults + overrides).
std::array<float, NumParams> presetValues (const Preset&);
} // namespace ib
