#include "Engine.h"
#include <cstring>

namespace ib
{
namespace
{
inline void storeMax (std::atomic<float>& a, float v)
{
    if (v > a.load (std::memory_order_relaxed)) a.store (v, std::memory_order_relaxed);
}
inline double fastDbToGain (double db) { return std::exp (db * 0.11512925464970229); }   // ln(10)/20
} // namespace

// ============================================================================ Iron
const Iron::Voice& Iron::voice (int t)
{
    //                         bias  drive  highs  hyst   HF cut   LF cut  low boost  headroom
    static const Voice nickel { 0.22, 1.00, 0.12, 0.12, 32000.0,  6.0, 2.5, 1.0 };   // round, 2nd-harmonic rich
    static const Voice steel  { 0.06, 1.25, 0.20, 0.25, 21000.0, 14.0, 2.0, 1.0 };   // grittier, more 3rd, tighter lows
    static const Voice clean  { 0.02, 0.20, 0.10, 0.00, 45000.0,  3.0, 0.5, 0.6 };   // modern, barely there
    return t == 1 ? steel : (t == 2 ? clean : nickel);
}

void Iron::prepare (double sampleRate)
{
    fs = sampleRate;
    type = -1;
    for (int c = 0; c < 2; ++c) split[c].set (fs, 200.0);
    reset();
}

void Iron::reset()
{
    for (int c = 0; c < 2; ++c)
    {
        split[c].reset(); hf[c].reset(); lf[c].reset();
        low[c].reset(); high[c].reset();
        lastErr[c] = 0;
    }
}

void Iron::set (int t, double drive, double lowWeight)
{
    const auto& v = voice (t);
    if (t != type)
    {
        type = t;
        for (int c = 0; c < 2; ++c)
        {
            low[c].s.setBias (v.bias); high[c].s.setBias (v.bias * 0.5);
            hf[c].set (fs, v.hfCut); lf[c].set (fs, v.lfCut);
            low[c].reset(); high[c].reset();
        }
    }
    const double g = v.headroom * dbToGain (v.driveScale * clampd (drive, 0, 1) * 24.0);
    gLow = std::max (0.05, g * (1.0 + v.lowBoost * clampd (lowWeight, 0, 1)));
    gHigh = std::max (0.05, g * v.highAmount);
    hyst = v.hyst;
}

inline double Iron::process (int c, double x)
{
    const double lo = split[c].process (x), hi = x - lo;
    const double u = gLow * lo + hyst * lastErr[c];
    const double yu = low[c].process (u);
    lastErr[c] = clampd (u - yu, -4.0, 4.0);
    double y = yu / gLow + high[c].process (gHigh * hi) / gHigh;
    y = hf[c].process (y);
    return lf[c].process (y);
}

// ============================================================================ Glue
void Glue::prepare (double sampleRate)
{
    fs = sampleRate;
    hpfHz = -1;
    set (s);
    reset();
}

void Glue::reset()
{
    for (int c = 0; c < 2; ++c) { gr[c] = slowGr[c] = 0; sc[c].reset(); }
    deepestGrDb = 0;
}

void Glue::set (const Settings& st)
{
    s = st;
    auto coeff = [this] (double sec) { return std::exp (-1.0 / (std::max (1.0e-5, sec) * fs)); };
    atk = coeff (s.attackMs * 0.001);
    fixedRel = s.releaseSec > 0 ? coeff (s.releaseSec) : 0.0;
    autoFast = coeff (0.1); autoSlow = coeff (1.2);
    slowC = coeff (2.0);
    makeup = dbToGain (s.makeupDb);
    if (s.scHpfHz != hpfHz)
    {
        hpfHz = s.scHpfHz;
        for (int c = 0; c < 2; ++c) { if (hpfHz > 0) sc[c].highpass (fs, hpfHz, 0.707); else sc[c].setIdentity(); }
    }
}

double Glue::staticGr (double lvl, double threshDb, double ratio)
{
    const double knee = 4.0, slope = 1.0 / ratio - 1.0, over = lvl - threshDb;
    if (over > knee * 0.5) return slope * over;
    if (over > -knee * 0.5) return slope * (over + knee * 0.5) * (over + knee * 0.5) / (2 * knee);
    return 0.0;
}

inline void Glue::process (double& l, double& r)
{
    const double aL = std::fabs (sc[0].process (l)), aR = std::fabs (sc[1].process (r));
    const double m = std::max (aL, aR);
    const double det[2] = { s.link * m + (1.0 - s.link) * aL, s.link * m + (1.0 - s.link) * aR };
    double* io[2] = { &l, &r };
    for (int c = 0; c < 2; ++c)
    {
        const double target = staticGr (gainToDb (det[c], -120.0), s.threshDb, s.ratio);
        double rel = fixedRel;
        if (s.releaseSec <= 0)
        {
            // Auto: short peaks recover fast, sustained reduction recovers slowly.
            const double sustained = clampd (-slowGr[c] / 6.0, 0.0, 1.0);
            rel = autoFast + (autoSlow - autoFast) * sustained;
        }
        const double k = target < gr[c] ? atk : rel;
        gr[c] = target + k * (gr[c] - target);
        slowGr[c] = gr[c] + slowC * (slowGr[c] - gr[c]);
        deepestGrDb = std::min (deepestGrDb, gr[c]);
        const double g = fastDbToGain (gr[c]) * makeup;
        *io[c] *= g * s.mix + (1.0 - s.mix);
    }
}

// ============================================================================ Tape
void Tape::prepare (double sampleRate)
{
    fs = sampleRate;
    sat = bump = hfLoss = -1;
    for (int c = 0; c < 2; ++c) shaper[c].s.setBias (0.04);
    reset();
}

void Tape::reset()
{
    for (int c = 0; c < 2; ++c) { pre[c].reset(); de[c].reset(); headBump[c].reset(); gap[c].reset(); shaper[c].reset(); }
}

void Tape::set (double s, double b, double h)
{
    if (s == sat && b == bump && h == hfLoss) return;
    sat = s; bump = b; hfLoss = h;
    const double emph = 5.0 * sat;
    g = dbToGain (sat * 14.0);
    for (int c = 0; c < 2; ++c)
    {
        pre[c].highShelf (fs, 3000.0, emph);
        de[c].highShelf (fs, 3000.0, -emph);
        headBump[c].peak (fs, 90.0, 0.9, bump * 3.5);
        if (hfLoss < 0.005) gap[c].setIdentity();
        else gap[c].lowpass (fs, 22000.0 * std::pow (5500.0 / 22000.0, hfLoss), 0.6);
    }
}

inline double Tape::process (int c, double x)
{
    double y = pre[c].process (x);
    y = shaper[c].process (g * y) / g;
    y = de[c].process (y);
    y = headBump[c].process (y);
    return gap[c].process (y);
}

// ============================================================================ Clip
void Clipper::reset() { for (auto& s : shaper) s.reset(); peakOver = 0; }

void Clipper::set (double ceilingDb, double softness)
{
    ceiling = dbToGain (ceilingDb);
    const double k = 1.0 - 0.85 * clampd (softness, 0, 1);
    for (auto& s : shaper) s.s.k = k;
}

inline double Clipper::process (int c, double x)
{
    const double u = x / ceiling;
    peakOver = std::max (peakOver, std::fabs (u));
    return ceiling * shaper[c].process (u);
}

// ============================================================================ Engine
Engine::Engine()
{
    const auto* d = paramDefs();
    for (int i = 0; i < NumParams; ++i) values[(size_t) i] = d[i].def;
}

int Engine::latencyFor (int idx)
{
    if (idx <= 0) return 0;
    if (idx == 1) return Oversampler2x::kLatency;
    return Oversampler2x::kLatency + Oversampler2x::kLatency / 2;   // inner stage runs at 2x
}

int Engine::latencySamples() const { return latencyFor ((int) values[Oversample]); }

void Engine::prepare (double sampleRate, int block)
{
    fs = sampleRate;
    maxBlock = std::max (1, block);
    osIndex = -1;
    inGain.setTime (fs, 0.02); outGain.setTime (fs, 0.02);
    update();
    for (auto* s : { &inGain, &outGain, &onIron, &onGlue, &onTape, &onClip }) s->snap (s->target);
    reset();
}

void Engine::reset()
{
    iron.reset(); glue.reset(); tape.reset(); clip.reset();
    for (int c = 0; c < 2; ++c) { os1[c].reset(); os2[c].reset(); }
    std::memset (lastOut, 0, sizeof (lastOut));
}

void Engine::setOversampling (int idx)
{
    idx = std::max (0, std::min (2, idx));
    if (idx == osIndex) return;
    osIndex = idx;
    osFactor = 1 << idx;
    fsOs = fs * osFactor;
    iron.prepare (fsOs); glue.prepare (fsOs); tape.prepare (fsOs); clip.reset();
    for (int c = 0; c < 2; ++c) { os1[c].reset(); os2[c].reset(); }
    for (auto* s : { &onIron, &onGlue, &onTape, &onClip }) s->setTime (fsOs, 0.02);
}

void Engine::update()
{
    const auto& v = values;
    setOversampling ((int) std::lround (v[Oversample]));
    inGain.target = dbToGain (v[InGain]);
    outGain.target = dbToGain (v[OutGain]);
    onIron.target = v[IronOn] > 0.5f ? 1.0 : 0.0;
    onGlue.target = v[GlueOn] > 0.5f ? 1.0 : 0.0;
    onTape.target = v[TapeOn] > 0.5f ? 1.0 : 0.0;
    onClip.target = v[ClipOn] > 0.5f ? 1.0 : 0.0;
    glueFirst = std::lround (v[Order]) == 1;

    iron.set ((int) std::lround (v[IronType]), v[IronDrive], v[IronLow]);

    auto pick = [] (const float* table, float idx, int n) { return (double) table[std::max (0, std::min (n - 1, (int) std::lround (idx)))]; };
    Glue::Settings gs;
    gs.threshDb = v[GlueThresh];
    gs.ratio = pick (ratioValues(), v[GlueRatio], 4);
    gs.attackMs = pick (attackMs(), v[GlueAttack], 6);
    gs.releaseSec = pick (releaseSec(), v[GlueRelease], 5);
    gs.scHpfHz = pick (scHpfHz(), v[GlueScHpf], 5);
    gs.makeupDb = v[GlueMakeup];
    gs.mix = clampd (v[GlueMix], 0, 1);
    gs.link = clampd (v[Link], 0, 1);
    glue.set (gs);

    tape.set (v[TapeSat], v[TapeBump], v[TapeHf]);
    clip.set (v[ClipCeiling], v[ClipSoft]);
}

inline void Engine::processSample (double& l, double& r)
{
    auto doIron = [&]
    {
        const double a = onIron.next();
        if (a < 1.0e-6) return;
        const double wl = iron.process (0, l), wr = iron.process (1, r);
        l += a * (wl - l); r += a * (wr - r);
    };
    auto doGlue = [&]
    {
        const double a = onGlue.next();
        if (a < 1.0e-6) return;
        double wl = l, wr = r;
        glue.process (wl, wr);
        l += a * (wl - l); r += a * (wr - r);
    };

    if (glueFirst) { doGlue(); doIron(); }
    else { doIron(); doGlue(); }

    const double at = onTape.next();
    if (at > 1.0e-6)
    {
        const double wl = tape.process (0, l), wr = tape.process (1, r);
        l += at * (wl - l); r += at * (wr - r);
    }
    const double ac = onClip.next();
    if (ac > 1.0e-6)
    {
        const double wl = clip.process (0, l), wr = clip.process (1, r);
        l += ac * (wl - l); r += ac * (wr - r);
    }
}

void Engine::process (const float* inL, const float* inR, float* outL, float* outR, int n)
{
    update();
    double pkIn = 0, pkOut = 0;
    for (int i = 0; i < n; ++i)
    {
        const double gi = inGain.next();
        double l = inL[i] * gi, r = inR[i] * gi;
        pkIn = std::max (pkIn, std::max (std::fabs (l), std::fabs (r)));

        if (osFactor == 1)
            processSample (l, r);
        else
        {
            double x[2] = { l, r };
            double a[2][4];
            for (int c = 0; c < 2; ++c)
            {
                double u0, u1;
                os1[c].upsample (x[c], u0, u1);
                if (osFactor == 2) { a[c][0] = u0; a[c][1] = u1; }
                else { os2[c].upsample (u0, a[c][0], a[c][1]); os2[c].upsample (u1, a[c][2], a[c][3]); }
            }
            for (int k = 0; k < osFactor; ++k) processSample (a[0][k], a[1][k]);
            for (int c = 0; c < 2; ++c)
            {
                if (osFactor == 2) x[c] = os1[c].downsample (a[c][0], a[c][1]);
                else
                {
                    // two statements: the call order matters (argument evaluation order is unspecified)
                    const double d0 = os2[c].downsample (a[c][0], a[c][1]);
                    const double d1 = os2[c].downsample (a[c][2], a[c][3]);
                    x[c] = os1[c].downsample (d0, d1);
                }
            }
            l = x[0]; r = x[1];
        }

        const double go = outGain.next();
        l *= go; r *= go;

        // true-peak style estimate: the samples themselves plus three cubic-interpolated points
        // between the previous two samples (one sample of look-behind, no added latency)
        const double y[2] = { l, r };
        for (int c = 0; c < 2; ++c)
        {
            const double ym1 = lastOut[c][2], y0 = lastOut[c][1], y1 = lastOut[c][0], y2 = y[c];
            const double c1 = 0.5 * (y1 - ym1), c2 = ym1 - 2.5 * y0 + 2.0 * y1 - 0.5 * y2, c3 = 0.5 * (y2 - ym1) + 1.5 * (y0 - y1);
            for (double t : { 0.25, 0.5, 0.75 }) pkOut = std::max (pkOut, std::fabs (((c3 * t + c2) * t + c1) * t + y0));
            pkOut = std::max (pkOut, std::fabs (y2));
            lastOut[c][2] = y0; lastOut[c][1] = y1; lastOut[c][0] = y2;
        }

        outL[i] = (float) l;
        outR[i] = (float) r;
    }

    storeMax (inPeak, (float) pkIn);
    storeMax (outPeak, (float) pkOut);
    if (onGlue.v > 0.5) storeMax (grDb, (float) -glue.deepestGrDb);
    glue.deepestGrDb = 0;
    if (onClip.v > 0.5 && clip.peakOver > 1.0) storeMax (clipDb, (float) gainToDb (clip.peakOver));
    clip.peakOver = 0;
}
} // namespace ib
