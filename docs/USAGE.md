# Dust Press Studio Bench Guide

This page is the hands-on guide for learning Dust Press like a bench instrument, not just a bag of parameters. Use it when you want to hear what each control does, load or author presets safely, and wire hardware or MIDI without the docs drifting away from the code.

If you want the under-the-hood DSP walkthrough, jump to **[docs/DSP_ANATOMY.md](./DSP_ANATOMY.md)**. If you want board upload and wiring notes, use **[docs/PLATFORMIO.md](./PLATFORMIO.md)**. If you want to audition the processor on desktop, use **[native/README.md](../native/README.md)**.

## How to use this repo as a teaching bench

Treat the repo like a small lab:

1. Start with `examples/minimal/minimal.ino` or the native CLI so you can hear the engine with as little ceremony as possible.
2. Move to `examples/presets_demo/presets_demo.ino` once you want repeatable starting points and preset recall.
3. Use the control map in this document as the canonical range/default reference.
4. Use **[docs/DSP_ANATOMY.md](./DSP_ANATOMY.md)** after your ears already know what the controls feel like.

Recommended order for a first session:

1. Set `curve=0`, `drive_db=12`, `mix=0.5`, `chaos=0`.
2. Sweep `drive_db` first, then `bias`, then `env_to_drive_db`.
3. Change only one of `curve`, `chaos`, or `dirt` at a time.
4. Use `mix` and `limiter_ceiling_db` to keep comparisons level and musical.

## Bench setup: first 10 minutes

Use a loop you know well: dry drum bus, mono synth riff, bass phrase, or dynamic guitar DI all work. Keep output matched enough that louder does not automatically win.

Suggested starter patch:

| Parameter | Start value | Why |
| --- | --- | --- |
| `curve` | `0` | Tanh gives the easiest "baseline saturation" reference. |
| `drive_db` | `10-12 dB` | Enough to hear movement without instantly flattening everything. |
| `bias` | `0.0` | Start symmetrical so curve differences are easier to hear. |
| `env_to_drive_db` | `+3 dB` | Lets picking/attack teach you the dynamic behavior quickly. |
| `mix` | `0.5-0.7` | Parallel blend keeps transients and makes A/B easier. |
| `chaos` | `0` | Learn the core curves before adding jitter/crackle. |
| `limiter_ceiling_db` | `-1 dB` | Safe default while exploring hotter settings. |

What to listen for:

- `drive_db`: how quickly transients flatten and harmonics bloom.
- `bias`: whether the distortion leans asymmetrical, nasal, or spitty.
- `env_to_drive_db`: whether louder hits get dirtier or cleaner.
- `curve`: whether the breakup feels silky, chewy, raspy, or metallic.
- `mix`: whether you are improving the sound or only making it louder.

## Preset format: what belongs in `presets/presets.json`

`presets/presets.json` is an array of preset objects. Presets may be sparse: if a field is missing, loaders should fall back to the canonical defaults in the control map below.

Minimum useful preset:

```json
{
  "name": "Tape Glow",
  "curve": 0,
  "drive_db": 10,
  "bias": 0.0,
  "env_to_drive_db": 3,
  "mix": 0.6
}
```

Full preset shape supported by the demo/native loaders:

| Field | Type | Range | Default | Notes |
| --- | --- | --- | --- | --- |
| `name` | string | any | `"Untitled"` | Human-readable label |
| `curve` | int | `0..3` | `0` | `0=tanh 1=cubic 2=diode 3=fold` |
| `drive_db` | float | `0..36` | `12.0` | Input gain before shaping |
| `bias` | float | `-1..+1` | `0.0` | Asymmetry / even-odd emphasis |
| `env_to_drive_db` | float | `-12..+12` | `6.0` | Envelope modulation amount |
| `gate_comp` | float | `0..1` | `0.2` | Front-door gate/comp blend |
| `pre_tilt_db_per_oct` | float | `-6..+6` | `0.0` | Tone shaping before curves |
| `post_air_gain_db` | float | `-6..+6` | `0.0` | High-shelf after curves |
| `mix` | float | `0..1` | `0.5` | Wet fraction, not percent |
| `chaos` | float | `0..7` | `0.0` | Curve jitter + crackle depth |
| `dirt` | float | `0..1` | `0.1` | Soft push before limiter |
| `limiter_ceiling_db` | float | `-6..0` | `-1.0` | Lookahead limiter ceiling |
| `out_trim_db` | float | `-12..+6` | `0.0` | Final level trim |

Recommended loader flow:

1. Read `presets/presets.json` from SD, SerialFlash, disk, or host bundle.
2. Parse JSON into a preset struct.
3. Fill missing fields from the canonical defaults in this document.
4. Clamp every value before touching the DSP engine.
5. Translate preset names to setter names where needed, such as `curve` to `curve_index`.

Example loader skeleton:

```cpp
struct Preset {
  std::string name;
  int curve;
  float drive_db;
  float bias;
  float env_to_drive_db;
  float gate_comp;
  float pre_tilt_db_per_oct;
  float post_air_gain_db;
  float mix_0_to_1;
  float chaos;
  float dirt;
  float limiter_ceiling_db;
  float out_trim_db;
};

Preset loadPreset(const JsonObject& obj) {
  Preset p{};
  p.name = obj["name"] | "Untitled";
  p.curve = clamp<int>(obj["curve"] | 0, 0, 3);
  p.drive_db = clamp<float>(obj["drive_db"] | 12.0f, 0.0f, 36.0f);
  p.bias = clamp<float>(obj["bias"] | 0.0f, -1.0f, 1.0f);
  p.env_to_drive_db = clamp<float>(obj["env_to_drive_db"] | 6.0f, -12.0f, 12.0f);
  p.gate_comp = clamp<float>(obj["gate_comp"] | 0.2f, 0.0f, 1.0f);
  p.pre_tilt_db_per_oct = clamp<float>(obj["pre_tilt_db_per_oct"] | 0.0f, -6.0f, 6.0f);
  p.post_air_gain_db = clamp<float>(obj["post_air_gain_db"] | 0.0f, -6.0f, 6.0f);
  p.mix_0_to_1 = clamp<float>(obj["mix"] | 0.5f, 0.0f, 1.0f);
  p.chaos = clamp<float>(obj["chaos"] | 0.0f, 0.0f, 7.0f);
  p.dirt = clamp<float>(obj["dirt"] | 0.1f, 0.0f, 1.0f);
  p.limiter_ceiling_db = clamp<float>(obj["limiter_ceiling_db"] | -1.0f, -6.0f, 0.0f);
  p.out_trim_db = clamp<float>(obj["out_trim_db"] | 0.0f, -12.0f, 6.0f);
  return p;
}
```

## Control map + scaling (canonical)

This table is the single source of truth for parameter ranges, defaults, and expected tapers. `docs/CONTROL_MAP.csv` mirrors it for firmware loaders, spreadsheets, and control-surface tooling.

| Control | Parameter key | Range | Scale/taper | Default | What it teaches |
| --- | --- | --- | --- | --- | --- |
| Drive | `drive` | 0..36 dB | Log/audio taper | 12 dB | How hard you hit the curve |
| Bias | `bias` | -1..+1 | Linear bipolar | 0 | Symmetry vs asymmetry |
| Curve | `curve_index` | 0..3 | Integer steps | 0 | Flavor of nonlinearity |
| Env→Drive | `env_to_drive` | -12..+12 dB | Linear bipolar | +6 dB | Dynamic response to playing |
| GateComp | `gate_comp` | 0..1 | Linear | 0.2 | How much the front door glues/open-closes |
| Pre Tilt | `pre_tilt_db_per_oct` | -6..+6 | Linear | 0 | Which frequencies distort first |
| Post Air | `post_air_gain_db` | -6..+6 | Linear | 0 | How much top-end returns after dirt |
| Mix | `mix` | 0..100 % | Linear | 50 % | Parallel blend discipline |
| Chaos | `chaos_level` | 0..7 | Integer steps | 0 | Movement, crackle, instability |
| Dirt | `dirt_amount` | 0..1 | Linear | 0.1 | Pre-limit chew and density |
| Ceiling | `limiter_ceiling_db` | -6..0 | Linear | -1 | Safety vs aggression |
| Output | `out_trim_db` | -12..+6 | Linear | 0 | Honest level matching |

## What each control sounds like on the bench

### Drive, Bias, Curve

These three teach the core voice of Dust Press.

- `Drive` raises how hard the curve gets hit.
- `Bias` offsets the waveform before shaping, making the breakup more asymmetrical.
- `Curve` swaps the transfer itself.

Quick listening notes:

- `curve=0` (`tanh`): softest knee, easiest "tape-ish" reference.
- `curve=1` (`cubic`): denser mid push, useful for glue and chew.
- `curve=2` (`diode`): bite, asymmetry, more obvious attitude.
- `curve=3` (`fold`): metallic, bright, and easiest to push into weirdness.

### Env→Drive and GateComp

These make Dust Press feel played instead of statically distorted.

- Positive `env_to_drive_db` means louder hits get dirtier.
- Negative `env_to_drive_db` means peaks duck drive and the tone evens out.
- `gate_comp` changes how much low-level material stays present before the shaper wakes up.

Bench trick:

1. Freeze `curve`, `drive_db`, and `mix`.
2. Play soft then hard.
3. Sweep `env_to_drive_db` from `-6` to `+6`.
4. Listen for whether the patch feels more touch-sensitive or more controlled.

### Pre Tilt and Post Air

Think of these as "what do I distort?" versus "what do I restore after distortion?"

- `pre_tilt_db_per_oct` changes which parts of the spectrum hit the shaper harder.
- `post_air_gain_db` brightens or tames the result after shaping.

Rule of thumb:

- If the distortion is woolly, try negative pre-tilt.
- If the distortion is exciting but dull afterward, add a touch of post-air.
- If chaos gets scratchy, lower post-air before assuming the curve is wrong.

### Chaos, Dirt, Mix, Ceiling, Output

This is the "make it exciting without lying to yourself" cluster.

- `chaos` adds jitter and crackle behavior.
- `dirt` adds extra push before the limiter has to catch peaks.
- `mix` restores dry articulation.
- `limiter_ceiling_db` keeps the exploration survivable.
- `out_trim_db` is for honest level matching after you fall in love with something loud.

Practical ranges:

- `chaos 0-2`: movement and texture.
- `chaos 3-5`: audible crackle and instability.
- `chaos 6-7`: special effect territory.

## Preset listening tour

The shipped presets are useful as teaching waypoints, not just presets to keep forever:

| Preset | What to listen for | Teaching move |
| --- | --- | --- |
| `Tape Glow` | Smooth saturation with moderate dynamic push | Raise `pre_tilt_db_per_oct` and hear highs hit the curve first |
| `Velvet Push` | Cubic-style glue and forwardness | Sweep `mix` from `0.3` to `0.8` and compare punch retention |
| `Soot Fuzz` | Heavier diode bite and asymmetry | Move `bias` around zero and hear asymmetry become obvious |
| `Fold Storm` | Foldback + chaos edge | Pull `chaos` down to `1` and hear how much of the violence is the curve vs the chaos ladder |

If you add more presets, try to make each one teach one distinct interaction instead of stacking every dramatic move into every patch.

## Binding hardware controls

Use the canonical ranges above whether you are driving the engine from pots, encoders, MIDI CC, or a desktop host UI.

- Pots and sliders:
  - Use a log-ish mapping for `Drive` so the first half of travel is musically useful.
  - Use centered bipolar controls for `Bias`, `Env→Drive`, and `Pre Tilt`.
  - Use detents or quantization for `Curve` and `Chaos`.
  - Convert preset `mix` with `percent = mix * 100` if your UI is percent-based.
- MIDI CC:
  - Linear params: `value = min + (cc / 127.0) * (max - min)`.
  - Bipolar params: map midpoint to roughly zero.
  - `Drive`: pre-warp the CC response so low-to-mid drive has more resolution than the top end.
- Safety:
  - Always clamp, even if the sender glitches.
  - Snap integer params to valid steps before applying them.

## Linking presets to controls

When a preset loads, push each field into the matching engine parameter:

- `curve` -> `curve_index`
- `drive_db` -> `drive`
- `env_to_drive_db` -> `env_to_drive`
- `mix` -> engine wet mix in `0..1`, UI in `0..100%`

If the preset file and the control surface use different units, convert on load/save instead of letting the DSP layer guess. The engine should receive normalized, clamped values every time.

## Suggested bench exercises

### 1. Same curve, different dynamics

Keep `curve=1`, `mix=0.6`, `chaos=0`.

1. Set `env_to_drive_db=-6`.
2. Play and note the steadier tone.
3. Set `env_to_drive_db=+6`.
4. Compare how much the patch now reacts to attack.

### 2. What bias really does

Keep `drive_db` moderate and compare `bias=-0.4`, `0.0`, and `+0.4` on the same phrase. Do this on `curve=2` and `curve=0`. The difference will teach you why asymmetry is easier to hear on some shapes than others.

### 3. Pre/post tone split

1. Add `pre_tilt_db_per_oct=+2`.
2. Leave `post_air_gain_db=0`.
3. Then reset pre-tilt and try `post_air_gain_db=+2`.
4. Compare "what distorted harder" versus "what got brighter afterward."

### 4. Controlled chaos

Start from `Fold Storm`, then:

1. Lower `chaos` until the patch becomes usable in-context.
2. Raise `mix` until attack comes back.
3. Use `out_trim_db` to loudness-match the original.

That sequence teaches the difference between a fun solo sound and a mixable one.

## Where to go next

- For firmware wiring and upload flow: **[docs/PLATFORMIO.md](./PLATFORMIO.md)**
- For DSP implementation details: **[docs/DSP_ANATOMY.md](./DSP_ANATOMY.md)**
- For desktop rendering, preset round-tripping, and plugin notes: **[native/README.md](../native/README.md)**

Use this document as the "what should I try and what should it teach me?" page. Use the other docs once you need implementation detail.
