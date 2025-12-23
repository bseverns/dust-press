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

## Limiter probe regression (aka "catch the gremlins before the DAW does")

There's now a headless regression sniff test for the native limiter + envelope path. It drives the baked-in `dustpress_probe` harness with fixed seeds, chews on the CSV telemetry, and compares a few fingerprints against a checked-in baseline (RMS/peaks for the envelopes, limiter gain excursions, output peak). The goal: make DSP experiments fearless *and* catch drift before it sneaks into a DAW bounce.

### Run it locally

```bash
# Build the native probe once
cmake -S native -B native/build
cmake --build native/build --target dustpress_probe

# Reuse the baked seeds + 4-second stimulus at 44.1/48/96 kHz
python tools/dustpress_probe_regression.py

# Keep the CSV around if you want to eyeball the telemetry later
python tools/dustpress_probe_regression.py --keep-csv

# Or park it somewhere specific (useful for CI artefacts)
python tools/dustpress_probe_regression.py --csv-out native/build/probe_telemetry.csv
```

If you're intentionally reshaping the DSP, regenerate the baseline with intent and a commit message to match:

```bash
python tools/dustpress_probe_regression.py --update-baseline
```

The baseline lives in `tools/dustpress_probe_baseline.json` and records the probe runtime plus per-sample-rate stats. Seeds live in `native/src/probe.cpp`, so everyone runs the same gauntlet.

The script keeps things lean—streaming RMS/peak math instead of hoarding millions of samples—and screams if any sample rate drifts past the tolerance window.

### CI glue

GitHub Actions runs the same probe on every push/PR (Linux runner) via the `DustPress probe regression` job. We also publish the raw CSV as an artefact so you can diff real telemetry when something goes sideways. If the limiter or envelope math wanders, the workflow goes red before a DAW host ever sees it.
