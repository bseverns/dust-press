# Dust Press Usage Guide (Presets + Control Map)

This is the zine-style cheat sheet for loading presets, decoding what each field means, and wiring hardware or MIDI so the knobs actually move the engine. Take it, tweak it, make noise. For a deep dive on the DSP blocks and how to tune them, peep **[docs/DSP_ANATOMY.md](./DSP_ANATOMY.md)**.

## Loading `presets/presets.json`
- File is an array of preset objects. Each entry can include:
  - `name` (string): label shown to humans.
  - `curve` (int 0–3): index into the curve bank (0=tanh, 1=cubic, 2=diode, 3=fold).
  - `drive_db` (float): input gain before shaping, in dB.
  - `bias` (float -1..+1): offsets the wave for even/odd emphasis.
  - `env_to_drive_db` (float -12..+12): envelope modulation amount applied to drive (dB).
  - `mix` (float 0..1): 0 is dry, 1 is fully crushed; `presets.json` stores it as a 0–1 fraction while the control map lists 0–100%.
  - `chaos` (int 0–7, optional): adds curve jitter and crackle probability; missing means default of 0.
- Recommended loader flow (pseudo-C++):
  1. Read file from `presets/presets.json` into a buffer (SPI flash/SD is fine).
  2. Parse JSON into a struct with the fields above.
  3. For any missing field, fall back to the defaults in the control table below.
  4. Clamp values to the ranges below before pushing into the DSP engine.

```cpp
struct Preset {
  std::string name;
  int curve;
  float drive_db;
  float bias;
  float env_to_drive_db;
  float mix_0_to_1;
  int chaos;
};

Preset loadPreset(const JsonObject& obj) {
  Preset p{};
  p.name = obj["name"] | "Untitled";
  p.curve = clamp<int>(obj["curve"] | 0, 0, 3);
  p.drive_db = clamp<float>(obj["drive_db"] | 12.0f, 0.0f, 36.0f);
  p.bias = clamp<float>(obj["bias"] | 0.0f, -1.0f, 1.0f);
  p.env_to_drive_db = clamp<float>(obj["env_to_drive_db"] | 0.0f, -12.0f, 12.0f);
  p.mix_0_to_1 = clamp<float>(obj["mix"] | 0.5f, 0.0f, 1.0f);
  p.chaos = clamp<int>(obj["chaos"] | 0, 0, 7);
  return p;
}
```

## Control map + scaling (from `docs/CONTROL_MAP.csv`)
Use these ranges whether you map pots, encoders, or MIDI. “Scale” hints at taper: `log` favors subtlety near zero; `int` means step quantization.

| Control | Parameter key | Range | Scale/taper | Default | Notes |
| --- | --- | --- | --- | --- | --- |
| Drive | `drive` | 0..36 dB | Log/audio taper | 12 dB | Input gain to shaper |
| Bias | `bias` | -1..+1 | Linear bipolar | 0 | Odd/even emphasis |
| Curve | `curve_index` | 0..3 | Integer steps | 0 | 0=tanh 1=cubic 2=diode 3=fold |
| Env→Drive | `env_to_drive` | -12..+12 dB | Linear bipolar | +6 dB | Envelope mod of drive |
| GateComp | `gate_comp` | 0..1 | Linear | 0.2 | Low-level gate before shaper |
| Pre Tilt | `pre_tilt_db_per_oct` | -6..+6 | Linear | 0 | Tone before shaper |
| Post Air | `post_air_gain_db` | -6..+6 | Linear | 0 | Air shelf after shaper (10 kHz) |
| Mix | `mix` | 0..100 % | Linear | 50 % | Parallel clean blend |
| Chaos | `chaos_level` | 0..7 | Integer steps | 0 | Adds curve jitter & crackle prob |
| Dirt | `dirt_amount` | 0..1 | Linear | 0.1 | Soft clip pre-limiter |
| Ceiling | `limiter_ceiling_db` | -6..0 | Linear | -1 | Lookahead limiter ceiling |
| Output | `out_trim_db` | -12..+6 | Linear | 0 | Final trim |

## Binding hardware controls
- **Pots/sliders**: apply the scale column.
  - Log pot for `Drive` so most travel lives in the first 12 dB, reserve the last quarter for the 20–36 dB slam.
  - Bipolar controls (`Bias`, `Env→Drive`, `Pre Tilt`) should center at 0. Use deadband around 0.0 to avoid drift.
  - `Mix`: if your UI shows percent, convert preset fraction `mix` with `percent = mix * 100`.
  - `Curve` and `Chaos`: detented encoder or discrete switch. Coerce any analog reading to nearest int.
- **MIDI CC**: map 0–127 to the ranges above.
  - Linear params: `value = min + (cc/127.0)*(max-min)`.
  - Bipolar: remap 0–63 to the negative half, 64–127 to positive; or offset so 64 ≈ 0.
  - Log drive: pre-warp by squaring CC (or use MIDI-14bit) before mapping to 0–36 dB.
- **Safety**: always clamp to the min/max in the table, even if firmware glitches or the MIDI sender is rowdy.

## Linking presets to controls
- When a preset loads, push each field into its matching engine parameter (translate `curve`→`curve_index`, `drive_db`→`drive`, etc.).
- For knobs that use different units than the preset file, convert on load/save:
  - `mix` fraction ↔ % UI.
  - `drive_db` is already absolute dB; no extra math besides log scaling for UI.
- If a preset omits a field, inject the default above so the user never sees an undefined state.

Go make it howl, but keep the limiter ceiling honest.
