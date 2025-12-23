# DUST PRESS — Dynamic Waveshaper + Envelope Ecology

A playable distortion whose curve morphs with input envelope and a chaos ladder. Whisper = silky saturation; push = crackle and
foldback. Parallel clean path, pre/post EQ tilt, and a lookahead limiter keep it musical and safe.

> Reality check (today): the DSP core is awake—gate/comp, envelope-morphed drive, tilt/air EQ, limiter ceiling, mix, and trim
> are wired up. Expect saturation that leans harder as you dig in while the lookahead limiter keeps the top end honest.

## Platform
Currently, this codebase lives as a set of VST3 plugins. Making sure this sounds right is the first priority.
Eventual hardware (what it was made for): Teensy 4.x + SGTL5000 (Teensy Audio Library), 44.1 kHz / 128-sample blocks. Because hardware feels cool.

## Quick Start
- Hardware jam: crack open `examples/minimal/minimal.ino` or `examples/presets_demo/presets_demo.ino` in Arduino/TeensyDuino (or the matching PlatformIO env) and blast it to a Teensy 4.0/4.1. Pin maps, upload checklists, and control-surface wiring now live in **[docs/PLATFORMIO.md](docs/PLATFORMIO.md)** so this page can stay about sound-shaping intent.
- Feed audio into the Audio Shield; twist `Drive`, `Bias`, and `Env→Drive` to hear the envelope-morphed dirt wake up.

## Signal Flow (napkin diagram)
```
Input → GateComp → Pre Tilt EQ → Drive → Bias → Curve Bank → Chaos Mod → Dirt → Post Air EQ → Lookahead Limiter (Ceiling) → Mix with Dry → Output Trim
                 ↘────────────── Envelope Follower ───────↗
```
### DSP block cheat sheet (who owns what)
Here’s the “open the hood” version of the napkin flow so you can jump from knob to live code without spelunking. Each stop lists the class wired in `src/DustPress.cpp`, the setters that matter, and where to read the tuning riffs in **[docs/DSP_ANATOMY.md](docs/DSP_ANATOMY.md)**.

| Block order | Class & member | Key setters | Tuning notes |
| --- | --- | --- | --- |
| Envelope tap → gate/comp | `EnvelopeFollower env` feeds the gate/open calc; gate depth lives on `setGateComp` and makeup gain is auto-set inside it. | `EnvelopeFollower::setAttackMs`, `EnvelopeFollower::setReleaseMs`, `AudioDustPress::setGateComp` | “Gate/Comp front-door” and “Drive + envelope modulation” explain the envelope math and gate makeup. |
| Drive (modded by env) | `ParamSmoother driveSmoother` shapes `setDriveDb` jumps before multiplying by the envelope-driven boost from `setEnvToDriveDb`. | `ParamSmoother::setTimeMs`, `ParamSmoother::setTarget`, `AudioDustPress::setDriveDb`, `AudioDustPress::setEnvToDriveDb` | See “Drive + envelope modulation” for how the modulated dB sum is converted and why the smoother exists. |
| Pre Tilt EQ | `TiltEQ tilt` with per-channel state to keep stereo honest. | `TiltEQ::setSlope`, `AudioDustPress::setPreTilt` | “Pre Tilt EQ” block for dB/oct tricks before the shaper. |
| Drive curves, bias, chaos, dirt | `CurveBank curves` handles bias offsets, dirt pre-push, chaos jitter/crackle, and the curve selector. | `CurveBank::setIndex`, `CurveBank::setBias`, `CurveBank::setDirt`, `CurveBank::setChaos`, `AudioDustPress::setCurveIndex`, `AudioDustPress::setBias`, `AudioDustPress::setDirt`, `AudioDustPress::setChaos` | “Bias, Chaos, Dirt, and Curve Bank” dives into each curve + chaos range suggestions. |
| Post Air EQ | `AirEQ air` with stereo channel states. | `AirEQ::setGainDb`, `AudioDustPress::setPostAir` | “Post Air EQ” covers when to lift or pull the shelf after distortion. |
| Limiter | `LimiterLookahead limiter` clamps post-air peaks with a tiny buffer. | `LimiterLookahead::setCeilingDb`, `LimiterLookahead::setLookaheadMs`, `LimiterLookahead::setSampleRate`, `AudioDustPress::setCeiling` | “Lookahead limiter” spells out ceiling targets and how hard to lean on it. |
| Mix + trim | Parallel blend and final level happen in `update` via `mix/dryMix` and `outputTrimLin`. | `AudioDustPress::setMix`, `AudioDustPress::setOutputTrimDb` | “Mix + Output trim” reminds you to rebalance after gate/drive tweaks. |

* Envelope follower feeds Env→Drive; curve bank includes tanh/cubic/diode/foldback shapes.
* Chaos sprinkles jitter and crackle probability onto the chosen curve.
* GateComp only opens/closes the pre-drive gate + makeup stage. Chaos stays on its own knob—no stealth coupling.
* Dirt soft-clips before the limiter so the limiter only catches true peaks.
* AudioMath reality: `src/DustPress.cpp` is fully wired; envelope-morphed drive, tilt/air EQ, limiter ceiling, mix, and trim are live.
* Want the gory DSP path with tuning notes? Hit **[docs/DSP_ANATOMY.md](docs/DSP_ANATOMY.md)** for a block-by-block walkthrough from gate/comp through the limiter.

## Controls
Single source of truth: the control map table in **[docs/USAGE.md](docs/USAGE.md)**. `docs/CONTROL_MAP.csv` mirrors it for spreadsheet
nerds, but the markdown owns the ranges, tapers, and defaults. Snapshot:

- Drive/Bias/Curve/Env→Drive/Chaos sculpt the dirt shape and how the envelope leans into it.
- GateComp/Pre Tilt/Post Air shape tone around the shaper.
- Mix/Dirt/Ceiling/Output handle parallel blend, pre-limit soft clip, limiter ceiling, and final trim.

## Build + Upload (TeensyDuino)
Arduino path still lives here—fire up the IDE + TeensyDuino, aim at Teensy 4.0/4.1, and send one of the example sketches. When you need board setup screenshots, wiring pinouts, or “did I pick the right core version?” reminders, jump straight to **[docs/PLATFORMIO.md](docs/PLATFORMIO.md)**.

## PlatformIO + Horizon-ish control surface
Same vibe, different toolchain: the repo ships with a PlatformIO preset and Horizon-flavored control surface map. Use it if you want serial + MIDI on the same USB pipe and a structured pinout, then hit **[docs/PLATFORMIO.md](docs/PLATFORMIO.md)** for the full wiring atlas and upload walk-through.

CLI reminder:

```bash
pio run -e teensy41 -t upload    # or -e teensy40 if that’s your board
pio device monitor               # watch serial/MIDI chatter
```

## Desktop-native playground (first stop toward VST land)
Need to audition Dust Press off-hardware? There’s now a minimal CMake build under `native/` that wraps the DSP blocks in a float-buffer processor and a CLI renderer. Build it, fling a WAV through, and keep hacking:

```bash
cmake -S native -B native/build
cmake --build native/build
native/build/dustpress_cli --in input.wav --out output.wav --drive-db 10 --curve 2 --mix 0.8
```

The flow mirrors `AudioDustPress::update` (envelope gate → drive mod → tilt → curve bank → air → limiter → mix). See `native/README.md` for the punkier rundown.

Need the JUCE/VST3 plugin to mirror Horizon’s host workflow? Hop to **[native/README.md](native/README.md#jucevst3-plugin-build-single-source-of-truth)** for the single, canonical build path—the legacy `native/juce` stub is gone, and everything funnels through `native/plugin` now.

## Preset listening tour
Five starter presets live in `presets/presets.json`; they’re clamped to the canonical table in **[docs/USAGE.md](docs/USAGE.md)** so defaults never drift. Think tape-ish sheen (Tape Glow), console-ish warmth (Velvet Push), diode fuzz grime (Soot Fuzz), foldback chaos (Fold Storm), and limiter-forward safety net (Ghost Ceiling). Tweak them or roll your own and let the control map keep you honest.

## Folders
- `src/` — core classes (engine, curve bank, envelope follower, EQs, limiter, smoother).
- `examples/` — minimal wiring sketch plus `presets_demo` for SD/SerialFlash preset loading.
- `presets/` — starter presets JSON.
- `docs/` — control map CSV.
- `tools/` — host-side helpers like the chaos clip harness (see `tools/README.md`).
- `native/` — desktop CMake build + CLI for tossing WAVs through the DSP.

## License
MIT — see `LICENSE`.
