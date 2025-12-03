# Offline curve clip harness

We keep audio binaries out of git, but you can regenerate the chaos-per-curve clips on demand with a single command. Drop any dry 16-bit WAV riff into the repo and run the harness to spit the expected filenames under `docs/media/` so the anatomy doc links light up locally.

## Quick use

```bash
python tools/curve_clip_harness.py path/to/dry_riff.wav
```

That renders `tanh/cubic/diode/foldback` variants at chaos 1.0 / 3.0 / 6.0 into:

- `docs/media/tanh_chaos-low.wav`
- `docs/media/tanh_chaos-mid.wav`
- `docs/media/tanh_chaos-high.wav`
- `docs/media/cubic_chaos-low.wav`
- `docs/media/cubic_chaos-mid.wav`
- `docs/media/cubic_chaos-high.wav`
- `docs/media/diode_chaos-low.wav`
- `docs/media/diode_chaos-mid.wav`
- `docs/media/diode_chaos-high.wav`
- `docs/media/foldback_chaos-low.wav`
- `docs/media/foldback_chaos-mid.wav`
- `docs/media/foldback_chaos-high.wav`

## Tunable bits

- `--drive-db` (default 12 dB) to lean harder into the curves if your sample sits cold.
- `--bias`/`--dirt` to pre-shape before the curve bank.
- `--chaos-levels` to sweep different depths; names fall back to the numeric value if it is not 1/3/6.
- `--output-dir` if you want to stash renders somewhere other than `docs/media/`.

## How faithful is it?

The math is a 1:1 port of `src/CurveBank.cpp` (bias + dirt + chaos jitter/crackle) so the offline renders sound like firmware. No JUCE/PlatformIO baggage—just Python stdlib and a dash of punk intent.
