# PlatformIO + Horizon-ish Control Surface

You asked for a PlatformIO-friendly build that mirrors the Horizon control surface: this is it. Ten knobs ring the dirt/EQ/limiter spine and two buttons step the discrete states (curve bank + chaos depth). Swap pins to match your wiring loom, but keep the ranges—everything stays clamped to `docs/CONTROL_MAP.csv` so presets and the panel agree.

## TL;DR wiring (edit `src/main.cpp`)
- **Knobs**: Drive, Bias, Env→Drive, GateComp, Pre Tilt, Post Air, Mix, Dirt, Ceiling, Output Trim.
- **Buttons**: one steps the curve bank (0=tanh, 1=cubic, 2=diode, 3=fold); the other steps chaos depth (0–7).
- **Pins**: declared at the top of `src/main.cpp` as `POT_*` and `BUTTON_*`. Change them to match your PCB/panel labels.

## Build + upload
```bash
# Teensy 4.1 default
pio run -e teensy41 -t upload

# Teensy 4.0 alt
pio run -e teensy40 -t upload
```

Serial/MIDI share the USB config, so `pio device monitor` shows your printouts while you wiggle knobs.

## What the mapping math does
- **Drive**: squared pot taper makes a linear pot feel log-ish before mapping to 0..36 dB.
- **Bias / Env→Drive**: bipolar mapping puts 0 at pot center so you can swing negative/positive evenly.
- **Gate/tilt/air/mix/dirt/ceiling/output**: straight linear scaling to the ranges in `CONTROL_MAP.csv` so hardware, docs, and presets stay in lockstep.
- **Curve/chaos buttons**: simple rising-edge detectors with pullups; each tap bumps the index and wraps around. No debounce library needed—short wires, hardware pullups, and the Teensy loop are plenty snappy here.

## Why PlatformIO instead of Arduino
- Reproducible builds and board configs live next to the DSP code.
- Teensy 4.0 + 4.1 envs are already declared in `platformio.ini`; just pick your poison.
- The whole thing is ready for CI/automation if you want to grow it beyond a lab jam box.
