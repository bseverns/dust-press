#!/usr/bin/env python3
"""
Host-side curve renderer for Dust Press demos.

Drop a dry riff into the repo (16-bit PCM WAV, any channel count), then run this
script to spit out chaos sweeps per curve under docs/media/. We keep the WAVs out
of the repo--this harness is your repeatable, binary-free generator.

The math mirrors the embedded CurveBank (bias, dirt, chaos jitter/crackle) and
lets you set a pre-drive scalar if you want to lean into the folds.
"""
from __future__ import annotations

import argparse
import math
import pathlib
import wave
from typing import Iterable, List, Tuple

# Curve math mirrors src/CurveBank.cpp so the offline render matches firmware.
class CurveBankHost:
    def __init__(self, index: int, bias: float, dirt: float, chaos: float):
        self.index = index
        self.bias = bias
        self.dirt = dirt
        self.chaos = chaos
        self._chaos_state = 1  # LCG seed

    def process_sample(self, x: float) -> float:
        shaped = x + self.bias
        shaped += self.dirt * shaped * shaped * 0.5 * (1.0 if shaped >= 0.0 else -1.0)

        chaos_norm = max(0.0, min(self.chaos, 7.0)) / 7.0
        shaped += self._chaos_sample() * (0.015 * chaos_norm)

        y = shaped
        curve = self.index & 0x3
        if curve == 0:
            y = math.tanh(shaped)
        elif curve == 1:
            y = self._cubic_soft_clip(shaped)
        elif curve == 2:
            y = self._diode_clip(shaped)
        elif curve == 3:
            y = self._foldback(shaped)
        else:
            y = self._hard_clip(shaped)

        return self._apply_crackle(y)

    def process_block(self, samples: List[float]) -> List[float]:
        return [self.process_sample(x) for x in samples]

    @staticmethod
    def _cubic_soft_clip(x: float) -> float:
        x2 = x * x
        return x - (x2 * x) * (1.0 / 3.0)

    @staticmethod
    def _diode_clip(x: float) -> float:
        alpha = 3.5
        if x >= 0.0:
            return 1.0 - math.exp(-alpha * x)
        return -(1.0 - math.exp(alpha * x))

    @staticmethod
    def _foldback(x: float) -> float:
        threshold = 1.0
        magnitude = abs(x)
        if magnitude <= threshold:
            return x
        folded = math.fmod(magnitude - threshold, threshold * 2.0)
        signed_fold = folded if folded <= threshold else (threshold * 2.0 - folded)
        return (1.0 if x >= 0.0 else -1.0) * signed_fold

    @staticmethod
    def _hard_clip(x: float) -> float:
        if x > 1.0:
            return 1.0
        if x < -1.0:
            return -1.0
        return x

    def _chaos_sample(self) -> float:
        self._chaos_state = (self._chaos_state * 1664525 + 1013904223) & 0xFFFFFFFF
        return (float(self._chaos_state >> 9) / 16777216.0) - 1.0

    def _apply_crackle(self, x: float) -> float:
        if self.chaos <= 0.0:
            return x

        chaos_norm = max(0.0, min(self.chaos, 7.0)) / 7.0
        self._chaos_state = (self._chaos_state * 1103515245 + 12345) & 0xFFFFFFFF
        rand01 = float(self._chaos_state & 0x00FFFFFF) / 16777216.0
        probability = 0.01 + chaos_norm * 0.04
        if rand01 < probability:
            spike = float((self._chaos_state >> 8) & 0x00FFFFFF) / 8388608.0 - 1.0
            x = self._hard_clip(x + spike * (0.1 + 0.2 * chaos_norm))
        return x


def read_wav(path: pathlib.Path) -> Tuple[int, List[float]]:
    with wave.open(str(path), "rb") as wf:
        n_channels = wf.getnchannels()
        sample_width = wf.getsampwidth()
        sample_rate = wf.getframerate()
        n_frames = wf.getnframes()
        frames = wf.readframes(n_frames)

    if sample_width != 2:
        raise ValueError("Only 16-bit PCM WAVs are supported for now.")

    import array

    pcm = array.array("h")
    pcm.frombytes(frames)
    samples = [x / 32768.0 for x in pcm]

    if n_channels > 1:
        # Fold down to mono to match the quick doc demo.
        samples = [
            0.5 * (samples[i] + samples[i + 1]) for i in range(0, len(samples), n_channels)
        ]
    return sample_rate, samples


def write_wav(path: pathlib.Path, sample_rate: int, samples: Iterable[float]) -> None:
    clipped = [max(-1.0, min(1.0, x)) for x in samples]
    import array

    pcm = array.array("h", [int(x * 32767.0) for x in clipped])
    with wave.open(str(path), "wb") as wf:
        wf.setnchannels(1)
        wf.setsampwidth(2)
        wf.setframerate(sample_rate)
        wf.writeframes(pcm.tobytes())


def render_curve_set(
    input_path: pathlib.Path,
    output_dir: pathlib.Path,
    drive_db: float,
    bias: float,
    dirt: float,
    chaos_levels: List[float],
) -> None:
    sample_rate, dry = read_wav(input_path)
    drive_lin = math.pow(10.0, drive_db / 20.0)
    driven = [x * drive_lin for x in dry]

    curve_names = ["tanh", "cubic", "diode", "foldback"]
    chaos_labels = {
        1.0: "low",
        3.0: "mid",
        6.0: "high",
    }

    output_dir.mkdir(parents=True, exist_ok=True)

    for curve_index, curve_name in enumerate(curve_names):
        for chaos in chaos_levels:
            bank = CurveBankHost(curve_index, bias=bias, dirt=dirt, chaos=chaos)
            processed = bank.process_block(driven)
            label = chaos_labels.get(chaos, f"{chaos:g}")
            outfile = output_dir / f"{curve_name}_chaos-{label}.wav"
            write_wav(outfile, sample_rate, processed)
            print(f"wrote {outfile}")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate chaos-per-curve demo clips without committing binaries."
    )
    parser.add_argument(
        "input",
        type=pathlib.Path,
        default=pathlib.Path("docs/assets/sample.wav"),
        help="Path to a dry 16-bit PCM WAV riff to feed the curves.",
    )
    parser.add_argument(
        "--output-dir",
        type=pathlib.Path,
        default=pathlib.Path("docs/media"),
        help="Where to drop rendered clips (default: docs/media).",
    )
    parser.add_argument(
        "--drive-db",
        type=float,
        default=12.0,
        help="Pre-curve drive in dB. Push hotter if your sample is quiet (default: 12 dB).",
    )
    parser.add_argument(
        "--bias",
        type=float,
        default=0.0,
        help="Bias offset before the curve (default: 0).",
    )
    parser.add_argument(
        "--dirt",
        type=float,
        default=0.0,
        help="Dirt pre-emphasis amount (default: 0).",
    )
    parser.add_argument(
        "--chaos-levels",
        type=float,
        nargs="+",
        default=[1.0, 3.0, 6.0],
        help="Chaos depths to render (default: 1.0 3.0 6.0).",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    render_curve_set(
        input_path=args.input,
        output_dir=args.output_dir,
        drive_db=args.drive_db,
        bias=args.bias,
        dirt=args.dirt,
        chaos_levels=args.chaos_levels,
    )


if __name__ == "__main__":
    main()
