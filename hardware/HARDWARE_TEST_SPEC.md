# STM32H745 Ballistic Corrector — Hardware Test Specification

**Version:** 1.0  
**Date:** 2026-07-28  
**Audience:** Manufacturing, QA, field service engineers  
**Scope:** Validation procedures for prototype and production units

---

## 1. Pre-Assembly Inspection (PAI)

### 1.1 PCB Visual Inspection

```
Checklist (Acceptance Criteria):
[ ] PCB color: Green (standard), any discoloration → REJECT
[ ] Surface finish: Shiny, uniform (HASL) or matte (ENIG)
    - Any dark spots or blistering → REJECT
[ ] Silkscreen: Legible part numbers, reference designators
[ ] Test pads: No solder bridges between adjacent pads
[ ] Edge cuts: Smooth, no rough edges or splinters
[ ] Fiducials: 3x brass pads present (1mm diameter)
[ ] Dimensions: Measure board corners, tolerance ±0.5mm
```

### 1.2 Component Incoming Inspection

```
Critical components (100% inspection):
├─ STM32H745ZIT6 (MCU)
│  └─ Check: Part code laser-marked, date code legible
│  └─ Visual: No bent pins, cracks, or discoloration
│
├─ ICM-20689 (IMU)
│  └─ Check: Seal integrity (if moisture-sensitive bag)
│  └─ Humidity indicator card: Color check
│
├─ W25Q128JV (Flash)
│  └─ Check: Part number match BOM (0xEF 0x40 0x18)
│
└─ CP2102N (USB-UART)
   └─ Check: Date code within 2 years
   └─ Visual: No damage to leads

Sampling for others:
├─ Every 5th resistor: Measure value ±5% with multimeter
├─ Every 10th capacitor: Measure capacitance ±10%
└─ Diodes/inductors: Visual + continuity check
```

---

## 2. Post-Assembly Inspection (PAI)

### 2.1 Solder Joint Quality (AOI + Visual)

```
Automated Optical Inspection (AOI) Rules:
├─ Bridge detection: No solder bridges > 0.2mm
├─ Voids: <25% of pad area (IPC A-610 Class 2)
├─ Volume: Fillet covers 50-90% of component lead
├─ Color: Shiny silver (SAC305) or dull gray (lead-free)
└─ Cracks: None visible at 10x magnification

Manual inspection (sample 1 per 10 boards):
├─ LQFP-144 pads: Solder meniscus uniform, no cold joints
├─ Capacitor terminations: Even fillet both ends
├─ Resistor ends: Completely covered, no exposed end caps
└─ Connector pins: Adequate solder, no excess

REJECT if:
├─ Solder bridges between adjacent pins
├─ Open solder joints (<30% of pad coverage)
├─ Solder splash on adjacent components
└─ Component rotation >5° off nominal
```

### 2.2 Thermal Image Analysis (Infrared Camera)

```
After reflow soldering (board cooling to 25°C):

Check temperature uniformity:
├─ MCU die (center of LQFP-144): Should be coolest initially
├─ Power traces (VCC area): Look for local hotspots
├─ All components: Expect ±10°C variance across board

Suspect issues (investigate with microscope):
├─ Temperature >30°C higher than neighbors → Solder void/bridge
├─ Uneven cooling → Possible delamination
└─ Local cold spot → Possible component not soldered

FAIL if: Gradient >20°C (indicates serious defect)
```

---

## 3. Electrical Testing (Bare Board)

### 3.1 Continuity & Isolation

```
Multimeter tests (all in ohms):

Continuity (should be <1Ω):
├─ All VCC_3V3 pads to VCC_3V3 rail
├─ All GND pads to GND plane
├─ All signal traces (random sampling)
└─ USB D+ to CP2102N DP pin

Isolation (should be >1MΩ):
├─ VCC_3V3 to GND (unpowered): >10MΩ expected
├─ VCC_3V3 to USB_D± (unpowered): >1MΩ
├─ Different voltage rails to each other
└─ Reverse-polarity protection diode: <0.5Ω forward, >1MΩ reverse

Test points:
├─ Measure across decoupling caps: <0.1Ω (good solder joints)
└─ Measure QSPI traces: <0.5Ω end-to-end
```

### 3.2 Power Supply Loading Test

```
Procedure (powered board test):

1. Voltage rail checks (multimeter, DC voltage):
   ┌─ VCC_12V input: 12.0 ± 0.5V (no load)
   ├─ VCC_5V (buck output): 4.95-5.05V (PASS if ±2%)
   ├─ VCC_3V3: 3.25-3.35V (PASS if ±5%)
   ├─ VCC_1V8: 1.70-1.90V (PASS if ±10%)
   └─ Measure at 3 different points on each rail (check uniformity)

2. Load current measurement:
   ├─ At startup (no firmware): <50 mA @ 3.3V (just decoupling caps)
   ├─ After 1 second: <100 mA (MCU booting, no peripherals)
   ├─ After 10 seconds: Stabilize at <200 mA (idle state)
   └─ FAIL if: Current draw >500 mA (indicates short circuit)

3. Voltage ripple (oscilloscope at 3.3V, AC coupling):
   ├─ Peak-to-peak: <50 mV
   ├─ Frequency components: 2.2 MHz (buck converter) with sidebands
   └─ FAIL if: Ripple >100 mV (indicates poor decoupling)

4. Thermal monitoring:
   ├─ Hold powered for 5 minutes
   ├─ Touch test: Any component >50°C → investigate
   ├─ IR camera: No hotspots >60°C (TBD: thermal imaging report)
   └─ FAIL if: Sustained heating (indicates resistive short)
```

---

## 4. Component-Level Functional Tests (Powered)

### 4.1 Clock Tree Verification

```
Test: Verify PLL lock and core clocks

Instruments needed:
├─ Oscilloscope (200 MHz bandwidth minimum)
├─ Frequency counter (10 MHz resolution)
└─ Logic analyzer (optional, for clock edge quality)

Procedure:
1. Apply power (12V barrel connector)
2. Observe NRST pin: Should pulse low <10 µs, then high
3. Wait 500 ms for PLL lock (firmware bootup)
4. Measure frequencies:
   ├─ PA5/PB3 (SPI1 clock): Idle ~0 MHz (no clocking until IMU command)
   ├─ PA2 (UART TX): Count bit periods @ 921600 baud
   │  (Expected: 10.851 µs per bit = 921,600 bits/sec)
   └─ Internal oscillator (Bluetooth, if populated): Should see 32 kHz on LSI

Expected measurements:
├─ UART TX baud rate: 921,600 ± 50 baud
├─ SPI idle frequency: 0 Hz (active low on CS)
└─ All clocks stable within 1 Hz jitter

FAIL if:
├─ UART baud >922,000 or <921,000 (>50 baud error)
├─ Frequent resets (NRST pulsing repeatedly)
└─ Clock glitches (sudden frequency jumps)
```

### 4.2 UART Debug Output (Serial Monitor)

```
Test: Verify UART connectivity and firmware boot

Equipment:
├─ USB-UART cable (CP2102N on board → USB-A on PC)
├─ Serial terminal software (PuTTY, Minicom, Arduino IDE)
└─ PC with USB port

Procedure:
1. Connect USB-UART to PC (driver should auto-load)
2. Open serial port at 921600 baud, 8N1 (no handshake)
3. Power cycle board (disconnect/reconnect 12V)
4. Watch terminal for boot messages:

   Expected output:
   ┌────────────────────────────────────────────┐
   │ === STM32H745 Shotgun Testbench v1.4.0 === │
   │ M7 Core: 480 MHz, M4 Core: 240 MHz         │
   │ Initializing sensors...                    │
   │   ✓ IMU initialized                        │
   │   ✓ LRF initialized                        │
   │ Hardware initialization complete.          │
   │ System ready (STATE_IDLE)                  │
   │ Initializing FreeRTOS...                   │
   │ RTOS initialization complete.              │
   │ Starting FreeRTOS scheduler...             │
   └────────────────────────────────────────────┘

Capture to file: Save terminal log for manufacturing records
Timing check: All messages should appear within 2 seconds
Character check: No garbled text, no missing lines

FAIL if:
├─ No output appears (UART bridge not connected or broken)
├─ Garbled text (baud rate mismatch or clock failure)
├─ Incomplete boot sequence (watchdog reset loop)
└─ Error messages about sensor initialization
```

### 4.3 SPI IMU Verification (Logic Analyzer)

```
Test: Verify ICM-20689 communication via SPI

Equipment:
├─ Logic analyzer (8-channel, 100 MHz sampling minimum)
├─ Probes on: CLK, MOSI, MISO, CS
└─ Optional: USB multimeter for current draw

Procedure:
1. Connect logic analyzer to SPI1 pins (PA5/PA6/PA7/PB0)
2. Trigger on CS falling edge (chip select low)
3. Power on board and wait for boot messages
4. Send command via firmware or REPL to read IMU ID register
   (Expected: Read address 0x00 → should return 0x71)

Capture analysis:
├─ Timing: Clock frequency ~10 MHz (100 ns period)
├─ Setup/hold times: Should meet ICM-20689 datasheet specs
├─ Data pattern: 
│  └─ First byte: 0x80 | 0x00 = 0x80 (read ID register)
│  └─ Second byte: 0x71 (chip ID response from ICM-20689)
├─ CS pulse: Deassert (high) after 2-byte transfer
└─ Idle state: CS high, CLK low, MOSI/MISO idle

Acceptable patterns:
├─ Multiple clock pulses (16-bit transfer for 2 bytes)
├─ MOSI data changes on clock rising edge
├─ MISO data sampled on clock rising edge (mode 0)
└─ Response consistent across multiple reads

FAIL if:
├─ No clock activity (frozen/crashed)
├─ Wrong byte order (LSB-first instead of MSB)
├─ ID register reads 0x00 or 0xFF (chip not responding)
└─ Sporadic bus activity (intermittent connection)
```

### 4.4 I2C Sensor Verification (Logic Analyzer)

```
Test: Verify VL53L0X and MCP9808 on I2C1

Equipment:
├─ Logic analyzer with I2C decoder
├─ Probes on: PB8 (SCL), PB9 (SDA)
└─ Pull-up verification: 10kΩ ± 5% to VCC_3V3

Procedure:
1. Connect logic analyzer to I2C1 bus
2. Trigger on START condition (SCL high, then SDA falling)
3. Power board, wait for boot
4. Issue I2C scan command via firmware (read device IDs)

Expected I2C traffic:
├─ Device 0x29 (VL53L0X): Should respond with model ID 0xEE
├─ Device 0x50 (MCP9808): Should respond with temp data (16-bit)
├─ Slave ACK: SDA pulled low after address byte
├─ Data rate: 100-400 kHz (standard I2C mode)

Waveform checks:
├─ Rising edge: Smooth (capacitive load), <1 µs slew rate
├─ Falling edge: Fast (open-drain pull-down), <100 ns
├─ Clock stretching: Slave may hold SCL low (max 10 ms wait)
└─ Idle state: Both SCL and SDA high (via pull-ups)

FAIL if:
├─ No I2C activity (bus stuck low)
├─ Address no-ack (slave not responding)
├─ Slow rise time on SCL/SDA (>5 µs indicates weak pull-up)
└─ Garbled data (corrupted byte transmission)
```

### 4.5 QSPI Flash Verification

```
Test: Verify W25Q128JV QSPI Flash communication

Equipment:
├─ Logic analyzer (8-channel for quad lines: CLK, D0, D1, D2, D3, CS)
├─ Optional: Programmer/reader device (for offline verification)
└─ Jumper to bypass Flash (if test fails, can use onboard copy)

Procedure:
1. Connect logic analyzer to QSPI pins (PB2, PD11, PD12, PE2, PD13, PA3)
2. Power board and trigger on CS falling edge
3. Send Flash read command via firmware:
   a. Chip Select LOW (CS → 0)
   b. Send command: 0x9F (Read JEDEC ID)
   c. Receive 3 bytes: Expected 0xEF 0x40 0x18
   d. Chip Select HIGH (CS → 1)

Expected capture:
├─ Byte 1: 0xEF (manufacturer ID = Winbond)
├─ Byte 2: 0x40 (memory type = QSPI flash)
├─ Byte 3: 0x18 (capacity ID = 128 Mbit = 16 MB)
├─ Clock: 108 MHz (quad mode) or 10 MHz (fallback SPI)
└─ Timing: <1 ms for single read operation

Stress test (hold for 60 seconds):
├─ Continuous reading: Random addresses from 0x000000 to 0xFFFFFF
├─ Check for errors: Any data mismatch → FAIL
└─ Temperature monitoring: Flash should stay <60°C

FAIL if:
├─ JEDEC response is wrong (0x00, 0xFF, or mismatched bytes)
├─ Clock rate too low (<10 MHz, may indicate MCU clock issue)
├─ Garbled data during reads (bit errors)
└─ Intermittent communication (occasional timeouts)
```

---

## 5. Integrated Functional Tests

### 5.1 Shot Detection Simulation Test

```
Test: Verify complete signal path from accelerometer to logging

Procedure:
1. Prepare test harness:
   ├─ Bench-top vibration platform (can simulate shock)
   ├─ Accelerometer calibrated reference (±0.5g accuracy)
   └─ Optional: Automated test jig with motor to provide repeatable impulses

2. Mount DUT (device under test):
   ├─ Place STM32H745 board on vibration platform
   ├─ Align Z-axis with vertical (gravity reference)
   └─ Orient X/Y for lateral shake

3. Apply test sequence:
   ├─ Static baseline: Measure 1g gravity offset in Z-axis (±0.2g tolerance)
   ├─ Low impulse: 2g spike for 10 ms (should NOT trigger shot detection)
   ├─ Threshold impulse: 5g spike for 20 ms (SHOULD trigger shot detection)
   │  └─ Expected: LED or UART message "SHOT DETECTED"
   ├─ High recoil: 10g spike for 50 ms (triggers, logs to Flash)
   │  └─ Expected: Shot logged, anomaly detector updates baseline
   └─ Repeated fire: 5x shots at 2-second intervals
      └─ Expected: All shots logged, recoil stats accumulate

4. Verify logging:
   ├─ Connect USB-UART, dump session log from M4 core
   ├─ Cross-check shot count: Should match manual impulse count
   ├─ Verify timestamps: Should be monotonically increasing
   └─ Verify CRC32: All records should pass checksum validation

Acceptance criteria:
├─ 100% shot detection rate (no missed shots)
├─ 0% false positive rate (noise should not trigger)
├─ Logging latency: <50 ms from impulse to stored record
└─ Timestamp accuracy: ±5 ms RMS error (1 kHz sampling)

FAIL if:
├─ Missed shots: >5% of applied impulses not logged
├─ False positives: >2 unintended detections
├─ Data corruption: CRC mismatch on any record
└─ Timing errors: >100 ms latency or >20 ms timestamp jitter
```

### 5.2 Thermal Drift Compensation Test

```
Test: Verify barrel temperature monitoring and compensation

Equipment:
├─ Temperature chamber (capable of -10 to +100°C)
├─ Thermocouple probe (reference thermometer, ±0.5°C)
├─ Board mounted in thermal enclosure with airflow
└─ UART monitor to log temperature readings

Procedure:
1. Initialize board at room temperature (25°C ± 2°C)
2. Let sensors equilibrate for 5 minutes
3. Apply temperature ramps:
   ├─ RAMP 1: 25°C → 50°C (simulates barrel heating after 50 shots)
   │  └─ Measure: MCP9808 temperature every 10 seconds
   │  └─ Expected: Linear rise, ±1°C accuracy vs. reference
   ├─ RAMP 2: 50°C → 75°C (elevated operating condition)
   │  └─ Check: Anomaly detector does NOT flag thermal runaway (1.3x baseline)
   └─ RAMP 3: 75°C → 45°C (cool down, post-session)
      └─ Verify: Hysteresis handling (no oscillation at threshold)

4. Data capture (UART log):
   ```
   [00:00:05] Thermal baseline initialized: 25.0°C
   [00:01:23] Barrel temp: 30.2°C (ΔT = +5.2°C)
   [00:02:45] Barrel temp: 35.1°C (ΔT = +10.1°C)
   [00:04:12] Barrel temp: 40.3°C (ΔT = +15.3°C)
   ...
   [00:15:30] Barrel temp: 75.0°C (ΔT = +50.0°C)
   [00:15:31] Anomaly check: Normal (baseline 25.0°C, 1.3x = 32.5°C < 75.0°C → WARN)
   ```

Acceptance criteria:
├─ Accuracy: ±1°C vs. thermocouple reference
├─ Response time: <2 seconds to reach new temperature
├─ No spikes: Noise <0.5°C RMS
├─ Baseline tracking: Slow drift (EWMA alpha=0.05) should adapt over minutes
└─ Anomaly detection: Correct threshold triggers

FAIL if:
├─ Error >±2°C (sensor calibration issue)
├─ Erratic jumps: Noise >2°C (EMI or connection issue)
├─ Unresponsive: No temperature change after 10 minutes
└─ Anomaly detector stuck: Never flags warning even at 100°C
```

### 5.3 Watchdog Recovery Test

```
Test: Verify IWDG resets frozen MCU

Procedure:
1. Boot board normally (expect clean startup messages)
2. Send firmware command to trigger watchdog timeout:
   ├─ Method A: Deliberate infinite loop (no watchdog strobe)
   ├─ Method B: Disable interrupts and wait 6 seconds
   └─ Method C: Halt MCU via JTAG and timeout
3. Observe behavior:
   ├─ After 5 seconds: NRST pin should pulse low briefly
   ├─ Board auto-resets: Watchdog-triggered reset (not user-initiated)
   ├─ Boot messages reappear: Same initialization sequence
   └─ No data loss: Previous session log still intact in Flash

Expected NRST signature (oscilloscope):
├─ Pulse width: 10-100 µs (short, hard reset)
├─ Return to high: Smooth rise (RC filter), no oscillation
├─ No spikes: Clean pulse, no noise

PASS if:
├─ Single NRST pulse (no chatter)
├─ Board recovers to STATE_IDLE
├─ No corruption of previous session data
└─ Watchdog re-armed after recovery

FAIL if:
├─ No reset occurs (watchdog not armed)
├─ Repeated resets (watchdog loop, not strobing)
└─ Partial recovery (some peripherals not reinitialized)
```

---

## 6. Environmental Testing (Optional, Prototype Phase)

### 6.1 Temperature Cycling

```
Condition: -10°C to +85°C, 10 cycles (1 hour per temperature)

Procedure:
1. Place board in thermal chamber
2. Cycle temperature: -10°C → +85°C → -10°C (repeat 10x)
3. Dwell time: 1 hour at each extreme
4. Measure voltages and currents every 15 minutes
5. Verify no thermal shorts (resistance shouldn't drop below 1kΩ)

PASS if:
├─ All voltage rails stable (±5% VCC_3.3V) during cycle
├─ No thermal runaway (current doesn't spike unexpectedly)
├─ No mechanical stress (no solder cracks on flex points)
└─ No intermittent failures (all tests still pass after cycling)
```

### 6.2 Humidity Test (Moisture Stress)

```
Condition: 85% RH at 85°C for 48 hours (IPC-TM-650 2.6.3.3)

Procedure:
1. Place board in humidity chamber
2. Maintain: 85°C, 85% RH for 48 hours
3. Test continuity and isolation before/after
4. Check for condensation on crystal or sensitive areas

PASS if:
├─ Insulation resistance: >1MΩ between all traces
├─ No visible corrosion (copper traces, IC leads)
├─ No swelling of capacitors
└─ All tests pass after drying (24 hours at room temp)
```

---

## 7. Production Acceptance Test (PAT)

All boards must pass before shipment:

```
┌─────────────────────────────────────────────────────┐
│ PRODUCTION ACCEPTANCE TEST (PAT)                    │
│ STM32H745 Ballistic Corrector PCB                  │
└─────────────────────────────────────────────────────┘

Serial Number: ________________   Date: ________________

[ ] 1. Visual inspection: No solder bridges, cracks, discoloration
[ ] 2. Power supply: VCC_3V3 = 3.30V ±0.16V (measured 3 points)
[ ] 3. UART output: Boot messages appear, 921600 baud clean
[ ] 4. IMU detection: SPI reads ID register = 0x71
[ ] 5. Flash detection: QSPI reads JEDEC = 0xEF 0x40 0x18
[ ] 6. I2C sensors: LRF at 0x29, Temp at 0x50 respond
[ ] 7. Watchdog: Forced timeout → NRST pulse observed
[ ] 8. No hot spots: Thermal imaging <50°C after 5 min run
[ ] 9. Continuity check: All power traces <0.1Ω

RESULT:    [ ] PASS    [ ] FAIL (describe issue: _______________)

Test Engineer: ___________________   Date: ________________
```

---

## 8. Troubleshooting Guide

| Symptom | Root Cause | Test to Confirm | Fix |
|---------|-----------|-----------------|-----|
| No UART output | CP2102N not programmed or connected | Check USB enumeration | Reprogram driver or replace chip |
| Garbled UART output | Baud rate mismatch | Measure UART timing | Verify PLL frequency (480 MHz check) |
| IMU not responding | SPI connection broken or CS stuck | Scope SPI bus | Reflow SPI trace joints, check CS pull-up |
| Flash not detected | QSPI CS not toggling | Logic analyzer on CS pin | Verify QSPI GPIO configuration |
| Board draws >500mA | Short circuit, usually on power rail | IR thermal image | Inspect for solder bridges with magnifier |
| Watchdog never fires | IWDG not strobed in firmware | Trigger deliberate timeout | Check sensor_fusion_task running |
| Temperature sensor stuck | I2C bus held low by slave | Oscilloscope on SCL/SDA | Power-cycle board, check pull-ups |

---

**End of Test Specification**  
**Next step:** Fabricate PCB prototype, run PAI/PAT, iterate design based on results
