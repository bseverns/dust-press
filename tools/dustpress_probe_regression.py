#!/usr/bin/env python3
"""Keep the Dust Press limiter honest.

This script runs ``native/build/dustpress_probe`` with the same seeds baked
into the C++ harness, slurps the CSV telemetry, and checks a handful of
"don't break these" metrics against a checked-in baseline. Think envelope RMS,
peak smackdowns, and how far the limiter gain ever dips.

Usage (after building the probe target):

    python tools/dustpress_probe_regression.py

Regenerate the baseline if you *intentionally* change the DSP shape:

    python tools/dustpress_probe_regression.py --update-baseline

You can also point at a different probe binary or stretch the runtime:

    python tools/dustpress_probe_regression.py \
        --probe-path native/build/dustpress_probe \
        --seconds 4.0
"""

from __future__ import annotations

import argparse
import csv
import json
import math
import subprocess
import tempfile
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, Iterable, List, Tuple

REPO_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_BASELINE = REPO_ROOT / "tools" / "dustpress_probe_baseline.json"


@dataclass
class RunningStat:
    """Lightweight streaming stats so we do not hoard a million floats."""

    sum_sq: float = 0.0
    count: int = 0
    peak: float = 0.0
    minimum: float | None = None
    maximum: float | None = None

    def ingest(self, value: float, track_bounds: bool = False) -> None:
        self.count += 1
        self.sum_sq += value * value
        self.peak = max(self.peak, abs(value))
        if track_bounds:
            if self.minimum is None or value < self.minimum:
                self.minimum = value
            if self.maximum is None or value > self.maximum:
                self.maximum = value

    def rms(self) -> float:
        return math.sqrt(self.sum_sq / self.count) if self.count else 0.0


@dataclass
class SampleRateMetrics:
    env: RunningStat = field(default_factory=RunningStat)
    limiter_env: RunningStat = field(default_factory=RunningStat)
    limiter_gain: RunningStat = field(default_factory=RunningStat)
    gate_gain: RunningStat = field(default_factory=RunningStat)
    out_peak: RunningStat = field(default_factory=RunningStat)

    def to_dict(self) -> Dict[str, float]:
        return {
            "env_rms": self.env.rms(),
            "env_peak": self.env.peak,
            "limiter_env_rms": self.limiter_env.rms(),
            "limiter_env_peak": self.limiter_env.peak,
            "limiter_gain_min": self._safe_bound(self.limiter_gain.minimum),
            "limiter_gain_max": self._safe_bound(self.limiter_gain.maximum),
            "gate_gain_min": self._safe_bound(self.gate_gain.minimum),
            "gate_gain_max": self._safe_bound(self.gate_gain.maximum),
            "out_peak": self.out_peak.peak,
        }

    @staticmethod
    def _safe_bound(value: float | None) -> float:
        return 0.0 if value is None else value


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Probe the DSP and check telemetry baselines")
    parser.add_argument(
        "--probe-path",
        default=str(REPO_ROOT / "native" / "build" / "dustpress_probe"),
        help="Path to the dustpress_probe binary (default: native/build/dustpress_probe)",
    )
    parser.add_argument(
        "--seconds",
        type=float,
        default=4.0,
        help="How long to run the probe stimulus at each sample rate",
    )
    parser.add_argument(
        "--baseline",
        type=Path,
        default=DEFAULT_BASELINE,
        help="Baseline JSON to compare against",
    )
    parser.add_argument(
        "--update-baseline",
        action="store_true",
        help="Rewrite the baseline file using the current DSP output",
    )
    parser.add_argument(
        "--csv-out",
        type=Path,
        help=(
            "Optional path to stash the raw probe CSV. Handy for CI artefacts or when "
            "you want to eyeball the telemetry later."
        ),
    )
    parser.add_argument(
        "--keep-csv",
        action="store_true",
        help="Keep the raw probe CSV instead of deleting the temp file",
    )
    return parser.parse_args()


def run_probe(probe_path: Path, seconds: float, csv_path: Path) -> None:
    if not probe_path.exists():
        raise FileNotFoundError(f"Probe binary not found: {probe_path}")

    cmd = [str(probe_path), "--seconds", f"{seconds}", "--out", str(csv_path)]
    print(f"Running {' '.join(cmd)}")
    subprocess.run(cmd, check=True)


def load_metrics(csv_path: Path) -> Dict[str, Dict[str, float]]:
    metrics: Dict[str, SampleRateMetrics] = {}
    with csv_path.open(newline="") as handle:
        reader = csv.DictReader(handle)
        for row in reader:
            sr = row["sample_rate"]
            if sr not in metrics:
                metrics[sr] = SampleRateMetrics()

            tap = metrics[sr]
            tap.env.ingest(float(row["env"]))
            tap.limiter_env.ingest(float(row["limiter_env"]))
            tap.limiter_gain.ingest(float(row["limiter_gain"]), track_bounds=True)
            tap.gate_gain.ingest(float(row["gate_gain"]), track_bounds=True)
            tap.out_peak.ingest(float(row["out_peak"]))

    return {sr: stat.to_dict() for sr, stat in metrics.items()}


def write_baseline(baseline_path: Path, seconds: float, metrics: Dict[str, Dict[str, float]]) -> None:
    baseline = {
        "seconds": seconds,
        "metrics": metrics,
        "notes": "Generated by tools/dustpress_probe_regression.py; seeds live in native/src/probe.cpp.",
    }
    baseline_path.write_text(json.dumps(baseline, indent=2, sort_keys=True))
    print(f"Wrote baseline to {baseline_path}")


def compare_metrics(current: Dict[str, Dict[str, float]], baseline: Dict[str, Dict[str, float]]) -> List[str]:
    failures: List[str] = []
    rel_tol = 5e-4
    abs_tol = 5e-4

    for sr, expected in baseline.items():
        if sr not in current:
            failures.append(f"Missing sample rate {sr} in current telemetry")
            continue

        observed = current[sr]
        for key, expected_val in expected.items():
            if key not in observed:
                failures.append(f"[{sr}] Missing metric {key}")
                continue
            actual_val = observed[key]
            if not math.isclose(actual_val, expected_val, rel_tol=rel_tol, abs_tol=abs_tol):
                failures.append(
                    f"[{sr}] {key} drifted: expected {expected_val:.6f}, got {actual_val:.6f}"
                )

    for sr in current:
        if sr not in baseline:
            failures.append(f"Unexpected sample rate present in current telemetry: {sr}")

    return failures


def main() -> int:
    args = parse_args()
    probe_path = Path(args.probe_path)
    baseline_path = Path(args.baseline)

    csv_path: Path
    # If the caller gave us an explicit output path we skip the temp file dance so
    # CI can scoop up the artefact even on failure. Otherwise fall back to a temp
    # that evaporates unless --keep-csv is set.
    if args.csv_out:
        csv_path = args.csv_out
        run_probe(probe_path, args.seconds, csv_path)
        metrics = load_metrics(csv_path)
        print(f"Probe CSV written to: {csv_path}")
    else:
        with tempfile.NamedTemporaryFile(prefix="dustpress_probe_", suffix=".csv", delete=not args.keep_csv) as tmp:
            csv_path = Path(tmp.name)
            run_probe(probe_path, args.seconds, csv_path)
            metrics = load_metrics(csv_path)

            if args.keep_csv:
                print(f"Probe CSV kept at: {csv_path}")

        if args.update_baseline:
            write_baseline(baseline_path, args.seconds, metrics)
            return 0

        if not baseline_path.exists():
            raise FileNotFoundError(
                f"No baseline found at {baseline_path}. Run with --update-baseline to record one."
            )

        baseline_blob = json.loads(baseline_path.read_text())
        baseline_seconds = baseline_blob.get("seconds")
        if baseline_seconds and not math.isclose(float(baseline_seconds), float(args.seconds), rel_tol=0.0, abs_tol=1e-3):
            print(
                f"[warn] baseline captured at {baseline_seconds}s but you're running {args.seconds}s."
            )
        failures = compare_metrics(metrics, baseline_blob.get("metrics", {}))
        if failures:
            print("\nProbe regression detected:")
            for failure in failures:
                print(f"  - {failure}")
            return 1

        print("All probe metrics match the baseline. The limiter stays in line.")
        return 0


if __name__ == "__main__":
    raise SystemExit(main())
