// The parameter table: the single source of truth for the engine, the plugin's
// AudioProcessorValueTreeState, the presets and the editor.
#pragma once

namespace ib
{
enum class Kind { Float, Choice, Bool };

struct ParamDef
{
    const char* id;
    const char* name;
    Kind kind;
    float min, max, def;
    float centre;          // value shown at 12 o'clock for skewed ranges (0 = linear)
    const char* unit;
    const char* choices;   // '|' separated, for Choice
};

// X(enumName, id, display name, kind, min, max, default, centre, unit, choices)
#define IB_PARAMS(X) \
    X (InGain,      "in_gain",      "Input",            Float, -24, 24, 0, 0, "dB", "") \
    X (OutGain,     "out_gain",     "Output",           Float, -24, 24, 0, 0, "dB", "") \
    X (Oversample,  "oversample",   "Oversampling",     Choice, 0, 2, 0, 0, "", "Off|2x|4x") \
    X (Link,        "link",         "Stereo Link",      Float,  0, 1, 1, 0, "%", "") \
    X (Order,       "order",        "Order",            Choice, 0, 1, 0, 0, "", "Iron > Glue|Glue > Iron") \
    X (IronOn,      "iron_on",      "Iron On",          Bool,   0, 1, 1, 0, "", "") \
    X (IronDrive,   "iron_drive",   "Iron Drive",       Float,  0, 1, 0.3f, 0, "%", "") \
    X (IronType,    "iron_type",    "Iron Type",        Choice, 0, 2, 0, 0, "", "Nickel|Steel|Clean") \
    X (IronLow,     "iron_low",     "Low Weight",       Float,  0, 1, 0.5f, 0, "%", "") \
    X (GlueOn,      "glue_on",      "Glue On",          Bool,   0, 1, 1, 0, "", "") \
    X (GlueThresh,  "glue_thresh",  "Threshold",        Float, -40, 0, -12, 0, "dB", "") \
    X (GlueRatio,   "glue_ratio",   "Ratio",            Choice, 0, 3, 1, 0, "", "1.5:1|2:1|4:1|10:1") \
    X (GlueAttack,  "glue_attack",  "Attack",           Choice, 0, 5, 3, 0, "", "0.1 ms|0.3 ms|1 ms|3 ms|10 ms|30 ms") \
    X (GlueRelease, "glue_release", "Release",          Choice, 0, 4, 4, 0, "", "0.1 s|0.3 s|0.6 s|1.2 s|Auto") \
    X (GlueScHpf,   "glue_schpf",   "SC HPF",           Choice, 0, 4, 0, 0, "", "Off|60 Hz|90 Hz|150 Hz|250 Hz") \
    X (GlueMakeup,  "glue_makeup",  "Make-up",          Float,  0, 18, 0, 0, "dB", "") \
    X (GlueMix,     "glue_mix",     "Glue Mix",         Float,  0, 1, 1, 0, "%", "") \
    X (TapeOn,      "tape_on",      "Tape On",          Bool,   0, 1, 0, 0, "", "") \
    X (TapeSat,     "tape_sat",     "Saturation",       Float,  0, 1, 0.3f, 0, "%", "") \
    X (TapeBump,    "tape_bump",    "Head Bump",        Float,  0, 1, 0.4f, 0, "%", "") \
    X (TapeHf,      "tape_hf",      "HF Loss",          Float,  0, 1, 0.25f, 0, "%", "") \
    X (ClipOn,      "clip_on",      "Clip On",          Bool,   0, 1, 0, 0, "", "") \
    X (ClipCeiling, "clip_ceiling", "Ceiling",          Float, -12, 0, -0.3f, 0, "dB", "") \
    X (ClipSoft,    "clip_soft",    "Softness",         Float,  0, 1, 0.5f, 0, "%", "")

enum P : int
{
#define IB_ENUM(e, ...) e,
    IB_PARAMS (IB_ENUM)
#undef IB_ENUM
    NumParams
};

inline const ParamDef* paramDefs()
{
#define IB_DEF(e, id, name, kind, mn, mx, df, ct, unit, ch) ParamDef { id, name, Kind::kind, (float) (mn), (float) (mx), (float) (df), (float) (ct), unit, ch },
    static const ParamDef defs[NumParams] = { IB_PARAMS (IB_DEF) };
#undef IB_DEF
    return defs;
}

// Index for an id string, or -1.
int paramIndex (const char* id);

// Values behind the stepped controls.
inline const float* ratioValues()   { static const float v[] = { 1.5f, 2.0f, 4.0f, 10.0f }; return v; }
inline const float* attackMs()      { static const float v[] = { 0.1f, 0.3f, 1.0f, 3.0f, 10.0f, 30.0f }; return v; }
inline const float* releaseSec()    { static const float v[] = { 0.1f, 0.3f, 0.6f, 1.2f, -1.0f /* auto */ }; return v; }
inline const float* scHpfHz()       { static const float v[] = { 0.0f, 60.0f, 90.0f, 150.0f, 250.0f }; return v; }
} // namespace ib
