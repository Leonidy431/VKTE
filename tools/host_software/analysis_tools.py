"""Offline analysis helpers for shotgun test-bench sessions.

Computes group statistics (mean point of impact, spread) and fits the
barrel-temperature-versus-point-of-impact drift observed across a firing
string. Pure standard library; no third-party dependencies required.

Targets Python 3.9+ and follows PEP 8 / PEP 257 conventions.
"""

from __future__ import annotations

import json
import math
from dataclasses import dataclass
from typing import List, Sequence, Tuple


@dataclass
class GroupStats:
    """Summary statistics for one shot group."""

    shot_count: int
    mean_x_mm: float
    mean_y_mm: float
    extreme_spread_mm: float
    mean_radius_mm: float


def load_session(path: str) -> dict:
    """Load a session JSON file previously written by ``shotgun_logger``."""
    with open(path, "r", encoding="utf-8") as handle:
        return json.load(handle)


def compute_group_stats(shots: Sequence[dict]) -> GroupStats:
    """Compute point-of-impact statistics for a list of shot dictionaries.

    Args:
        shots: Sequence of shot records with ``hit_x_mm`` / ``hit_y_mm`` keys.

    Returns:
        A populated :class:`GroupStats`.

    Raises:
        ValueError: If ``shots`` is empty.
    """
    if not shots:
        raise ValueError("cannot compute statistics for an empty group")

    xs = [float(shot["hit_x_mm"]) for shot in shots]
    ys = [float(shot["hit_y_mm"]) for shot in shots]
    mean_x = sum(xs) / len(xs)
    mean_y = sum(ys) / len(ys)

    radii = [math.hypot(x - mean_x, y - mean_y) for x, y in zip(xs, ys)]
    mean_radius = sum(radii) / len(radii)

    extreme_spread = _extreme_spread(list(zip(xs, ys)))

    return GroupStats(
        shot_count=len(shots),
        mean_x_mm=mean_x,
        mean_y_mm=mean_y,
        extreme_spread_mm=extreme_spread,
        mean_radius_mm=mean_radius,
    )


def _extreme_spread(points: List[Tuple[float, float]]) -> float:
    """Return the largest center-to-center distance between any two impacts."""
    worst = 0.0
    for i in range(len(points)):
        for j in range(i + 1, len(points)):
            distance = math.hypot(
                points[i][0] - points[j][0],
                points[i][1] - points[j][1],
            )
            worst = max(worst, distance)
    return worst


def fit_thermal_drift(shots: Sequence[dict]) -> Tuple[float, float]:
    """Fit vertical POI shift against barrel temperature by least squares.

    Args:
        shots: Sequence of shot records with ``barrel_temp_c`` and
            ``hit_y_mm`` keys.

    Returns:
        A ``(slope_mm_per_c, intercept_mm)`` tuple. Slope is the vertical
        POI drift per degree Celsius of barrel temperature.

    Raises:
        ValueError: If fewer than two shots or all temperatures are equal.
    """
    if len(shots) < 2:
        raise ValueError("need at least two shots to fit a trend")

    temps = [float(shot["barrel_temp_c"]) for shot in shots]
    shifts = [float(shot["hit_y_mm"]) for shot in shots]
    count = len(temps)

    sum_t = sum(temps)
    sum_s = sum(shifts)
    sum_tt = sum(t * t for t in temps)
    sum_ts = sum(t * s for t, s in zip(temps, shifts))

    denom = count * sum_tt - sum_t * sum_t
    if abs(denom) < 1e-9:
        raise ValueError("temperatures are constant; slope is undefined")

    slope = (count * sum_ts - sum_t * sum_s) / denom
    intercept = (sum_s - slope * sum_t) / count
    return slope, intercept


def summarize(path: str) -> None:
    """Print a human-readable summary for a session file."""
    session = load_session(path)
    shots = session.get("shots", [])
    stats = compute_group_stats(shots)

    print(f"Session: {session.get('name', '?')}")
    print(f"  Ammo: {session.get('ammo_type', '?')}")
    print(f"  Shots: {stats.shot_count}")
    print(f"  Mean POI: ({stats.mean_x_mm:.1f}, {stats.mean_y_mm:.1f}) mm")
    print(f"  Mean radius: {stats.mean_radius_mm:.1f} mm")
    print(f"  Extreme spread: {stats.extreme_spread_mm:.1f} mm")

    try:
        slope, _ = fit_thermal_drift(shots)
        print(f"  Thermal drift: {slope:.3f} mm/degC (vertical)")
    except ValueError as error:
        print(f"  Thermal drift: not available ({error})")


if __name__ == "__main__":
    import sys

    if len(sys.argv) != 2:
        print("usage: python analysis_tools.py <session.json>")
        raise SystemExit(2)
    summarize(sys.argv[1])
