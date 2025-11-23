# DUST PRESS — Dynamic Waveshaper + Envelope Ecology

A playable distortion whose curve morphs with input envelope and a chaos ladder. Whisper = silky saturation; push = crackle and
foldback. Parallel clean path, pre/post EQ tilt, and a lookahead limiter keep it musical and safe.

> Reality check (today): the DSP core is awake—gate/comp, envelope-morphed drive, tilt/air EQ, limiter ceiling, mix, and trim
> are wired up. Expect saturation that leans harder as you dig in while the lookahead limiter keeps the top end honest.

## Platform
Teensy 4.x + SGTL5000 (Teensy Audio Library), 44.1 kHz / 128-sample blocks.

## Quick Start
- Open `examples/minimal/minimal.ino` in Arduino + TeensyDuino for the cleanest wiring demo.
- Or crack open `examples/presets_demo/presets_demo.ino` to see SD/SerialFlash preset loading with JSON clamping.
- Select Teensy 4.0/4.1 and upload.
- Feed audio into the Audio Shield; twist `Drive`, `Bias`, and `Env→Drive`.

## Signal Flow (napkin diagram)
```
Input → GateComp → Pre Tilt EQ → Drive → Bias → Curve Bank → Chaos Mod → Dirt → Post Air EQ → Lookahead Limiter (Ceiling) → Mix with Dry → Output Trim
                 ↘────────────── Envelope Follower ───────↗
```
* Envelope follower feeds Env→Drive; curve bank includes tanh/cubic/diode/foldback shapes.
* Chaos sprinkles jitter and crackle probability onto the chosen curve.
* GateComp only opens/closes the pre-drive gate + makeup stage. Chaos stays on its own knob—no stealth coupling.
* Dirt soft-clips before the limiter so the limiter only catches true peaks.
* AudioMath reality: `src/DustPress.cpp` is fully wired; envelope-morphed drive, tilt/air EQ, limiter ceiling, mix, and trim are live.
* Want the gory DSP path with tuning notes? Hit **[docs/DSP_ANATOMY.md](docs/DSP_ANATOMY.md)** for a block-by-block walkthrough from gate/comp through the limiter.

## Controls
The canonical control map lives in **[docs/USAGE.md](docs/USAGE.md)** (mirrored by `docs/CONTROL_MAP.csv`). Peek there for the full
table of ranges, tapers, and defaults; the presets loader clamps to that sheet so firmware, docs, and CSV all agree. TL;DR:

- Drive/Bias/Curve/Env→Drive/Chaos sculpt the dirt shape and how the envelope leans into it.
- GateComp/Pre Tilt/Post Air shape tone around the shaper.
- Mix/Dirt/Ceiling/Output handle parallel blend, pre-limit soft clip, limiter ceiling, and final trim.

## Build + Upload (TeensyDuino)
1. Install [Arduino IDE](https://www.arduino.cc/en/software) and [TeensyDuino](https://www.pjrc.com/teensy/td_download.html) (match versions).
2. In Arduino, open **File → Examples → dust-press → minimal** (or navigate to `examples/minimal/minimal.ino`).
3. Tools menu: set **Board** to Teensy 4.0/4.1, **USB Type** to what you need (usually Serial + MIDI), **CPU Speed** 600 MHz.
4. Connect the Teensy Audio Shield to the Teensy 4.x (line inputs → audio). Plug in USB.
5. Click **Upload**. If the linker complains about space, remove unused `Serial.print` debugging.
6. On current builds you should hear the full crush path; verify gate/drive/tilt/air/limit/mix moves in Audio System Design Tool scopes or by ear.
7. Jam: feed line-level audio, tweak knobs or send control values over whatever UI you wire up.

## Preset listening tour
| Preset | Expected vibe |
| --- | --- |
| Tape Glow | Gentle tape-ish push. Tanh curve, ~10 dB drive, slight env lift. Leaves transients intact, adds a wispy halo. |
| Velvet Push | Cubic curve with mid drive and mild bias. Feels like a warm console bus leaning into saturation when you dig in. |
| Soot Fuzz | Diode curve, high drive, forward bias. Splattery mid fuzz with some octave grit; keep mix down for parallel hair. |
| Fold Storm | Foldback curve, hot drive, negative bias, chaos 4. Metallic shredding with random crackle—ride the mix to taste. |
| Ghost Ceiling | Any curve, moderate drive, ceiling at -3 dB, mix 70%. Limiter-first vibe that keeps the room breathing while you stomp on it. |

## Folders
- `src/` — core classes (engine, curve bank, envelope follower, EQs, limiter, smoother).
- `examples/` — minimal wiring sketch plus `presets_demo` for SD/SerialFlash preset loading.
- `presets/` — starter presets JSON.
- `docs/` — control map CSV.

## License
MIT — see `LICENSE`.
