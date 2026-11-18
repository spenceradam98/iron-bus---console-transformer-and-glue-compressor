// Small DSP building blocks for Iron Bus. Plain C++17, scalar code only (no SIMD intrinsics),
// so it runs on any x86-64 (macOS 11 Intel) or arm64 machine.
#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace ib
{
constexpr double kPi = 3.14159265358979323846;

inline double dbToGain (double db) { return std::pow (10.0, db / 20.0); }
inline double gainToDb (double g, double floorDb = -120.0) { return g > 0 ? std::max (floorDb, 20.0 * std::log10 (g)) : floorDb; }
inline double clampd (double x, double lo, double hi) { return x < lo ? lo : (x > hi ? hi : x); }

// log(cosh(x)) without overflow: |x| + log1p(exp(-2|x|)) - log(2)
inline double logCosh (double x)
{
    const double a = std::fabs (x);
    return a + std::log1p (std::exp (-2.0 * a)) - 0.69314718055994530942;
}

// RBJ biquad, transposed direct form II, double state.
struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;

    void reset() { z1 = z2 = 0; }
    void setIdentity() { b0 = 1; b1 = b2 = a1 = a2 = 0; }
    inline double process (double x)
    {
        const double y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }
    void norm (double B0, double B1, double B2, double A0, double A1, double A2)
    {
        b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0; a1 = A1 / A0; a2 = A2 / A0;
    }
    static double w0 (double fs, double fc) { return 2.0 * kPi * clampd (fc, 5.0, fs * 0.47) / fs; }

    void lowpass (double fs, double fc, double q)
    {
        const double w = w0 (fs, fc), c = std::cos (w), al = std::sin (w) / (2 * q);
        norm ((1 - c) / 2, 1 - c, (1 - c) / 2, 1 + al, -2 * c, 1 - al);
    }
    void highpass (double fs, double fc, double q)
    {
        const double w = w0 (fs, fc), c = std::cos (w), al = std::sin (w) / (2 * q);
        norm ((1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + al, -2 * c, 1 - al);
    }
    void peak (double fs, double fc, double q, double db)
    {
        const double A = std::pow (10.0, db / 40.0), w = w0 (fs, fc), c = std::cos (w), al = std::sin (w) / (2 * q);
        norm (1 + al * A, -2 * c, 1 - al * A, 1 + al / A, -2 * c, 1 - al / A);
    }
    void highShelf (double fs, double fc, double db, double S = 0.8)
    {
        const double A = std::pow (10.0, db / 40.0), w = w0 (fs, fc), c = std::cos (w), s = std::sin (w);
        const double al = s / 2 * std::sqrt ((A + 1 / A) * (1 / S - 1) + 2), sa = 2 * std::sqrt (A) * al;
        norm (A * ((A + 1) + (A - 1) * c + sa), -2 * A * ((A - 1) + (A + 1) * c), A * ((A + 1) + (A - 1) * c - sa),
              (A + 1) - (A - 1) * c + sa, 2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - sa);
    }
};

struct OnePoleLP
{
    double a = 0, z = 0;
    void set (double fs, double fc) { a = std::exp (-2.0 * kPi * clampd (fc, 1.0, fs * 0.49) / fs); }
    inline double process (double x) { z = x + a * (z - x); return z; }
    void reset (double v = 0) { z = v; }
};

// First-order high-pass (the transformer's low-frequency coupling: a little phase shift in the lows).
struct OnePoleHP
{
    double r = 0.999, x1 = 0, y1 = 0;
    void set (double fs, double fc) { r = std::exp (-2.0 * kPi * fc / fs); }
    void reset() { x1 = y1 = 0; }
    inline double process (double x) { const double y = (1.0 + r) * 0.5 * (x - x1) + r * y1; x1 = x; y1 = y; return y; }
};

// Deterministic xorshift RNG (no allocation, real-time safe).
struct Rng
{
    unsigned s = 0x9E3779B9u;
    explicit Rng (unsigned seed = 0x9E3779B9u) : s (seed ? seed : 1u) {}
    inline unsigned next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
    inline double uni() { return (next() >> 8) * (1.0 / 16777216.0); }   // [0,1)
    inline double bi() { return uni() * 2.0 - 1.0; }                      // [-1,1)
};

// One-pole parameter smoother.
struct Smoothed
{
    double v = 0, target = 0, a = 0.999;
    void setTime (double fs, double sec) { a = std::exp (-1.0 / (std::max (1.0e-4, sec) * fs)); }
    void snap (double x) { v = target = x; }
    inline double next() { v = target + a * (v - target); return v; }
    bool settledAt (double x) const { return std::fabs (v - x) < 1.0e-6 && std::fabs (target - x) < 1.0e-9; }
};

// ---------------------------------------------------------------- antiderivative anti-aliasing (ADAA)
// First-order ADAA: y[n] = (F(x[n]) - F(x[n-1])) / (x[n] - x[n-1]), falling back to f(midpoint)
// when the step is tiny. Cuts aliasing of the waveshapers a lot for very little CPU, which keeps the
// "Off" oversampling mode usable on a tracking session.
template <typename Shape>
struct Adaa
{
    Shape s;
    double x1 = 0, F1 = 0;
    void reset() { x1 = 0; F1 = s.F (0.0); }
    inline double process (double x)
    {
        const double F = s.F (x), d = x - x1;
        const double y = std::fabs (d) > 1.0e-5 ? (F - F1) / d : s.f (0.5 * (x + x1));
        x1 = x; F1 = F;
        return y;
    }
};

// tanh(u + b) - tanh(b), normalised so the slope at u = 0 is 1. The bias b gives even (2nd) harmonics.
struct BiasedTanh
{
    double b = 0, tb = 0, norm = 1;
    void setBias (double bias) { b = bias; tb = std::tanh (b); norm = 1.0 / (1.0 - tb * tb); }
    inline double f (double u) const { return (std::tanh (u + b) - tb) * norm; }
    inline double F (double u) const { return (logCosh (u + b) - logCosh (b) - u * tb) * norm; }
};

// Linear up to the knee k, then a tanh curve that reaches the ceiling (1.0) smoothly. k = 1 is a hard clip.
struct KneeClip
{
    double k = 0.5;
    inline double f (double x) const
    {
        const double a = std::fabs (x);
        double y;
        if (a <= k) y = a;
        else if (k >= 1.0 - 1.0e-6) y = 1.0;
        else y = k + (1.0 - k) * std::tanh ((a - k) / (1.0 - k));
        return x < 0 ? -y : y;
    }
    inline double F (double x) const
    {
        const double a = std::fabs (x);
        if (a <= k) return 0.5 * a * a;
        if (k >= 1.0 - 1.0e-6) return 0.5 * k * k + k * (a - k);
        const double w = 1.0 - k;
        return 0.5 * k * k + k * (a - k) + w * w * logCosh ((a - k) / w);
    }
};

// ---------------------------------------------------------------- oversampling
// 2x half-band polyphase FIR up/down sampler. 65 taps, Kaiser window: about 90 dB stopband.
// Linear phase: up + down together delay the signal by exactly kLatency base-rate samples.
class Oversampler2x
{
public:
    static constexpr int kTaps = 65;
    static constexpr int kLatency = (kTaps - 1) / 2;   // 32 samples at 2x rate twice = 32 base samples

    Oversampler2x()
    {
        const int M = kTaps - 1;
        const double beta = 9.0;
        auto bessel0 = [] (double x)
        {
            double sum = 1, term = 1;
            for (int k = 1; k < 30; ++k) { term *= (x / (2 * k)) * (x / (2 * k)); sum += term; }
            return sum;
        };
        double total = 0;
        for (int i = 0; i < kTaps; ++i)
        {
            const double m = i - M / 2.0;
            const double sinc = 2 * i == M ? 0.5 : std::sin (kPi * 0.5 * m) / (kPi * m);
            const double r = 2.0 * i / M - 1.0;
            h[(size_t) i] = sinc * bessel0 (beta * std::sqrt (std::max (0.0, 1.0 - r * r))) / bessel0 (beta);
            total += h[(size_t) i];
        }
        for (auto& c : h) c /= total;
        reset();
    }

    void reset() { up.fill (0); down.fill (0); upPos = downPos = 0; }

    // One input sample -> two output samples.
    inline void upsample (double x, double& y0, double& y1)
    {
        upPos = (upPos + kHist - 1) % kHist;
        up[(size_t) upPos] = x;
        up[(size_t) (upPos + kHist)] = x;   // mirrored copy: reads never wrap
        const double* xp = &up[(size_t) upPos];
        double a = 0, b = 0;
        for (int k = 0; k <= kTaps / 2; ++k) a += h[(size_t) (2 * k)] * xp[k];
        for (int k = 0; k < kTaps / 2; ++k) b += h[(size_t) (2 * k + 1)] * xp[k];
        y0 = 2.0 * a; y1 = 2.0 * b;
    }

    // Two input samples (oldest first) -> one output sample.
    inline double downsample (double x0, double x1)
    {
        // evaluate the filter on the even sample so the delay is a whole number of base samples
        push (x0);
        const double* xp = &down[(size_t) downPos];
        double y = 0;
        for (int i = 0; i < kTaps; ++i) y += h[(size_t) i] * xp[i];
        push (x1);
        return y;
    }

private:
    static constexpr int kHist = kTaps;
    inline void push (double x)
    {
        downPos = (downPos + kHist - 1) % kHist;
        down[(size_t) downPos] = x;
        down[(size_t) (downPos + kHist)] = x;
    }
    std::array<double, kTaps> h {};
    std::array<double, 2 * kHist> up {}, down {};
    int upPos = 0, downPos = 0;
};
} // namespace ib
