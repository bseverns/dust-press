# Dust Press JUCE preview plugin (control-map mirror)

This is the host-facing test mule: a JUCE VST3/AU that mirrors the exact control map in [`docs/USAGE.md`](../../docs/USAGE.md) so you
can automate and audition Dust Press in a DAW before committing to firmware.
Think of it as a studio notebook sketch that keeps the DSP honest.

## Parameters (automation IDs → ranges → defaults)
These IDs are the automation keys—copy/paste them into your DAW lane and trust
that the ranges match the canonical control table. Defaults mirror the control
map so the plugin opens in the same posture as the embedded build.

| ID (automation key) | Host label | Range | Default | Notes |
| --- | --- | --- | --- | --- |
| `drive` | Drive | 0..36 dB (log taper) | 12 dB | Pre-shaper gain |
| `bias` | Bias | -1..+1 | 0 | Offsets the curve for even/odd | 
| `curve_index` | Curve | 0..3 (tanh/cubic/diode/fold) | 0 | Integer steps |
| `env_to_drive` | Env→Drive | -12..+12 dB | +6 dB | Envelope to drive modulation |
| `gate_comp` | Gate/Comp | 0..1 | 0.2 | Pre-drive gate + makeup |
| `pre_tilt_db_per_oct` | Pre Tilt | -6..+6 dB/oct | 0 | Tone into the shaper |
| `post_air_gain_db` | Post Air | -6..+6 dB | 0 | 10 kHz air shelf post-shaper |
| `mix` | Mix | 0..1 (0–100%) | 0.5 | Parallel blend |
| `chaos_level` | Chaos | 0..7 | 0 | Curve jitter/crackle steps |
| `dirt_amount` | Dirt | 0..1 | 0.1 | Soft clip pre-limiter |
| `limiter_ceiling_db` | Ceiling | -6..0 dBFS | -1 | Lookahead limiter ceiling |
| `out_trim_db` | Output | -12..+6 dB | 0 | Final trim |

## Editor layout (why the knobs land where they do)
The GUI mirrors the control map flow so muscle memory from hardware sticks:

- **Row 1:** Drive → Bias → Env→Drive (how hard you hit the curve and how the envelope leans on it).
- **Row 2:** Curve + Chaos neighbors, then Gate/Comp → Pre Tilt → Post Air for tone staging.
- **Row 3:** Dirt → Mix → Ceiling → Output for the cleanup + safety net.

Each knob double-clicks back to its default so you can snap back to the control
map baseline after experiments.

## Building the plugin
We keep it opt-in so CI doesn’t have to fetch JUCE unless you mean it.

```bash
cmake -S native -B native/build -DDUSTPRESS_BUILD_JUCE_PLUGIN=ON
cmake --build native/build --target dustpress_plugin
```

That pulls JUCE via `FetchContent`, builds the VST3/AU in `native/build`, and
links against the same `dustpress_native` DSP core as the CLI. Drop the VST3
into your DAW and automate by the IDs above. If you swap the curve bank or
control ranges later, update this table first—the DAW is the harshest truth
mirror we have.
