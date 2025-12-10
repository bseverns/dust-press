# DustPress JUCE plugin target

This is the scrappy, canonical VST3 skin around `NativeDustPress`. The old
`native/juce` target is retired; point every plugin experiment at this
directory. It mirrors the control ranges from [`docs/USAGE.md`](../../docs/USAGE.md), speaks stereo in/out, and keeps the DSP identical to the firmware path. Presets come straight from [`presets/presets.json`](../../presets/presets.json) so DAW state matches the Teensy demos.

## Build/Install
### Prereqs (macOS)
- **CMake 3.21+** so presets actually run (Catalina ships too old). Quick fix: `brew install cmake`.
- The shipped preset asks for **Ninja**; grab it with `brew install ninja`.
- No Ninja, but you trust Xcode? Configure with `-G Xcode` (or a manual `cmake -S native -B native/build -G Xcode ...`) and drive the build from there.

1. Use the preset if you just want the VST3 and don't feel like herding dependencies:
   ```bash
   cmake --preset native-plugin-release
   cmake --build --preset native-plugin-release --config Release
   ```
   - The preset flips on the plugin target, fetches JUCE, installs it into `native/.juce-kit` for reuse, and lets JUCE fetch the Steinberg VST3 SDK for you—same Horizon-flavored bootstrapper, now the only path.
2. Prefer hand-managed deps? You can still point at your existing JUCE install. `DUSTPRESS_BUILD_PLUGIN=ON` is the only plugin toggle; the legacy `DUSTPRESS_BUILD_JUCE_PLUGIN` option has been excised:
   ```bash
   cmake -S native -B native/build \
     -DDUSTPRESS_BUILD_PLUGIN=ON \
     -DDUSTPRESS_FETCH_JUCE=OFF \
     -DCMAKE_PREFIX_PATH=/path/to/JUCE
   cmake --build native/build --target DustPressPlugin
   ```
3. CMake drops a VST3 bundle under `native/build/plugin-release/DustPressPlugin_artefacts/` (or your custom build dir). Point your DAW at the VST3 output folder or copy it into your system plug-ins directory.

> CI note: `.github/workflows/plugin-build.yml` exercises the preset on macOS, Windows, and Linux, then tars up the artefacts. Signing/notarization hooks are stubbed so you can drop in your own cert flow.

## Parameter map
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
- **Mix (0–100 %, default 50 %)** → scaled to `setMix`

## Host behavior notes
- `prepareToPlay` pushes the host sample rate into the DSP, resets the lookahead buffers, and sets latency to the limiter delay so PDC stays honest.
- `processBlock` defers MIDI, keeps your channel count tidy, and updates parameters every block so automation stays tight.
- State/presets are serialized through `AudioProcessorValueTreeState`, so DAW sessions recall exactly what you dialed.

Go get weird: automate Chaos into the limiter ceiling, or map Drive to a MIDI fader and crank until it breathes.
