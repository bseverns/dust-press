# DUST PRESS — Dynamic Waveshaper + Envelope Ecology (Matter)

A playable distortion whose curve morphs with input envelope and a chaos ladder. Whisper = silky saturation; push = crackle and foldback.
Parallel clean path, pre/post EQ tilt, and a lookahead limiter keep it musical and safe.

## Signal path (sketchbook version)
- Split into a clean safety lane and a dirty lane.
- Dirty lane hits a tilt EQ pivoted around 1.2 kHz so you can lean bright or dark before the shaper.
- Envelope follower tugs drive in dB—play softer for silk, harder for splatter.
- CurveBank picks a flavor (tanh, sine fold, cubic, hard clip) and SoftSaturation sprinkles extra dirt with a blend knob.
- AirEQ lifts the ultra-highs, then a tiny lookahead limiter catches peaks before they leave the nest.
- Mix knob crossfades back toward clean if you need less chaos.

## Platform
Teensy 4.x + SGTL5000 (Teensy Audio Library), 44.1 kHz / 128-sample blocks.

## Quick Start
- Open `examples/minimal/minimal.ino` in Arduino + TeensyDuino.
- Select Teensy 4.0/4.1 and upload.
- Feed audio into the Audio Shield; twist `Drive`, `Bias`, and `Env→Drive`.

## Folders
- `src/` — core classes (engine, curve bank, envelope follower, EQs, limiter, smoother).
- `examples/` — minimal wiring sketch.
- `presets/` — starter presets JSON.
- `docs/` — control map CSV.

## License
MIT — see `LICENSE`.