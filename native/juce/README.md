# Dust Press JUCE plugin (VST3 playground)

This is the host-facing mule that mirrors the control map in [`docs/USAGE.md`](../../docs/USAGE.md).
It is purpose-built for VST3 and wraps the exact DSP the Teensy runs—no AU fork,
no side quest. Treat it like a studio notebook sketch you can automate in a DAW
before flashing hardware.

## Build/Install (single path, same as `native/plugin`)
1. Fast path (mirrors the Horizon-ish workflow):
   ```bash
   cmake --preset native-plugin-release
   cmake --build --preset native-plugin-release --config Release
   ```
   - The preset flips on the plugin target, fetches JUCE, and lets JUCE fetch the Steinberg VST3 SDK for you.
   - If `native/.juce-kit` already exists (from `tools/bootstrap_juce.sh` or a previous preset run), the build reuses it instead of refetching.
2. Steering your own JUCE install? Point CMake at it:
   ```bash
   cmake -S native -B native/build \
     -DDUSTPRESS_BUILD_PLUGIN=ON \
     -DDUSTPRESS_FETCH_JUCE=OFF \
     -DCMAKE_PREFIX_PATH=/path/to/JUCE
   cmake --build native/build --target DustPressPlugin
   ```
   - A repo-local JUCE install at `native/.juce-kit` works here too—skip the prefix flag and the plugin CMake will auto-sniff it.
3. CMake drops a VST3 bundle under `native/build/plugin-release/DustPressPlugin_artefacts/` (or your custom build dir). Point your DAW at the VST3 output or copy it into your system plug-ins directory.

## Parameter map (automation IDs → ranges)
These mirror [`docs/USAGE.md`](../../docs/USAGE.md) and the `native/plugin` target exactly—no hidden tapers, no AU-only quirks.

- **Drive (0–36 dB, log taper, default 12 dB)** → `setDriveDb`
- **Bias (-1..+1)** → `setBias`
- **Curve (choices tanh/cubic/diode/fold)** → `setCurveIndex`
- **Chaos (0–7 steps)** → `setChaos`
- **Env→Drive (-12..+12 dB, default +6)** → `setEnvToDriveDb`
- **GateComp (0..1, default 0.2)** → `setGateComp`
- **Pre Tilt (-6..+6 dB/oct)** → `setPreTilt`
- **Post Air (-6..+6 dB)** → `setPostAir`
- **Dirt (0..1, default 0.1)** → `setDirt`
- **Limiter Ceiling (-6..0 dBFS, default -1)** → `setCeiling`
- **Output Trim (-12..+6 dB)** → `setOutputTrimDb`
- **Mix (0–100 %, default 50 %) → scaled to `setMix`**

## Host behavior notes
- `prepareToPlay` pushes the host sample rate into the DSP, resets the lookahead buffers, and sets latency to the limiter delay so PDC stays honest.
- `processBlock` defers MIDI, keeps your channel count tidy, and updates parameters every block so automation stays tight.
- State/presets ride through `AudioProcessorValueTreeState`, so DAW sessions recall exactly what you dialed.

That’s it—one preset path, JUCE + VST3 SDK fetched on demand, `.juce-kit` cached for later, and zero legacy AU/VST3 split brain.
