# STM32H745 Shotgun Test Bench - Host Software

Complete Python suite for acquiring, analyzing, and managing ballistic test data from the embedded test bench hardware.

## Overview

This directory contains three modules:

1. **`cli.py`** — Complete command-line interface for all test bench operations
2. **`shotgun_logger.py`** — Serial acquisition API and session management
3. **`analysis_tools.py`** — Offline statistics (POI, spread, thermal drift)

## Quick Start

### Prerequisites

```bash
pip install pyserial  # Optional; only needed for live hardware
```

(No dependencies required for mock mode or analysis of saved sessions.)

### Simulate Test Data (No Hardware)

```bash
python3 cli.py mock --shots 15 --session "factory_28g" --ammo "factory_ammo" --distance 25.0
```

Output: `session_factory_28g.json` with 15 simulated shots.

### Analyze Results

```bash
python3 cli.py analyze session_factory_28g.json --thermal --recoil
```

Shows:
- Mean point of impact (POI)
- Extreme spread (largest bullet hole distance)
- Thermal drift coefficient (mm per °C of barrel temperature)
- Recoil amplitude statistics

### Export to CSV

```bash
python3 cli.py export session_factory_28g.json --format csv --output shots.csv
```

For use in Excel, Google Sheets, or other analysis tools.

### Validate Data

```bash
python3 cli.py validate session_factory_28g.json
```

Checks for:
- Missing required fields
- Out-of-range values (negative recoil, extreme temperatures)
- Monotonicity (timestamps must increase)

## Complete Workflow

Run the full demo:

```bash
bash demo.sh
```

This will:
1. Simulate two ammunition groups (factory vs. reloaded)
2. Analyze each group independently
3. Validate data integrity
4. Export to CSV
5. Compare statistics across ammo types

## Command Reference

### `mock` — Simulate Shots

Simulate realistic shot data with thermal heating and recoil variance.

```bash
python3 cli.py mock [OPTIONS]
  --shots N              Number of shots to simulate [default: 10]
  --session NAME         Session name [default: "mock_session"]
  --ammo TYPE            Ammunition type [default: "simulated"]
  --distance METERS      Distance to target [default: 25.0]
  --output FILE          Save to file [default: "session_*.json"]
```

**Example:**
```bash
python3 cli.py mock --shots 20 --session "test_load_1" --ammo "2.6gr_powder"
```

### `acquire` — Stream from Hardware

Acquire shots from the STM32H745 test bench via serial link.

```bash
python3 cli.py acquire PORT [OPTIONS]
  PORT                   Serial port (e.g., /dev/ttyUSB0, COM3)
  --session NAME         Session name
  --ammo TYPE            Ammunition profile ID
  --distance METERS      Target distance [default: 25.0]
  --baudrate RATE        Serial speed [default: 921600]
  --timeout SEC          Serial timeout [default: 1.0]
  --duration SEC         Maximum acquisition time (Ctrl+C to stop)
  --output FILE          Save to file [default: "session_*.json"]
```

**Example:**
```bash
python3 cli.py acquire /dev/ttyUSB0 --session "factory_28g" --ammo "factory_ammo" --duration 60
```

Streams shots to the console as they arrive. Press `Ctrl+C` to stop and save.

### `analyze` — Compute Statistics

Analyze a saved session file.

```bash
python3 cli.py analyze SESSION_FILE [OPTIONS]
  SESSION_FILE           JSON session file
  --thermal              Include thermal drift analysis (mm/°C)
  --recoil               Include recoil amplitude statistics
```

**Output:**
- **Point of Impact:** mean X, Y, mean radius, extreme spread
- **Thermal Drift:** POI shift per degree Celsius (vertical axis)
- **Recoil Analysis:** mean, min, max peak acceleration, amplitude decay

### `export` — Convert to CSV/JSON

Export session data for external tools.

```bash
python3 cli.py export SESSION_FILE [OPTIONS]
  --format {csv,json}    Output format [default: csv]
  --output FILE          Output file
```

**Example:**
```bash
python3 cli.py export session_factory_28g.json --format csv --output factory_28g.csv
```

CSV columns: `shot_id`, `timestamp_ms`, `distance_m`, `barrel_temp_c`, `recoil_peak_g`, `hit_x_mm`, `hit_y_mm`, `ammo_type`

### `validate` — Check Data Integrity

Detect errors and warnings in session files.

```bash
python3 cli.py validate SESSION_FILE
```

**Checks:**
- Missing required fields (shot_id, timestamp_ms, recoil_peak_g)
- Out-of-range values (negative recoil, extreme temperatures)
- Timestamp monotonicity (must be increasing)
- Sanity bounds (recoil 2–30g, temperature –40 to +300°C)

### `command` — Send Firmware Commands

Send control commands to the test bench (requires hardware).

```bash
python3 cli.py command {start|stop|status|thermal} [OPTIONS]
  --port PORT            Serial port
  --session-id ID        Session ID (for start command)
  --ammo-id ID           Ammo type ID (for start command)
  --threshold TEMP_C     Thermal threshold (for thermal command)
  --baudrate RATE        Serial speed [default: 921600]
```

**Examples:**
```bash
# Start a session with specific ammo type
python3 cli.py command start --port /dev/ttyUSB0 --session-id 1 --ammo-id 3

# Set thermal warning threshold
python3 cli.py command thermal --port /dev/ttyUSB0 --threshold 200

# Stop the current session
python3 cli.py command stop --port /dev/ttyUSB0

# Query system status (response handled by firmware)
python3 cli.py command status --port /dev/ttyUSB0
```

## API Reference (Python)

For programmatic use, import the modules directly:

### ShotgunTestBench

```python
from shotgun_logger import ShotgunTestBench, ShotEvent, Session

# Connect to hardware
bench = ShotgunTestBench("/dev/ttyUSB0")

# Start a session
session = bench.start_session("my_group", "factory_ammo", 25.0)

# Stream shots
for shot in bench.stream_shots(duration_s=60):
    print(f"Shot {shot.shot_id}: {shot.recoil_peak_g}g at {shot.barrel_temp_c}°C")

# End and save
session = bench.end_session()
session.save("results.json")
bench.close()
```

### GroupStats

```python
from analysis_tools import compute_group_stats, fit_thermal_drift, load_session

# Load a session
session = load_session("results.json")
shots = session["shots"]

# Compute statistics
stats = compute_group_stats(shots)
print(f"Mean POI: ({stats.mean_x_mm}, {stats.mean_y_mm}) mm")
print(f"Extreme spread: {stats.extreme_spread_mm} mm")

# Thermal drift analysis
slope, intercept = fit_thermal_drift(shots)
print(f"Drift: {slope:.3f} mm/°C")
```

## Session File Format

Sessions are stored as JSON with this structure:

```json
{
  "name": "factory_group_1",
  "ammo_type": "factory_28g",
  "expected_distance_m": 25.0,
  "shots": [
    {
      "shot_id": 1,
      "timestamp_ms": 1685101234567,
      "distance_m": 25.0,
      "barrel_temp_c": 22.5,
      "recoil_peak_g": 12.36,
      "hit_x_mm": 2,
      "hit_y_mm": 1,
      "ammo_type": "factory_28g"
    },
    ...
  ]
}
```

## Typical Workflows

### Single Ammunition Profiling

```bash
# Acquire 30 shots of one ammo type
python3 cli.py acquire /dev/ttyUSB0 --session "factory_28g" --ammo "factory" --duration 300

# Analyze
python3 cli.py analyze session_factory_28g.json --thermal --recoil

# Export for spreadsheet
python3 cli.py export session_factory_28g.json --format csv
```

### Multi-Load Comparison

```bash
# Test load 1
python3 cli.py acquire /dev/ttyUSB0 --session "load_1" --ammo "2.5gr_powder" --output load_1.json

# Test load 2
python3 cli.py acquire /dev/ttyUSB0 --session "load_2" --ammo "2.6gr_powder" --output load_2.json

# Compare
python3 cli.py analyze load_1.json --thermal --recoil
python3 cli.py analyze load_2.json --thermal --recoil

# Export both to CSV and compare in spreadsheet
python3 cli.py export load_1.json --format csv --output load_1.csv
python3 cli.py export load_2.json --format csv --output load_2.csv
```

### Thermal Stability Testing

```bash
# Long session (track barrel heating)
python3 cli.py acquire /dev/ttyUSB0 --session "thermal_test" --duration 900

# Analyze thermal drift
python3 cli.py analyze thermal_test.json --thermal

# Output shows: POI drift coefficient (mm/°C)
# Indicates how much zero shifts per degree of barrel temperature
```

## Troubleshooting

### "pyserial is required for live acquisition"

Install with: `pip install pyserial`

### Serial port not found

- Check your USB connection
- On macOS: `/dev/tty.usbserial-*`
- On Linux: `/dev/ttyUSB*`
- On Windows: `COM3`, `COM4`, etc.

List available ports:
```bash
python3 -m serial.tools.list_ports
```

### No shots recorded

- Verify test bench is powered and firmware is running
- Check serial connection (LED indicators on bench)
- Try a quick test: `python3 cli.py command status --port /dev/ttyUSB0`
- Press the trigger on the test bench (mock shoots)

### Thermal drift calculation fails

Need at least 2 shots with different barrel temperatures. Short sessions may show "constant temperature" error.

### CSV export shows unusual numbers

Recoil is in g (gravitational acceleration), temperature in °C, distance in mm. If values seem inverted, check the units in your spreadsheet formatter.

## Performance

- **Mock simulation:** <100 ms for 100 shots
- **Hardware acquisition:** Limited by serial speed (921600 baud) and IMU sample rate (1000 Hz)
  - Typical: ~100 shots per minute (~10 seconds per shot including ballistic time)
- **Analysis:** <10 ms for 100 shots (pure Python, no dependencies)

## Future Extensions

Planned additions (not yet implemented):

- IMU calibration support (`CMD_CALIBRATE_IMU`)
- Firmware profile save/load (`CMD_SAVE_PROFILE`, `CMD_LOAD_PROFILE`)
- Flash log download (`CMD_DOWNLOAD_LOGS`)
- Real-time plotting (live recoil curve, thermal drift graph)
- Advanced statistical model (robust regression, outlier detection)

## License

MIT. See LICENSE in the project root.

## Support

For issues or feature requests, check:
1. This README and the `--help` output
2. The demo script (`demo.sh`) for working examples
3. The firmware documentation in `../..`
