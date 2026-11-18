# Iron Bus (working name): design notes

A master or bus processor: transformer saturation, a glue compressor, an optional light tape stage
and a soft clipper in one 1U strip. JUCE 8 + CMake, VST3 + AU + Standalone, macOS 11+ universal
(scalar C++, no `-march`) and Windows (static CRT). Effect only, no MIDI.

## Signal flow

```
in -> Input gain -> [ oversampling block (Off / 2x / 4x) ......................................... ]
                    [  Iron -> Glue   (or Glue -> Iron)  ->  Tape  ->  Clip                         ] -> Output gain -> out
                                        ^ sidechain: HPF -> stereo-linked peak detector
```

Every stage has an in/out switch with a 20 ms crossfade, so switching never clicks. With all four
stages out the strip is a bit-exact pass-through (tested).

## Stages

**Iron (transformer).** The input is split at ~200 Hz with a one-pole filter (the two bands sum back
exactly). The lows drive a biased tanh core harder than the highs (core flux rises as frequency
falls; Low Weight sets how much harder). A small remanence term feeds back how hard the core was
pushed on the previous sample (hysteresis-lite). Then a gentle HF roll-off and a first-order LF
coupling high-pass that adds the slight low-end phase shift of an output transformer. The curve is
normalised to unity small-signal gain, so Drive adds colour, not level.

| Type | Character |
|---|---|
| Nickel | Biased core: rich 2nd harmonic, rounder lows, soft top (HF corner ~32 kHz) |
| Steel | Little bias, more drive: 3rd-harmonic grit, tighter lows (14 Hz coupling), HF corner ~21 kHz |
| Clean | Modern, barely there: low drive, very little bias, wide bandwidth |

**Glue (VCA bus compressor).** Faderhouse's mix-bus compressor: soft-knee (4 dB) peak detector in
dB, stepped attack (0.1 / 0.3 / 1 / 3 / 10 / 30 ms) and release (0.1 / 0.3 / 0.6 / 1.2 s), Auto
release (fast for peaks, slower as reduction is sustained), make-up and parallel mix. Added here:
a 1.5:1 ratio, a 12 dB/oct sidechain high-pass (60 / 90 / 150 / 250 Hz) so kick and bass don't pump
the mix, and a variable stereo link (100% = one gain for both sides). The engine tests check that the
steady-state gain reduction matches every ratio within 0.6 dB.

**Tape.** Tapehead Echo's record path at subtle settings: 3 kHz pre-emphasis, tanh saturation,
matching de-emphasis (so highs compress first, like tape), head bump (a peak near 90 Hz, up to
+3.5 dB) and HF loss (a low-pass from 22 kHz down to 5.5 kHz).

**Clip.** Linear up to a knee, then a tanh curve that meets the ceiling smoothly; Softness moves the
knee from the ceiling (hard clip) down to -16 dB below it. The output ladder is true-peak style (each
sample plus three interpolated points between samples) and the CLIP LED lights when the clipper is
working.

## Oversampling and aliasing

- Off (default): zero latency, for tracking. All waveshapers (iron, tape, clip) use first-order
  antiderivative anti-aliasing (ADAA), which removes most aliasing at almost no CPU cost.
- 2x / 4x: 65-tap half-band polyphase FIR stages (Kaiser window, about 90 dB stopband), linear phase.
  Latency is a whole number of samples (32 at 2x, 48 at 4x), reported to the host; the tests check the
  impulse lands exactly there and a 1 kHz tone nulls by more than 90 dB.
- The whole chain (compressor included) runs at the oversampled rate. Everything on at 4x uses about
  6% of one core for a stereo 48 kHz stream on the CI-class Linux box.

## Presets

26 factory presets in six banks: Mix Bus, Drums, Vocals & Keys, Mastering, Character, Utility
(Gentle Glue, Mix Bus 2:1, Warm Iron, Drum Bus Smack, Parallel Crush, Vocal Bus, Tape Master,
Loud & Clipped, Steel Grit, ...). User presets save as `.ibpreset` files in the user's
application-data folder (`IronBus/Presets`) and show up in a User bank.

## UI

A 1U rack strip with rack ears: chunky console knobs with coloured caps (stepped knobs have one tick
per detent), a gain-reduction VU needle, and an LED ladder for output and clipping. Six themes: the
five Faderhouse console schemes (Studio Grey, Black Session, Cream Classic, Midnight, Daylight) plus
Scope's LED Rack. Theme and window size are saved with the session; the window scales from 60% to 200%.

## Decisions taken for v0.1

- Name: Iron Bus (working name: `PRODUCT_NAME` in CMakeLists.txt and the name plate in the editor).
- Stepped attack, release, ratio and SC HPF, like the classic console centre-section compressor.
- Stereo link is a continuous 0 to 100% control (default 100%) rather than an on/off switch.
- Tape and Clip are out by default; Iron and Glue are in, so the plugin "does something" when inserted.
- Plugin codes: manufacturer `Irnb`, plugin `Irb1`; installer ids `com.spenceradam98.ironbus`.

## Later

Mid/side mode for the glue, an external sidechain input, a "Faderhouse link" that reads the console's
bus compressor settings, and level-matched A/B.
