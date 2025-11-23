# Dust Press DSP Anatomy — Gate/Comp to Limiter

This is the studio-notebook walkthrough of the DSP spine. Start at the front gate/compressor, ride through the tilt + drive + curve rack, and land in the limiter with a mix-out trim. Inline code refs point to the live implementation so you can tweak with confidence.

## Big-picture flow

1. **Gate/Comp + makeup** → **Drive + Env modulation** → **Pre Tilt EQ** → **Bias/Chaos/Dirt + Curve** → **Post Air EQ** → **Limiter** → **Mix + Output trim**.
2. Envelope follower is tapped early, driving the gate/comp openness and the env-to-drive bump before the shaper.

```mermaid
flowchart LR
    subgraph Frontend[Gate/Comp Front Door]
        env[Envelope Follower]
        gate_comp[gate_comp]
        env --> gateGain[gate gain]
        gate_comp --> gateMakeup[compMakeup = 1 + 0.2 * gate_comp]
        gateMakeup --> gateGain
        input[[Input]] --> gateGain
    end

    subgraph Drive[Drive + Env Mod]
        driveDb[drive_db smoother]
        env --> envToDrive[env_to_drive]
        driveDb --> driveGain[drive gain]
        envToDrive --> driveGain
    end

    subgraph Tilt[Pre Tilt EQ]
        tilt[pre_tilt_db_per_oct]
    end

    subgraph Curves[Bias / Chaos / Dirt + Curve Bank]
        bias[bias]
        dirt[dirt]
        chaos[chaos]
        curve[Curve selector]
        bias --> curve
        dirt --> curve
        chaos --> curve
    end

    subgraph Post[Post Air + Limiter + Mix]
        air[post_air_gain_db]
        limiter[Limiter ceiling]
        mix[mix + dryMix]
        trim[out_trim_db]
    end

    gateGain --> driveGain
    driveGain --> tilt
    tilt --> curve
    curve --> air
    air --> limiter
    limiter --> mix
    mix --> trim[[Output]]
```

## Gate/Comp front-door
- `setGateComp` pushes `gateComp` into two places: the gate weighting and a small makeup gain `compMakeup` (`1 + gateComp * 0.2`) to keep tails audible when the gate opens.
- In `update`, `gateGain` lerps between unity and the squared envelope (`envVal^2`) so transients open faster while low-level content stays tucked.
- **Tuning moves:**
  - Raising `gate_comp` lifts tails and also auto-raises makeup; if you crank drive afterwards, back off output trim to keep the limiter relaxed.
  - Leaving `gate_comp` near zero gives you raw pick attack with minimal glue; set `env_to_drive` higher to get dynamic saturation instead of gating.

## Drive + envelope modulation
- `setDriveDb` feeds a smoother so jumps don’t click; the modulated drive at runtime is `driveSmoother.process() + envVal * envDriveAmt`, then converted to linear gain.
- **Tuning moves:**
  - Positive `env_to_drive` makes louder hits push harder into the curves; negative values duck the drive on peaks for steadier tone.
  - Keep drive moderate when using heavy chaos or diode/foldback shapes—otherwise the limiter will work overtime.

## Pre Tilt EQ
- `setPreTilt` sets the slope in dB/oct before shaping, changing which partials hit the curve harder.
- **Tuning moves:**
  - Tilt down (-dB/oct) to keep low-end from smearing the shaper; tilt up to spotlight air before distortion.
  - Pair with `post_air_gain_db` for pre/post tone sculpting: pre emphasizes what distorts, post restores sheen.

## Bias, Chaos, Dirt, and Curve Bank
- Bias offsets the wave before any shaping, changing odd/even emphasis (`curves.setBias`). Dirt adds magnitude-proportional push so curves bite sooner (`curves.setDirt`). Chaos raises both pre-curve jitter and post-curve crackle probability (`curves.setChaos`).
- The curve switch uses `index & 0x3` to pick tanh, cubic, diode, or foldback transfers; chaos injects tiny LCG jitter and occasional clipped spikes for grit.
- **Tuning moves:**
  - More bias = asymmetry; try positive bias with diode for forward mid bite, negative with cubic for spongy sag.
  - Dirt thickens hits before the curve; start around 0.1–0.2 when running low drive for “chewy” push.
  - Chaos 0–2 sprinkles movement; 3–5 gets audible crackle; 6–7 is glitch-land—lower `drive` or raise `mix` to keep it musical.

## Post Air EQ
- `setPostAir` adjusts a high-shelf after the shaper (`air.process`), letting you restore or tame highs the shaper exaggerated.
- **Tuning moves:**
  - If chaos crackle feels sharp, pull air down a few dB; if foldback sounds muffled, push air up.

## Lookahead limiter
- `setCeiling` wires straight into `limiter.setCeilingDb`; limiter clamps after post-air, catching peaks from curves/chaos/dirt.
- **Tuning moves:**
  - Set ceiling around -1 to -3 dB for “studio safe” levels; raise dirt/drive until limiter just kisses peaks for polished grit.
  - For raw crunch, set ceiling near 0 and manage level with `out_trim_db` instead of hammering the limiter.

## Mix + Output trim
- Wet/dry mix happens after limiting: `mixed = wet * mix + dry * dryMix`, then `outputTrimDb` scales the result.
- **Tuning moves:**
  - Parallel blend keeps transient snap while letting chaos/foldback live underneath.
  - Use trim to re-level after gate_makeup/drive changes so presets swap consistently.

## Quick tweak recipes
- **Punchy glue:** gate_comp 0.3, env_to_drive +6 dB, cubic curve, dirt 0.1, ceiling -2 dB, mix 60%.
- **Shattered glass:** gate_comp 0.1, drive hot, bias -0.3, foldback, chaos 5, air +2 dB, mix 30% to taste.
- **Tape lean:** gate_comp 0.25, drive mid, bias +0.1, tanh, tilt -1 dB/oct, ceiling -1 dB, mix 70%.
