// Engine tests: parameter table and presets, unity gain with every stage off (at every oversampling
// setting, with the reported latency), glue gain reduction matching the ratio at steady state,
// transformer behaviour, clipper ceiling, every preset stable, and CPU headroom.
#include "Engine.h"
#include <chrono>
#include <cstdio>
#include <set>
#include <string>

using namespace ib;

static int failures = 0;
#define CHECK(cond, ...) do { if (! (cond)) { ++failures; std::printf ("FAIL %s:%d: ", __FILE__, __LINE__); std::printf (__VA_ARGS__); std::printf ("\n"); } } while (0)

static const double fs = 48000;

static void setAll (Engine& e, const std::array<float, NumParams>& v) { for (int i = 0; i < NumParams; ++i) e.setParam (i, v[(size_t) i]); }

static std::array<float, NumParams> allOff()
{
    auto v = presetValues (Preset { "", "", {} });
    v[IronOn] = 0; v[GlueOn] = 0; v[TapeOn] = 0; v[ClipOn] = 0;
    return v;
}

// Runs a mono signal (same on both channels) through the engine in blocks of 256.
static std::vector<float> run (Engine& e, const std::vector<float>& in, std::vector<float>* right = nullptr)
{
    std::vector<float> l = in, r = in;
    for (size_t pos = 0; pos < in.size(); pos += 256)
    {
        const int n = (int) std::min<size_t> (256, in.size() - pos);
        e.process (l.data() + pos, r.data() + pos, l.data() + pos, r.data() + pos, n);   // in place
    }
    if (right) *right = r;
    return l;
}

static std::vector<float> sine (double freq, double peakDb, double seconds)
{
    std::vector<float> x ((size_t) (seconds * fs));
    const double a = dbToGain (peakDb);
    for (size_t i = 0; i < x.size(); ++i) x[i] = (float) (a * std::sin (2 * kPi * freq * (double) i / fs));
    return x;
}

static double peakDb (const std::vector<float>& x, size_t from, size_t to)
{
    double p = 0;
    for (size_t i = from; i < to && i < x.size(); ++i) p = std::max (p, (double) std::fabs (x[i]));
    return gainToDb (p);
}

static double rms (const std::vector<float>& x, size_t from, size_t to)
{
    double s = 0; size_t n = 0;
    for (size_t i = from; i < to && i < x.size(); ++i) { s += (double) x[i] * x[i]; ++n; }
    return std::sqrt (s / std::max<size_t> (1, n));
}

// Amplitude of one frequency (Goertzel over a whole number of cycles).
static double toneAmp (const std::vector<float>& x, size_t from, size_t len, double freq)
{
    double re = 0, im = 0;
    for (size_t i = 0; i < len; ++i)
    {
        const double ph = 2 * kPi * freq * (double) i / fs;
        re += x[from + i] * std::cos (ph); im += x[from + i] * std::sin (ph);
    }
    return 2.0 * std::sqrt (re * re + im * im) / (double) len;
}

int main()
{
    // ---- parameter table
    {
        const auto* d = paramDefs();
        for (int i = 0; i < NumParams; ++i)
        {
            CHECK (d[i].min < d[i].max, "%s range", d[i].id);
            CHECK (d[i].def >= d[i].min && d[i].def <= d[i].max, "%s default out of range", d[i].id);
            CHECK (paramIndex (d[i].id) == i, "%s duplicate id", d[i].id);
            if (d[i].kind == Kind::Choice)
            {
                int n = 1; for (const char* c = d[i].choices; *c; ++c) if (*c == '|') ++n;
                CHECK (n == (int) d[i].max + 1, "%s has %d choices but max %g", d[i].id, n, d[i].max);
            }
        }
        CHECK (d[Oversample].def == 0, "oversampling must default to Off (zero latency for tracking)");
    }

    // ---- presets: names unique, ids valid, banks valid, about 20 of them
    {
        std::set<std::string> names, banks (presetBanks().begin(), presetBanks().end());
        for (const auto& p : factoryPresets())
        {
            CHECK (names.insert (p.name).second, "duplicate preset name %s", p.name.c_str());
            CHECK (banks.count (p.bank) == 1, "preset %s has unknown bank %s", p.name.c_str(), p.bank.c_str());
            for (const auto& kv : p.values)
            {
                const int idx = paramIndex (kv.first.c_str());
                CHECK (idx >= 0, "preset %s: unknown id %s", p.name.c_str(), kv.first.c_str());
                if (idx >= 0) CHECK (kv.second >= paramDefs()[idx].min && kv.second <= paramDefs()[idx].max, "preset %s: %s out of range", p.name.c_str(), kv.first.c_str());
            }
        }
        CHECK (factoryPresets().size() >= 20, "only %d presets", (int) factoryPresets().size());
        std::printf ("presets: %d in %d banks\n", (int) factoryPresets().size(), (int) presetBanks().size());
    }

    // ---- unity gain with all stages off, oversampling Off: bit-exact pass-through
    {
        Engine e; setAll (e, allOff()); e.prepare (fs, 256);
        CHECK (e.latencySamples() == 0, "latency %d with oversampling off", e.latencySamples());
        std::vector<float> x ((size_t) fs);
        Rng rng (7);
        for (auto& s : x) s = (float) (0.5 * rng.bi());
        const auto y = run (e, x);
        double maxErr = 0;
        for (size_t i = 0; i < x.size(); ++i) maxErr = std::max (maxErr, (double) std::fabs (y[i] - x[i]));
        CHECK (maxErr < 1.0e-6, "all-off pass-through error %g", maxErr);
        std::printf ("unity (OS off): max error %.2e\n", maxErr);
    }

    // ---- unity gain with all stages off, oversampling 2x and 4x: delayed by exactly the reported latency
    for (int os = 1; os <= 2; ++os)
    {
        auto v = allOff(); v[Oversample] = (float) os;
        Engine e; setAll (e, v); e.prepare (fs, 256);
        const int lat = e.latencySamples();
        CHECK (lat == Engine::latencyFor (os) && lat > 0, "latency %d for os %d", lat, os);

        std::vector<float> imp (4096, 0.0f); imp[100] = 1.0f;
        const auto yi = run (e, imp);
        size_t at = 0; for (size_t i = 0; i < yi.size(); ++i) if (std::fabs (yi[i]) > std::fabs (yi[at])) at = i;
        CHECK ((int) at == 100 + lat, "os %d: impulse peak at %d, expected %d", os, (int) at, 100 + lat);

        e.reset();
        const auto x = sine (1000.0, -6.0, 1.0);
        const auto y = run (e, x);
        double err = 0, sig = 0;
        for (size_t i = 4000; i < x.size(); ++i) { const double d = y[i] - x[i - (size_t) lat]; err += d * d; sig += (double) x[i - (size_t) lat] * x[i - (size_t) lat]; }
        const double snr = 10 * std::log10 (sig / std::max (1e-30, err));
        CHECK (snr > 60.0, "os %d: 1 kHz null only %.1f dB", os, snr);
        std::printf ("unity (OS %dx): latency %d, impulse at +%d, 1 kHz null %.1f dB\n", 1 << os, lat, (int) at - 100, snr);
    }

    // ---- glue: gain reduction matches the ratio at steady state, for every ratio, at 1x and 2x
    for (int os = 0; os <= 1; ++os)
        for (int ri = 0; ri < 4; ++ri)
        {
            auto v = allOff();
            v[GlueOn] = 1; v[GlueThresh] = -20; v[GlueRatio] = (float) ri; v[GlueAttack] = 3; v[GlueRelease] = 3; v[Oversample] = (float) os;
            Engine e; setAll (e, v); e.prepare (fs, 256);
            const double inDb = -4.0;
            const auto y = run (e, sine (200.0, inDb, 3.0));
            const double outDb = peakDb (y, (size_t) (2.0 * fs), y.size());
            const double ratio = ratioValues()[ri];
            const double expect = inDb - 16.0 * (1.0 - 1.0 / ratio);
            CHECK (std::fabs (outDb - expect) < 0.6, "os %d ratio %.1f: out %.2f dB, expected %.2f dB", os, ratio, outDb, expect);
            if (os == 0) std::printf ("glue %4.1f:1  out %.2f dB (expected %.2f), GR %.2f dB\n", ratio, outDb, expect, inDb - outDb);
        }

    // ---- glue: below threshold nothing happens, mix 0 is dry, make-up adds level
    {
        auto v = allOff(); v[GlueOn] = 1; v[GlueThresh] = -10;
        Engine e; setAll (e, v); e.prepare (fs, 256);
        const auto y = run (e, sine (200.0, -20.0, 1.0));
        CHECK (std::fabs (peakDb (y, (size_t) (0.5 * fs), y.size()) + 20.0) < 0.1, "glue changed a signal below threshold");

        v[GlueThresh] = -30; v[GlueRatio] = 3; v[GlueMix] = 0; v[GlueMakeup] = 6;
        Engine e2; setAll (e2, v); e2.prepare (fs, 256);
        const auto y2 = run (e2, sine (200.0, -10.0, 1.0));
        CHECK (std::fabs (peakDb (y2, (size_t) (0.5 * fs), y2.size()) + 10.0) < 0.1, "glue mix 0 is not dry");
    }

    // ---- glue: sidechain HPF reduces how much the lows push the compressor
    {
        auto v = allOff(); v[GlueOn] = 1; v[GlueThresh] = -20; v[GlueRatio] = 2;
        Engine a; setAll (a, v); a.prepare (fs, 256);
        v[GlueScHpf] = 4;
        Engine b; setAll (b, v); b.prepare (fs, 256);
        const auto x = sine (50.0, -4.0, 2.0);
        const double ga = peakDb (run (a, x), (size_t) fs, x.size()), gb = peakDb (run (b, x), (size_t) fs, x.size());
        CHECK (gb > ga + 6.0, "SC HPF 250 Hz: 50 Hz output %.1f dB vs %.1f dB without", gb, ga);
    }

    // ---- glue: stereo link. A loud left channel should duck the right one only when linked.
    {
        auto v = allOff(); v[GlueOn] = 1; v[GlueThresh] = -20; v[GlueRatio] = 3;
        for (float link : { 1.0f, 0.0f })
        {
            v[Link] = link;
            Engine e; setAll (e, v); e.prepare (fs, 256);
            auto l = sine (200.0, -2.0, 1.0), r = sine (200.0, -30.0, 1.0);
            for (size_t pos = 0; pos < l.size(); pos += 256)
            {
                const int n = (int) std::min<size_t> (256, l.size() - pos);
                e.process (l.data() + pos, r.data() + pos, l.data() + pos, r.data() + pos, n);
            }
            const double rOut = peakDb (r, (size_t) (0.5 * fs), r.size());
            if (link > 0.5f) CHECK (rOut < -40.0, "linked: quiet right channel not ducked (%.1f dB)", rOut);
            else CHECK (std::fabs (rOut + 30.0) < 0.2, "unlinked: quiet right channel changed (%.1f dB)", rOut);
        }
    }

    // ---- iron: unity small-signal gain at any drive, harmonics when driven, steel more 3rd than nickel
    {
        double h3Ratio[2] {};
        for (int type = 0; type < 3; ++type)
        {
            auto v = allOff(); v[IronOn] = 1; v[IronType] = (float) type; v[IronDrive] = 1.0f;
            Engine e; setAll (e, v); e.prepare (fs, 256);
            const auto quiet = run (e, sine (1000.0, -50.0, 1.0));
            const double g = gainToDb (rms (quiet, 24000, 48000) / (dbToGain (-50.0) / std::sqrt (2.0)));
            CHECK (std::fabs (g) < 0.6, "iron type %d: small-signal gain %.2f dB", type, g);

            e.reset();
            const auto loud = run (e, sine (100.0, -6.0, 1.0));
            const double f1 = toneAmp (loud, 24000, 4800, 100), f2 = toneAmp (loud, 24000, 4800, 200), f3 = toneAmp (loud, 24000, 4800, 300);
            const double thd = std::sqrt (f2 * f2 + f3 * f3) / f1;
            std::printf ("iron %-6s small-signal %+.2f dB, 100 Hz @ -6 dB: H2 %.1f dB, H3 %.1f dB\n", type == 0 ? "Nickel" : type == 1 ? "Steel" : "Clean",
                         g, gainToDb (f2 / f1), gainToDb (f3 / f1));
            if (type < 2) { CHECK (thd > 0.01, "iron type %d hardly saturates at full drive (THD %.4f)", type, thd); h3Ratio[type] = f3 / std::max (1e-12, f2); }
            else CHECK (thd < 0.05, "Clean iron too dirty (THD %.3f)", thd);
        }
        CHECK (h3Ratio[1] > h3Ratio[0], "Steel should have more 3rd (vs 2nd) than Nickel");

        // lows saturate more than highs
        auto v = allOff(); v[IronOn] = 1; v[IronDrive] = 0.8f; v[IronLow] = 1.0f;
        Engine e; setAll (e, v); e.prepare (fs, 256);
        const auto lo = run (e, sine (60.0, -6.0, 1.0));
        e.reset();
        const auto hi = run (e, sine (3000.0, -6.0, 1.0));
        const double dLo = toneAmp (lo, 24000, 4800, 180) / toneAmp (lo, 24000, 4800, 60);
        const double dHi = toneAmp (hi, 24000, 4800, 9000) / toneAmp (hi, 24000, 4800, 3000);
        CHECK (dLo > 3.0 * dHi, "lows should saturate harder than highs (H3 %.4f vs %.4f)", dLo, dHi);
    }

    // ---- clipper holds the ceiling
    for (int os = 0; os <= 2; ++os)
    {
        auto v = allOff(); v[ClipOn] = 1; v[ClipCeiling] = -1.0f; v[ClipSoft] = 0.0f; v[Oversample] = (float) os;
        Engine e; setAll (e, v); e.prepare (fs, 256);
        const auto y = run (e, sine (440.0, 12.0, 1.0));
        const double p = peakDb (y, 4800, y.size());
        const double allowed = os == 0 ? 0.01 : 1.0;   // the anti-alias filters ring a little above a hard clip
        CHECK (p < -1.0 + allowed, "os %d: clipped peak %.2f dB above ceiling -1 dB", os, p);
        CHECK (e.clipDb.load() > 10.0f, "clip meter shows %.1f dB", e.clipDb.load());
        std::printf ("clip OS %dx: peak %.2f dB (ceiling -1)\n", 1 << os, p);
    }

    // ---- every preset: finite, sane level, silence gives silence
    for (const auto& p : factoryPresets())
    {
        Engine e; setAll (e, presetValues (p)); e.prepare (fs, 256);
        std::vector<float> x ((size_t) (2 * fs));
        Rng rng (3);
        OnePoleLP lp; lp.set (fs, 2500);
        for (size_t i = 0; i < x.size() / 2; ++i)
            x[i] = (float) (0.4 * std::sin (2 * kPi * 80.0 * (double) i / fs) * (i % 12000 < 3000 ? 1.0 : 0.3) + 0.2 * lp.process (rng.bi()));
        std::vector<float> r;
        const auto y = run (e, x, &r);
        bool finite = true;
        for (size_t i = 0; i < y.size(); ++i) finite = finite && std::isfinite (y[i]) && std::isfinite (r[i]);
        CHECK (finite, "preset %s produced non-finite output", p.name.c_str());
        const double pk = peakDb (y, 0, y.size());
        CHECK (pk < 9.0, "preset %s peaks at %.1f dB", p.name.c_str(), pk);
        const double tail = peakDb (y, (size_t) (1.7 * fs), y.size());
        CHECK (tail < -90.0, "preset %s: silence in gives %.1f dB out", p.name.c_str(), tail);
        const double gain = gainToDb (rms (y, 0, x.size() / 2) / rms (x, 0, x.size() / 2));
        CHECK (std::fabs (gain) < 12.0, "preset %s changes level by %.1f dB", p.name.c_str(), gain);
    }

    // ---- CPU: everything on at 4x
    {
        auto v = presetValues (Preset { "", "", {} });
        v[TapeOn] = 1; v[ClipOn] = 1; v[Oversample] = 2;
        Engine e; setAll (e, v); e.prepare (fs, 256);
        const auto x = sine (220.0, -6.0, 10.0);
        const auto t0 = std::chrono::steady_clock::now();
        run (e, x);
        const double sec = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
        std::printf ("CPU: 10 s stereo at 4x with every stage on took %.3f s (%.1f%% of real time)\n", sec, 100.0 * sec / 10.0);
        CHECK (sec < 5.0, "too slow: %.2f s for 10 s of audio", sec);
    }

    if (failures == 0) std::printf ("all engine tests passed\n");
    else std::printf ("%d failure(s)\n", failures);
    return failures == 0 ? 0 : 1;
}
