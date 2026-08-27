#!/usr/bin/env python3
"""Command-line interface for the STM32H745 shotgun test bench.

Provides a complete workflow for acquiring shots, managing sessions, and
analyzing group statistics. Run with --help for usage.

Usage:
    # Simulate shots (mock mode, no hardware)
    python cli.py mock --shots 10

    # Acquire from real hardware
    python cli.py acquire /dev/ttyUSB0 --session "factory_28g" --distance 25.0

    # Analyze a saved session
    python cli.py analyze session.json
    python cli.py analyze session.json --thermal
"""

from __future__ import annotations

import argparse
import json
import sys
import time
from dataclasses import asdict
from pathlib import Path
from typing import Optional

from analysis_tools import compute_group_stats, fit_thermal_drift, load_session
from shotgun_logger import ShotEvent, Session, ShotgunTestBench


def cmd_mock(args) -> int:
    """Simulate shot events without real hardware."""
    print(f"📡 Mock mode: simulating {args.shots} shots")

    session = Session(
        name=args.session or "mock_session",
        ammo_type=args.ammo or "simulated",
        expected_distance_m=float(args.distance or 25.0),
    )

    for shot_num in range(1, args.shots + 1):
        # Simulate realistic shot data with some variance
        import random

        shot = ShotEvent(
            shot_id=shot_num,
            timestamp_ms=int(time.time() * 1000) + shot_num * 100,
            distance_m=float(args.distance or 25.0),
            barrel_temp_c=20.0 + (shot_num * 2.5),  # Barrel heats up
            recoil_peak_g=12.0 + random.gauss(0, 0.5),  # ~12g with variance
            hit_x_mm=int(random.gauss(0, 5)),  # ±5mm dispersion
            hit_y_mm=int(random.gauss(0, 5)),
            ammo_type=args.ammo or "simulated",
        )
        session.add(shot)
        print(
            f"  Shot {shot_num:2d}: "
            f"T={shot.barrel_temp_c:6.1f}°C, "
            f"Recoil={shot.recoil_peak_g:5.2f}g, "
            f"POI=({shot.hit_x_mm:+4d}, {shot.hit_y_mm:+4d}) mm"
        )

    # Save session
    output = args.output or f"session_{args.session or 'mock'}.json"
    session.save(output)
    print(f"\n✓ Saved {len(session.shots)} shots to {output}")
    return 0


def cmd_acquire(args) -> int:
    """Acquire shots from the real hardware over serial."""
    port = args.port
    session_name = args.session or "live_session"
    ammo_type = args.ammo or "unknown"
    distance = float(args.distance or 25.0)

    print(f"🔌 Connecting to {port} at {args.baudrate} baud...")

    try:
        bench = ShotgunTestBench(port, baudrate=args.baudrate, timeout=args.timeout)
    except RuntimeError as error:
        print(f"❌ Error: {error}", file=sys.stderr)
        return 1

    print(f"📋 Starting session: '{session_name}'")
    session = bench.start_session(session_name, ammo_type, distance)

    print("🎯 Streaming shots (press Ctrl+C to stop)...")
    shot_count = 0

    try:
        for shot in bench.stream_shots(duration_s=args.duration):
            shot_count += 1
            print(
                f"  Shot {shot.shot_id:3d}: "
                f"T={shot.barrel_temp_c:6.1f}°C, "
                f"Recoil={shot.recoil_peak_g:5.2f}g, "
                f"POI=({shot.hit_x_mm:+4d}, {shot.hit_y_mm:+4d}) mm"
            )
    except KeyboardInterrupt:
        print("\n⏹️  Interrupted by user")
    finally:
        session = bench.end_session()
        bench.close()

    if session and shot_count > 0:
        output = args.output or f"session_{session_name}.json"
        session.save(output)
        print(f"\n✓ Saved {shot_count} shots to {output}")
        return 0
    else:
        print("❌ No shots recorded", file=sys.stderr)
        return 1


def cmd_analyze(args) -> int:
    """Analyze a saved session file."""
    path = Path(args.session_file)

    if not path.exists():
        print(f"❌ File not found: {path}", file=sys.stderr)
        return 1

    try:
        session = load_session(str(path))
    except (json.JSONDecodeError, KeyError) as error:
        print(f"❌ Invalid session file: {error}", file=sys.stderr)
        return 1

    shots = session.get("shots", [])
    if not shots:
        print("❌ No shots in session", file=sys.stderr)
        return 1

    # Basic statistics
    print(f"\n📊 Session Analysis: {session.get('name', '?')}")
    print(f"   Ammunition: {session.get('ammo_type', '?')}")
    print(f"   Distance: {session.get('expected_distance_m', '?')} m")
    print(f"   Shots: {len(shots)}")

    try:
        stats = compute_group_stats(shots)
        print(f"\n🎯 Point of Impact:")
        print(f"   Mean: ({stats.mean_x_mm:+7.2f}, {stats.mean_y_mm:+7.2f}) mm")
        print(f"   Mean Radius: {stats.mean_radius_mm:7.2f} mm")
        print(f"   Extreme Spread: {stats.extreme_spread_mm:7.2f} mm")
    except ValueError as error:
        print(f"   (Group stats unavailable: {error})", file=sys.stderr)

    # Thermal drift analysis (if requested and available)
    if args.thermal:
        try:
            slope, intercept = fit_thermal_drift(shots)
            print(f"\n🌡️  Thermal Drift Analysis:")
            print(f"   Slope: {slope:+.4f} mm/°C (vertical)")
            print(f"   Intercept: {intercept:+.2f} mm")

            # Estimate zero shift over typical barrel temperature range
            temp_min = min(s["barrel_temp_c"] for s in shots)
            temp_max = max(s["barrel_temp_c"] for s in shots)
            shift_total = slope * (temp_max - temp_min)
            print(f"   Temperature range: {temp_min:.1f}–{temp_max:.1f}°C")
            print(f"   Total POI shift: {shift_total:+.2f} mm across range")
        except ValueError as error:
            print(f"   (Thermal drift unavailable: {error})", file=sys.stderr)

    # Recoil analysis (if available)
    if args.recoil:
        recoils = [float(s.get("recoil_peak_g", 0)) for s in shots if s.get("recoil_peak_g")]
        if recoils:
            mean_recoil = sum(recoils) / len(recoils)
            min_recoil = min(recoils)
            max_recoil = max(recoils)
            print(f"\n💥 Recoil Analysis:")
            print(f"   Mean: {mean_recoil:.2f} g")
            print(f"   Min: {min_recoil:.2f} g")
            print(f"   Max: {max_recoil:.2f} g")
            print(f"   Amplitude decay: {((max_recoil - min_recoil) / max_recoil * 100):.1f}%")

    print()
    return 0


def cmd_export(args) -> int:
    """Export session data to CSV or other formats."""
    path = Path(args.session_file)

    if not path.exists():
        print(f"❌ File not found: {path}", file=sys.stderr)
        return 1

    try:
        session = load_session(str(path))
    except (json.JSONDecodeError, KeyError) as error:
        print(f"❌ Invalid session file: {error}", file=sys.stderr)
        return 1

    shots = session.get("shots", [])
    if not shots:
        print("❌ No shots in session", file=sys.stderr)
        return 1

    output = args.output or f"{path.stem}.csv"
    fmt = args.format or "csv"

    if fmt == "csv":
        import csv

        with open(output, "w", newline="", encoding="utf-8") as f:
            if shots:
                writer = csv.DictWriter(f, fieldnames=shots[0].keys())
                writer.writeheader()
                writer.writerows(shots)
        print(f"✓ Exported {len(shots)} shots to {output} (CSV)")

    elif fmt == "json":
        with open(output, "w", encoding="utf-8") as f:
            json.dump(shots, f, indent=2)
        print(f"✓ Exported {len(shots)} shots to {output} (JSON)")

    else:
        print(f"❌ Unknown format: {fmt}", file=sys.stderr)
        return 1

    return 0


def cmd_validate(args) -> int:
    """Validate session data integrity."""
    path = Path(args.session_file)

    if not path.exists():
        print(f"❌ File not found: {path}", file=sys.stderr)
        return 1

    try:
        session = load_session(str(path))
    except (json.JSONDecodeError, KeyError) as error:
        print(f"❌ Invalid JSON: {error}", file=sys.stderr)
        return 1

    shots = session.get("shots", [])
    print(f"🔍 Validating {len(shots)} shots...")

    errors = []
    warnings = []

    for i, shot in enumerate(shots):
        # Required fields
        if "shot_id" not in shot:
            errors.append(f"  Shot {i}: missing shot_id")
        if "timestamp_ms" not in shot:
            errors.append(f"  Shot {i}: missing timestamp_ms")
        if "recoil_peak_g" not in shot:
            errors.append(f"  Shot {i}: missing recoil_peak_g")

        # Sanity checks
        recoil = shot.get("recoil_peak_g", 0)
        if recoil < 0:
            errors.append(f"  Shot {i}: negative recoil ({recoil}g)")
        elif recoil < 2 or recoil > 30:
            warnings.append(f"  Shot {i}: unusual recoil ({recoil}g)")

        temp = shot.get("barrel_temp_c", 20)
        if temp < -40 or temp > 300:
            errors.append(f"  Shot {i}: temperature out of range ({temp}°C)")

        # Timestamp monotonicity
        if i > 0:
            prev_ts = shots[i - 1].get("timestamp_ms", 0)
            curr_ts = shot.get("timestamp_ms", 0)
            if curr_ts < prev_ts:
                warnings.append(f"  Shot {i}: timestamp goes backward")

    if errors:
        print(f"\n❌ {len(errors)} error(s) found:")
        for err in errors:
            print(err)
    else:
        print("✓ No errors found")

    if warnings:
        print(f"\n⚠️  {len(warnings)} warning(s):")
        for warn in warnings:
            print(warn)
    else:
        print("✓ No warnings")

    return 0 if not errors else 1


def cmd_command(args) -> int:
    """Send a raw command to the test bench (requires hardware)."""
    if not args.port:
        print("❌ Port required for command mode", file=sys.stderr)
        return 1

    try:
        bench = ShotgunTestBench(args.port, baudrate=args.baudrate)
    except RuntimeError as error:
        print(f"❌ Connection failed: {error}", file=sys.stderr)
        return 1

    try:
        if args.cmd == "start":
            session_id = int(args.session_id or 1)
            ammo_id = int(args.ammo_id or 0)
            print(f"🚀 Sending CMD_START_SESSION (session={session_id}, ammo={ammo_id})")
            bench._send_command("start_session", session_id=session_id, ammo_type=ammo_id)
            print("✓ Command sent")

        elif args.cmd == "stop":
            print("🛑 Sending CMD_STOP_SESSION")
            bench._send_command("stop_session")
            print("✓ Command sent")

        elif args.cmd == "status":
            print("📊 Sending CMD_QUERY_STATUS")
            bench._send_command("query_status")
            print("✓ Command sent (awaiting response...)")
            # Would need async response handling for real implementation

        elif args.cmd == "thermal":
            temp_threshold = float(args.threshold or 180.0)
            print(f"🌡️  Sending CMD_SET_THERMAL_THRESHOLD ({temp_threshold}°C)")
            bench._send_command("set_thermal_threshold", threshold_c=temp_threshold)
            print("✓ Command sent")

        else:
            print(f"❌ Unknown command: {args.cmd}", file=sys.stderr)
            return 1

    finally:
        bench.close()

    return 0


def main() -> int:
    """Main entry point."""
    parser = argparse.ArgumentParser(
        prog="shotgun-bench",
        description="STM32H745 Shotgun Test Bench CLI",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Simulate 10 shots
  %(prog)s mock --shots 10

  # Acquire from hardware
  %(prog)s acquire /dev/ttyUSB0 --session "load1" --ammo "factory_28g"

  # Analyze results
  %(prog)s analyze session_load1.json --thermal --recoil

  # Export to CSV
  %(prog)s export session_load1.json --format csv

  # Validate data
  %(prog)s validate session_load1.json
        """,
    )

    subparsers = parser.add_subparsers(dest="command", help="Available commands")

    # Mock command
    mock_parser = subparsers.add_parser("mock", help="Simulate shots (no hardware)")
    mock_parser.add_argument("--shots", type=int, default=10, help="Number of shots")
    mock_parser.add_argument("--session", help="Session name")
    mock_parser.add_argument("--ammo", help="Ammunition type")
    mock_parser.add_argument("--distance", default=25.0, help="Distance (meters)")
    mock_parser.add_argument("--output", help="Output file (default: session_*.json)")
    mock_parser.set_defaults(func=cmd_mock)

    # Acquire command
    acquire_parser = subparsers.add_parser("acquire", help="Acquire from hardware")
    acquire_parser.add_argument("port", help="Serial port (e.g., /dev/ttyUSB0)")
    acquire_parser.add_argument("--session", help="Session name")
    acquire_parser.add_argument("--ammo", help="Ammunition type")
    acquire_parser.add_argument("--distance", default=25.0, help="Distance (meters)")
    acquire_parser.add_argument("--baudrate", type=int, default=921600, help="Baud rate")
    acquire_parser.add_argument("--timeout", type=float, default=1.0, help="Serial timeout")
    acquire_parser.add_argument("--duration", type=float, help="Acquisition duration (seconds)")
    acquire_parser.add_argument("--output", help="Output file (default: session_*.json)")
    acquire_parser.set_defaults(func=cmd_acquire)

    # Analyze command
    analyze_parser = subparsers.add_parser("analyze", help="Analyze session results")
    analyze_parser.add_argument("session_file", help="Session JSON file")
    analyze_parser.add_argument("--thermal", action="store_true", help="Include thermal analysis")
    analyze_parser.add_argument("--recoil", action="store_true", help="Include recoil analysis")
    analyze_parser.set_defaults(func=cmd_analyze)

    # Export command
    export_parser = subparsers.add_parser("export", help="Export session data")
    export_parser.add_argument("session_file", help="Session JSON file")
    export_parser.add_argument("--format", choices=["csv", "json"], default="csv")
    export_parser.add_argument("--output", help="Output file (default: session_*.csv)")
    export_parser.set_defaults(func=cmd_export)

    # Validate command
    validate_parser = subparsers.add_parser("validate", help="Validate session integrity")
    validate_parser.add_argument("session_file", help="Session JSON file")
    validate_parser.set_defaults(func=cmd_validate)

    # Command: send raw commands to hardware
    cmd_parser = subparsers.add_parser(
        "command",
        help="Send commands to the test bench",
        description="Send control commands to the test bench (e.g., start/stop session, query status)",
    )
    cmd_parser.add_argument("cmd", choices=["start", "stop", "status", "thermal"])
    cmd_parser.add_argument("--port", help="Serial port (e.g., /dev/ttyUSB0)")
    cmd_parser.add_argument("--session-id", help="Session ID (for start command)")
    cmd_parser.add_argument("--ammo-id", help="Ammo type ID (for start command)")
    cmd_parser.add_argument("--threshold", help="Thermal threshold in °C (for thermal command)")
    cmd_parser.add_argument("--baudrate", type=int, default=921600, help="Baud rate")
    cmd_parser.set_defaults(func=cmd_command)

    args = parser.parse_args()

    if not args.command:
        parser.print_help()
        return 0

    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
