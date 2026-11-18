// Factory presets. Each preset lists only what differs from the parameter defaults.
//
// Choice indices, for reference:
//   oversample:   0 Off, 1 2x, 2 4x
//   order:        0 Iron > Glue, 1 Glue > Iron
//   iron_type:    0 Nickel, 1 Steel, 2 Clean
//   glue_ratio:   0 1.5:1, 1 2:1, 2 4:1, 3 10:1
//   glue_attack:  0 0.1 ms, 1 0.3 ms, 2 1 ms, 3 3 ms, 4 10 ms, 5 30 ms
//   glue_release: 0 0.1 s, 1 0.3 s, 2 0.6 s, 3 1.2 s, 4 Auto
//   glue_schpf:   0 Off, 1 60 Hz, 2 90 Hz, 3 150 Hz, 4 250 Hz
#include "Engine.h"
#include <cstring>

namespace ib
{
int paramIndex (const char* id)
{
    const auto* d = paramDefs();
    for (int i = 0; i < NumParams; ++i)
        if (std::strcmp (d[i].id, id) == 0) return i;
    return -1;
}

namespace
{
const char* const kMix = "Mix Bus";
const char* const kDrums = "Drums";
const char* const kVox = "Vocals & Keys";
const char* const kMaster = "Mastering";
const char* const kColour = "Character";
const char* const kUtil = "Utility";

std::vector<Preset> build()
{
    return {
        // ------------------------------------------------------------ Mix Bus
        { "Gentle Glue", kMix, { {"iron_drive",0.25f}, {"glue_thresh",-14}, {"glue_ratio",1}, {"glue_attack",4}, {"glue_release",4}, {"glue_schpf",2}, {"glue_makeup",1.5f} } },
        { "Mix Bus 2:1", kMix, { {"iron_drive",0.3f}, {"glue_thresh",-16}, {"glue_ratio",1}, {"glue_attack",3}, {"glue_release",4}, {"glue_schpf",1}, {"glue_makeup",2} } },
        { "Feather 1.5", kMix, { {"iron_type",2}, {"iron_drive",0.2f}, {"glue_thresh",-20}, {"glue_ratio",0}, {"glue_attack",5}, {"glue_release",3}, {"glue_makeup",2} } },
        { "Warm Iron", kMix, { {"iron_drive",0.55f}, {"iron_low",0.7f}, {"glue_thresh",-12}, {"glue_ratio",1}, {"glue_attack",4}, {"glue_release",4}, {"glue_makeup",1} } },
        { "Console Sum", kMix, { {"iron_type",0}, {"iron_drive",0.4f}, {"iron_low",0.4f}, {"glue_on",0}, {"tape_on",1}, {"tape_sat",0.15f}, {"tape_bump",0.25f}, {"tape_hf",0.1f} } },

        // ------------------------------------------------------------ Drums
        { "Drum Bus Smack", kDrums, { {"iron_type",1}, {"iron_drive",0.45f}, {"glue_thresh",-20}, {"glue_ratio",2}, {"glue_attack",4}, {"glue_release",0}, {"glue_schpf",2}, {"glue_makeup",5} } },
        { "Parallel Crush", kDrums, { {"iron_type",1}, {"iron_drive",0.6f}, {"glue_thresh",-32}, {"glue_ratio",3}, {"glue_attack",1}, {"glue_release",0}, {"glue_makeup",14}, {"glue_mix",0.4f} } },
        { "Room Pump", kDrums, { {"glue_thresh",-26}, {"glue_ratio",3}, {"glue_attack",5}, {"glue_release",1}, {"glue_schpf",3}, {"glue_makeup",8}, {"glue_mix",0.6f}, {"tape_on",1}, {"tape_sat",0.35f} } },
        { "Breakbeat Tape", kDrums, { {"iron_drive",0.4f}, {"glue_thresh",-18}, {"glue_ratio",2}, {"glue_attack",3}, {"glue_release",4}, {"glue_makeup",4},
                                      {"tape_on",1}, {"tape_sat",0.55f}, {"tape_bump",0.6f}, {"tape_hf",0.45f}, {"clip_on",1}, {"clip_ceiling",-1} } },
        { "Kick & Snare Glue", kDrums, { {"iron_drive",0.35f}, {"glue_thresh",-16}, {"glue_ratio",2}, {"glue_attack",5}, {"glue_release",1}, {"glue_schpf",4}, {"glue_makeup",3} } },

        // ------------------------------------------------------------ Vocals & Keys
        { "Vocal Bus", kVox, { {"iron_type",0}, {"iron_drive",0.3f}, {"iron_low",0.3f}, {"glue_thresh",-18}, {"glue_ratio",1}, {"glue_attack",2}, {"glue_release",4}, {"glue_schpf",3}, {"glue_makeup",3} } },
        { "Backing Vox Blend", kVox, { {"iron_type",2}, {"glue_thresh",-24}, {"glue_ratio",2}, {"glue_attack",3}, {"glue_release",2}, {"glue_schpf",3}, {"glue_makeup",6} } },
        { "Keys Glue", kVox, { {"iron_drive",0.35f}, {"glue_thresh",-15}, {"glue_ratio",0}, {"glue_attack",4}, {"glue_release",3}, {"glue_makeup",2}, {"tape_on",1}, {"tape_sat",0.2f}, {"tape_hf",0.2f} } },
        { "Guitar Bus Grit", kVox, { {"iron_type",1}, {"iron_drive",0.6f}, {"iron_low",0.3f}, {"glue_thresh",-14}, {"glue_ratio",1}, {"glue_attack",4}, {"glue_release",4}, {"glue_schpf",3}, {"glue_makeup",1.5f} } },

        // ------------------------------------------------------------ Mastering
        { "Tape Master", kMaster, { {"oversample",1}, {"iron_drive",0.2f}, {"glue_thresh",-10}, {"glue_ratio",0}, {"glue_attack",5}, {"glue_release",4}, {"glue_schpf",1}, {"glue_makeup",1.5f},
                                    {"tape_on",1}, {"tape_sat",0.25f}, {"tape_bump",0.35f}, {"tape_hf",0.15f}, {"clip_on",1}, {"clip_ceiling",-0.3f}, {"clip_soft",0.7f} } },
        { "Loud & Clipped", kMaster, { {"oversample",2}, {"in_gain",4}, {"iron_type",1}, {"iron_drive",0.35f}, {"glue_thresh",-14}, {"glue_ratio",1}, {"glue_attack",3}, {"glue_release",4}, {"glue_makeup",3},
                                       {"clip_on",1}, {"clip_ceiling",-0.5f}, {"clip_soft",0.35f} } },
        { "Clean Master Glue", kMaster, { {"oversample",1}, {"iron_type",2}, {"iron_drive",0.15f}, {"glue_thresh",-12}, {"glue_ratio",0}, {"glue_attack",4}, {"glue_release",4}, {"glue_schpf",2}, {"glue_makeup",1},
                                          {"clip_on",1}, {"clip_ceiling",-1}, {"clip_soft",0.8f} } },
        { "Soft Ceiling", kMaster, { {"oversample",2}, {"iron_on",0}, {"glue_on",0}, {"clip_on",1}, {"clip_ceiling",-1}, {"clip_soft",0.6f} } },

        // ------------------------------------------------------------ Character
        { "Steel Grit", kColour, { {"iron_type",1}, {"iron_drive",0.8f}, {"iron_low",0.6f}, {"glue_thresh",-14}, {"glue_ratio",2}, {"glue_attack",3}, {"glue_release",4}, {"glue_makeup",2}, {"out_gain",-3} } },
        { "Nickel Bloom", kColour, { {"iron_type",0}, {"iron_drive",0.75f}, {"iron_low",0.9f}, {"order",1}, {"glue_thresh",-16}, {"glue_ratio",1}, {"glue_attack",5}, {"glue_release",3}, {"glue_makeup",2}, {"out_gain",-2} } },
        { "Saturated Cassette", kColour, { {"iron_drive",0.5f}, {"glue_thresh",-18}, {"glue_ratio",2}, {"glue_release",4}, {"glue_makeup",4},
                                           {"tape_on",1}, {"tape_sat",0.8f}, {"tape_bump",0.7f}, {"tape_hf",0.7f}, {"out_gain",-2} } },
        { "Glue Into Iron", kColour, { {"order",1}, {"iron_type",1}, {"iron_drive",0.65f}, {"glue_thresh",-20}, {"glue_ratio",2}, {"glue_attack",2}, {"glue_release",4}, {"glue_makeup",6}, {"out_gain",-3} } },

        // ------------------------------------------------------------ Utility
        { "Init", kUtil, {} },
        { "Bypass (All Off)", kUtil, { {"iron_on",0}, {"glue_on",0} } },
        { "Iron Only", kUtil, { {"glue_on",0}, {"iron_drive",0.4f} } },
        { "Glue Only", kUtil, { {"iron_on",0}, {"glue_thresh",-16}, {"glue_ratio",1}, {"glue_makeup",2} } },
    };
}
} // namespace

const std::vector<Preset>& factoryPresets()
{
    static const std::vector<Preset> presets = build();
    return presets;
}

const std::vector<std::string>& presetBanks()
{
    static const std::vector<std::string> banks = { kMix, kDrums, kVox, kMaster, kColour, kUtil };
    return banks;
}

std::array<float, NumParams> presetValues (const Preset& p)
{
    std::array<float, NumParams> v {};
    const auto* d = paramDefs();
    for (int i = 0; i < NumParams; ++i) v[(size_t) i] = d[i].def;
    for (const auto& kv : p.values)
    {
        const int idx = paramIndex (kv.first.c_str());
        if (idx >= 0) v[(size_t) idx] = std::min (d[idx].max, std::max (d[idx].min, kv.second));
    }
    return v;
}
} // namespace ib
