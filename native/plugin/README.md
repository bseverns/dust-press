# DustPress JUCE plugin target

This is the scrappy VST3 skin around `NativeDustPress`. It mirrors the control ranges from [`docs/USAGE.md`](../../docs/USAGE.md), speaks stereo in/out, and keeps the DSP identical to the firmware path.

## Build/Install
1. Install JUCE and make sure CMake can see it (via `CMAKE_PREFIX_PATH` or your package manager).
2. Configure the native build with the plugin target flipped on:
   ```bash
   cmake -S native -B native/build \
     -DDUSTPRESS_BUILD_PLUGIN=ON \
     -DCMAKE_PREFIX_PATH=/path/to/JUCE
   cmake --build native/build --target DustPressPlugin
   ```
3. CMake drops a VST3 bundle under `native/build/DustPressPlugin_artefacts/`. Point your DAW at the VST3 output folder or copy it into your system plug-ins directory.

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
