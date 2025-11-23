# DUST PRESS — Dynamic Waveshaper + Envelope Ecology (Matter)

A playable distortion whose curve morphs with input envelope and a chaos ladder. Whisper = silky saturation; push = crackle and
foldback. Parallel clean path, pre/post EQ tilt, and a lookahead limiter keep it musical and safe.

> Reality check (today): the DSP core is awake—gate/comp, envelope-morphed drive, tilt/air EQ, limiter ceiling, mix, and trim
> are wired up. Expect saturation that leans harder as you dig in while the lookahead limiter keeps the top end honest.

## Platform
Teensy 4.x + SGTL5000 (Teensy Audio Library), 44.1 kHz / 128-sample blocks.

## Quick Start
- Open `examples/minimal/minimal.ino` in Arduino + TeensyDuino.
- Select Teensy 4.0/4.1 and upload.
- Feed audio into the Audio Shield; twist `Drive`, `Bias`, and `Env→Drive`.

## Signal Flow (napkin diagram)
```
Input → GateComp → Pre Tilt EQ → Drive → Bias → Curve Bank → Chaos Mod → Dirt → Post Air EQ → Lookahead Limiter (Ceiling) → Mix with Dry → Output Trim
                 ↘────────────── Envelope Follower ───────↗
```
* Envelope follower feeds Env→Drive; curve bank includes tanh/cubic/diode/foldback shapes.
* Chaos sprinkles jitter and crackle probability onto the chosen curve.
* Dirt soft-clips before the limiter so the limiter only catches true peaks.
* AudioMath reality: `src/DustPress.cpp` is fully wired; envelope-morphed drive, tilt/air EQ, limiter ceiling, mix, and trim are live.
* Want the gory DSP path with tuning notes? Hit **[docs/DSP_ANATOMY.md](docs/DSP_ANATOMY.md)** for a block-by-block walkthrough from gate/comp through the limiter.

## Controls
| Control | Parameter | Range | Scale | Default | What it does |
| --- | --- | --- | --- | --- | --- |
| Drive | `drive` | 0..36 dB | log | 12 dB | Input gain into the waveshaper. |
| Bias | `bias` | -1..+1 | linear | 0 | Tilts the shape toward odd/even harmonics. |
| Curve | `curve_index` | 0..3 | int | 0 | 0 = tanh, 1 = cubic, 2 = diode, 3 = foldback. |
| Env→Drive | `env_to_drive` | -12..+12 dB | linear | +6 dB | Envelope modulation amount on Drive. |
| GateComp | `gate_comp` | 0..1 | linear | 0.2 | Low-level gate before shaping. |
| Pre Tilt | `pre_tilt_db_per_oct` | -6..+6 | linear | 0 | Broad tilt before the shaper (tone in). |
| Post Air | `post_air_gain_db` | -6..+6 | linear | 0 | 10 kHz-ish air shelf after shaping. |
| Mix | `mix` | 0..100 % | linear | 50 % | Parallel clean blend. |
| Chaos | `chaos_level` | 0..7 | int | 0 | Adds curve jitter & crackle probability. |
| Dirt | `dirt_amount` | 0..1 | linear | 0.1 | Soft clip before limiter. |
| Ceiling | `limiter_ceiling_db` | -6..0 | linear | -1 | Lookahead limiter ceiling. |
| Output | `out_trim_db` | -12..+6 | linear | 0 | Final trim after mix/limit. |

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
- `examples/` — minimal wiring sketch.
- `presets/` — starter presets JSON.
- `docs/` — control map CSV.

## License
MIT — see `LICENSE`.
