# Dust Press native playground

A scrappy, desktop-friendly test harness so you can feed WAVs through the
Dust Press DSP without flashing a Teensy. Think of it as the half-studio-notes,
half-lab-bench bridge on the way to a real VST/AudioUnit build.

## What lives here
- **`dustpress_native` static lib**: wraps the embedded DSP blocks in
  `NativeDustPress`, consuming float buffers instead of `AudioStream` blocks.
- **`dustpress_cli`**: a tiny command-line render tool that mirrors the Teensy
  controls (drive, bias, curve, chaos, tilt, air, limiter, mix) and writes out
  a processed WAV.
- **`dustpress_probe`**: a scrappy telemetry spigot that streams synthetic
  audio through `NativeDustPress` while jittering block sizes, sweeping
  parameters, and hopping across multiple sample rates. It prints limiter
  peaks to stdout and dumps a CSV of envelope/gate/limiter states so you can
  spot non-RT-safe behavior before a DAW does.
- **`third_party/dr_wav.h`**: single-header WAV IO so we avoid extra deps while
  prototyping.

## Build it (CMake, no Arduino required)
```bash
cmake -S native -B native/build
cmake --build native/build
```
That drops `dustpress_cli` and `libdustpress_native.a` into `native/build/`.

## Push a file through the DSP
```bash
native/build/dustpress_cli \
  --in input.wav \
  --out output.wav \
  --drive-db 12 \
  --curve 2 \
  --chaos 0.4 \
  --pre-tilt 1.5 \
  --post-air 4 \
  --mix 0.75
```
Everything is normalized to +/-1 floats; mono files get doubled to stereo.
Limiter ceiling defaults to -1 dBFS to mimic the firmware path.

### Stress-test the DSP like a DAW host
1. Build the native targets (same as above) and run the probe:
   ```bash
   cmake -S native -B native/build
   cmake --build native/build
   native/build/dustpress_probe --seconds 8 --out native_probe.csv
   ```
   It hucks a gnarly test tone through the processor at 44.1/48/96 kHz while
   smearing block sizes all over the place, modulating drive/bias/gate/mix, and
   logging limiter/envelope telemetry.
2. Plot it however you like. A quick-and-dirty Python peek:
   ```python
   import pandas as pd
   df = pd.read_csv("native_probe.csv")
   df[df.sample_rate==48000].plot(x="sample", y=["env","limiter_env","limiter_gain"])
   ```
   Treat it as lab notes: if the limiter gain ever whipsaws or the envelope
   flatlines when blocks jump from 32 to 384 samples, you've got a regression.

## How this maps to the Teensy code
- The signal flow is a straight port of `AudioDustPress::update`—envelope gate,
  drive modulation, tilt, curve bank, air shelf, lookahead limiter, wet/dry.
- Sample rate comes from the input WAV (set via `NativeDustPress::setSampleRate`).
  If you need to test other rates, re-render your source accordingly.
- Parameter setters mirror the embedded names (`setDriveDb`, `setBias`, etc.) so
  future VST/AUv3 wrappers can share the same vocabulary.

## Next experiments
- Swap the CLI for a JUCE `AudioProcessor` to prove out real-time host behavior.
- Add a simple plotter that renders curve/chaos outputs to PNGs alongside the
  WAVs (echoing the docs/DSP anatomy notes).
- Wire up a headless regression test: feed known WAVs through a preset batch and
  assert RMS/peak fingerprints to lock down future changes.
