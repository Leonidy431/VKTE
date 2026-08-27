# STM32H745 Firmware Integration Test Plan
**Version**: 1.0  
**Date**: 2026-08-18  
**Status**: DRAFT → ACTIVE upon prototype boot  
**Gate for Prototype Assembly**: All 9 acceptance criteria MUST PASS before proceeding to Step 9 (KiCad P&P)

---

## 1. Boot Sequence Validation

### 1.1 Console Output Test
**Objective**: Verify correct hardware initialization sequence and clock configuration.

**Setup**:
1. Connect STM32H745 PCB to adjustable 3.3V DC power supply
2. Connect UART2 debug interface (115200 baud, 8N1) to host terminal (e.g., `picocom /dev/ttyUSB0 -b 115200`)
3. Ensure FreeRTOS scheduler and both cores (M7, M4) are compiled into the firmware

**Procedure**:
1. Power on board (ramp supply 0→3.3V in 500 ms or DC constant)
2. Observe UART console output in real-time
3. Record all console messages and timestamps
4. Compare against expected sequence (see below)

**Expected Console Output** (all lines in order, <2 sec total):
```
=================================
STM32H745 Ballistic Corrector v1.0.0
M7 Core: 480 MHz, M4 Core: 240 MHz
Initializing sensors...
✓ IMU initialized (ICM-20689 via SPI1)
✓ LRF initialized (VL53L0X via I2C1)
✓ Magnetometer initialized (AK8963)
Hardware initialization complete.
System ready (STATE_IDLE)

Initializing FreeRTOS...
RTOS initialization complete.
Starting FreeRTOS scheduler...

=== STM32H745 M4 Core Boot ===
M4: 240 MHz, AXI-SRAM shared with M7
M4: IPC ring initialized
M4: Launching logging task...

[M4 Status]
State: 1 (IDLE)
Shots logged: 0
Flush count: 0
IPC queue depth: 0 / 256
=================================
```

**PASS Criterion**:
- ✅ All 8 key messages appear in correct order
- ✅ Total boot time <2.0 seconds
- ✅ No garbled UART output or character corruption
- ✅ No missing lines or incomplete initialization

**FAIL Criterion**:
- ❌ Any message missing or out of order
- ❌ Boot timeout >3 seconds
- ❌ Corrupted UART output (invalid characters, line breaks)
- ❌ Exception or hard fault logged

---

### 1.2 Inrush Current Limit Test
**Objective**: Verify 1Ω current-limiting resistor on Vdd prevents excessive inrush current.

**Setup**:
1. Measure current on Vdd via 1Ω current sense resistor (in series with Vdd supply)
2. Connect oscilloscope with current probe (AC-coupling, 1 mV/mA sensitivity)
3. Set multimeter to measure Vdd voltage in parallel

**Procedure**:
1. **Slow Ramp Test** (mimics low-battery recovery):
   - Set power supply to 2.5V initially
   - Ramp voltage slowly: 2.5V → 3.3V over 5 seconds
   - Record current waveform on oscilloscope
   - Observe peak inrush current and di/dt slope

2. **Fast Startup Test** (normal power-on):
   - Reset supply to 2.5V
   - Jump supply to 3.3V in <10 µs (hard step)
   - Measure voltage on NRST pin (should remain low ~1 ms during POR)
   - Record peak inrush current and overshoot

**PASS Criterion**:
- ✅ Peak inrush current <2.0 A (rated supply limit 2.5 A with 500 mA safety margin)
- ✅ di/dt <2 A/µs (1Ω series resistor provides RC filtering; target di/dt = ΔI / Δt where ΔI = 2A, Δt > 1µs)
- ✅ No voltage overshoot >3.6V on Vdd (maximum rated voltage)
- ✅ No oscillations or ringing >500 mA after initial spike

**FAIL Criterion**:
- ❌ Peak inrush >2.5 A (exceeds PSU limit, potential brownout)
- ❌ di/dt >2 A/µs (fast transients cause voltage sag, early brownout trigger)
- ❌ Vdd overshoot >3.6V (damages analog circuits, ADC reference)
- ❌ Sustained current >1.0 A for >100 ms (thermal runaway)

---

### 1.3 Brownout Reset Detection
**Objective**: Verify BOR threshold detection and graceful firmware shutdown.

**Setup**:
1. Board powered at 3.3V, firmware running normally
2. UART console connected and logging to file
3. Adjustable DC power supply with <100 ms response time for voltage dips

**Procedure**:

**Step 1 — Normal Boot:**
- Monitor UART for "System ready (STATE_IDLE)" message
- Verify clean NRST pulse >1 ms on oscilloscope
- Baseline established

**Step 2 — Brownout Warning Simulation:**
- While system running, dip Vdd to 2.8V for 100 ms, then release
- Expected: UART logs "[POWER] WARNING: Brownout threshold approaching (Vdd ~2.8V)"
- Firmware should NOT crash; STATE remains IDLE or transitions to STATE_LOW_POWER
- RF TX (if active) should be disabled by hackrf_shutdown()

**Step 3 — Brownout Reset Simulation:**
- Dip Vdd to 2.6V (below 2.7V BOR threshold) for 50 ms, then release
- Expected: UART logs "[POWER] CRITICAL: Brownout reset triggered (Vdd < 2.7V)"
- Hardware resets within 1 ms (NRST pin pulled low)
- System re-boots: "System ready" message appears again

**PASS Criterion**:
- ✅ Brownout warning message logged at 2.8V ±0.1V dip
- ✅ Brownout critical message logged at 2.6V ±0.1V dip
- ✅ NRST pulse width >1 ms, <10 ms during reset
- ✅ No crashes or watchdog resets (clean log on resume)
- ✅ System recovers cleanly after power restoration

**FAIL Criterion**:
- ❌ No warning message (threshold not detected)
- ❌ No critical message (reset ISR not triggered)
- ❌ NRST pulse <1 ms or >10 ms (timing outside spec)
- ❌ System crashes or enters exception handler
- ❌ Warning/critical messages arrive >2 sec after voltage dip (latency issue)

---

### 1.4 Thermal Monitoring
**Objective**: Verify NTC thermistor reading and thermal throttling logic.

**Setup**:
1. NTC thermistor mounted on or near STM32H745 die, via ADC input
2. Heating lamp or heat gun for temperature control
3. Calibrated thermometer (IR or contact) for reference temperature
4. Oscilloscope to measure RF TX power (if available)

**Procedure**:

**Baseline (Room Temperature, ~25°C):**
- Monitor UART: thermal task should log "die temp ~25°C" every 100 ms
- Verify UART message frequency: ~10 messages/sec

**Ramp to Thermal Warning (85°C):**
- Gradually heat die to 85°C (±3°C tolerance)
- Expected: UART logs "[THERMAL] WARNING: Die temp 85.0°C (limit 105.0°C)"
- RF TX power should remain full (no attenuation yet)

**Ramp to Thermal Critical (95°C):**
- Continue heating to 95°C (±3°C tolerance)
- Expected: UART logs "[THERMAL] CRITICAL: Die temp 95.0°C; suspending RF TX"
- RF TX should shut down immediately: hackrf_shutdown() called
- Oscilloscope should show RF TX current drop >100 mA within 10 ms

**Cool Down (<85°C):**
- Allow to cool below 85°C
- Verify UART logs no longer show warnings
- If RF TX requested, resume normally

**PASS Criterion**:
- ✅ Thermal warning triggered at 85°C ±3°C
- ✅ Thermal shutdown triggered at 95°C ±3°C
- ✅ RF TX disabled immediately upon shutdown (current drop verified)
- ✅ System resumes RF operation after cooldown
- ✅ No false positives or missed thresholds

**FAIL Criterion**:
- ❌ Thresholds triggered at wrong temperatures (>±5°C deviation)
- ❌ RF TX not disabled at 95°C (continues transmitting)
- ❌ System crashes or enters exception during thermal events
- ❌ No thermal task output (logging not working)

---

## 2. IPC Ring Buffer Validation

### 2.1 M7→M4 Enqueue/Dequeue Test
**Objective**: Verify inter-core communication ring buffer preserves shot events.

**Setup**:
1. Firmware compiled with M7 sensor fusion task + M4 logging task
2. Trigger generation: Use IMU or external pulse to create shot events (100 shots)
3. Monitor M4 logging task output via UART

**Procedure**:
1. Start system at idle
2. M7 core processes 100 shots, enqueues each to IPC ring
3. M4 core drains ring asynchronously, logs to Flash
4. Verify UART output: "M4 Flushing batch of 50 shots to Flash" (every 50 shots)

**Expected Behavior**:
- M7 enqueues 100 shots with shot_id = 0, 1, 2, ..., 99
- M4 dequeues all 100 in FIFO order (0, 1, 2, ..., 99)
- After first 50 shots: "M4 Flushing batch of 50" appears
- After all 100: "M4 Flushing batch of 50" appears again
- IPC queue depth returns to 0

**PASS Criterion**:
- ✅ All 100 shots enqueued successfully (ipc_enqueue_shot() returns 0)
- ✅ All 100 shots dequeued in FIFO order (shot_id 0→99)
- ✅ m4_get_ipc_queue_depth() = 100 initially, drops to 0 after drain
- ✅ Flash flush messages logged: "Flushing batch of 50" × 2
- ✅ No queue overflow or underflow errors

**FAIL Criterion**:
- ❌ ipc_enqueue_shot() returns −1 (queue full prematurely)
- ❌ Shots dequeued out of order or with gaps
- ❌ Queue depth mismatch (reports 100 but only 50 in queue)
- ❌ Missing flush messages (batch threshold not reached)

---

### 2.2 Ring Buffer Wrap-Around Test
**Objective**: Verify modulo arithmetic handles index wrap-around at 256-slot boundary.

**Setup**:
1. Same as 2.1, but with 256+ shots to force wrap-around

**Procedure**:
1. M7 enqueues 256 shots (filling entire ring)
2. M4 drains 128 shots
3. M7 enqueues 128 more shots (fills ring again, wrapping indices)
4. Verify all 256 shots + 128 more dequeued correctly

**Expected Behavior**:
- Shots 0–255 enqueued: queue_depth = 255 (N-1 since one slot reserved)
- Shot 256 rejected: ipc_enqueue_shot() returns −1 (queue full)
- After drain of 128: queue_depth = 127
- Shots 256–383 enqueued: queue_depth = 255 again
- All shots dequeued in FIFO order: 0, 1, ..., 255, 256, 257, ..., 383

**PASS Criterion**:
- ✅ Exactly 255 shots accepted on first fill (N-1 effective capacity)
- ✅ 256th shot rejected (full)
- ✅ Wrap-around indices calculated correctly (modulo 256)
- ✅ FIFO order preserved across wrap point (255→256)
- ✅ No data corruption or off-by-one errors

**FAIL Criterion**:
- ❌ More or fewer than 255 shots accepted initially
- ❌ Wrap-around corruption (shot_id mismatch)
- ❌ Queue depth calculation incorrect after wrap

---

## 3. Flash Persistence Validation

### 3.1 Session Log Flush Test
**Objective**: Verify shot events are correctly written to Flash storage.

**Setup**:
1. Flash storage: STM32 onboard FLASH (or external SPI Flash if applicable)
2. Session ID assigned (e.g., session_id = 1, ammo_type = .308 Winchester)
3. M7 assembles and logs 10 shots

**Procedure**:
1. Start session: start_session(1, 308)
2. M7 processes 10 shots (simulated via direct assembler calls or IMU input)
3. Manually trigger: session_flush_to_flash()
4. Read Flash memory region and parse ShotRecord structure
5. Verify all 10 shots persisted with correct timestamps

**PASS Criterion**:
- ✅ session_flush_to_flash() returns success (rc > 0 = shots flushed)
- ✅ UART logs "INFO: Flushed 10 shots to Flash"
- ✅ Flash read-back confirms all 10 shot records present
- ✅ Timestamps are monotonically increasing
- ✅ CRC/checksum validation passes (if implemented)

**FAIL Criterion**:
- ❌ session_flush_to_flash() returns error (rc < 0)
- ❌ Flash write timeout or partial write
- ❌ Shot records corrupted or unreadable on read-back
- ❌ CRC mismatch (data corruption)

---

### 3.2 Brownout Flush Test
**Objective**: Verify data integrity during unexpected power loss.

**Setup**:
1. Board running with session active (session_id = 2)
2. M7 processing shots; M4 batching to Flash
3. 20 shots queued in IPC ring before induced brownout

**Procedure**:
1. Start session and queue 20 shots
2. While shots pending in IPC ring (not yet flushed), dip Vdd to 2.6V
3. Hardware resets within 1 ms (BOR triggered)
4. On restart: read Flash and verify all 20 shots persisted

**Expected Behavior**:
- Brownout ISR calls session_flush_to_flash()
- Pending 20 shots written to Flash before reset
- On restart: boot messages logged, then: "[POWER] Resuming after brownout"

**PASS Criterion**:
- ✅ Zero shot loss due to power failure
- ✅ Brownout ISR executes (<1 ms window)
- ✅ Flash write completes before hardware reset
- ✅ On restart: all 20 shots recoverable from Flash

**FAIL Criterion**:
- ❌ Shots dropped (fewer than 20 on read-back)
- ❌ Flash corruption or CRC failure
- ❌ Incomplete write (partial records)

---

## 4. Real-Time Performance

### 4.1 Sensor Fusion Latency
**Objective**: Verify M7 real-time performance budget (<20 ms measurement→estimate).

**Setup**:
1. IMU sampling at 1 kHz (1 ms period)
2. Oscilloscope or logic analyzer on debug GPIO pins to timestamp events
3. GPIO0: toggle on IMU_read() entry
4. GPIO1: toggle on ipc_enqueue_shot() exit

**Procedure**:
1. Measure time delta between GPIO0 rising edge and GPIO1 falling edge
2. Repeat 100 samples, collect latency distribution
3. Calculate: mean, p50 (median), p99 (99th percentile)

**Expected Latency Budget**:
- IMU_read(): <1 ms (SPI transfer)
- Kalman filter (3 axes): <2 ms
- shot_assembler_process(): <1 ms
- ipc_enqueue_shot(): <5 µs
- **Total**: <5 ms (target), <10 ms (acceptable), >15 ms (FAIL)

**PASS Criterion**:
- ✅ Mean latency <5 ms
- ✅ p99 latency <10 ms
- ✅ No missed samples (1 kHz continuous)
- ✅ No jitter >1 ms (consistent timing)

**FAIL Criterion**:
- ❌ Mean latency >15 ms (exceeds budget)
- ❌ Jitter >3 ms (inconsistent timing, queue overflow likely)
- ❌ Missed samples (gaps in IMU input)

---

### 4.2 M4 Logging Task Throughput
**Objective**: Verify M4 can keep up with M7 shot generation (>50 shots/sec).

**Setup**:
1. M7 enqueues 50 shots/sec continuously (simulated or real)
2. M4 drains batch every 1 second
3. Monitor: batch_count, flush_count, queue_depth

**Procedure**:
1. Enable high-frequency shot generation (50 Hz)
2. Let system run for 10 seconds
3. Observe UART logs:
   - "Flushing batch of 50 shots" messages every ~1 sec (expected 10 total)
   - m4_get_flush_count() should be ~10
   - Queue depth should remain <50 (never close to overflow at 255 limit)

**PASS Criterion**:
- ✅ Throughput >50 shots/sec maintained
- ✅ m4_get_flush_count() increments consistently
- ✅ IPC queue depth stays <100 (headroom available)
- ✅ No "IPC queue full" warnings

**FAIL Criterion**:
- ❌ Queue fills to 255 (M4 not keeping up)
- ❌ Shots dropped (ipc_enqueue_shot() returns −1)
- ❌ Flush count stalled (logging task hung)

---

## 5. Acceptance Criteria Summary

**Gate for Prototype Assembly**: All METRICS must achieve PASS status.

| # | Test | Metric | Target | Result |
|---|------|--------|--------|--------|
| 1 | Console Boot | All 8 messages in <2 sec | ✓ | ⏳ PENDING |
| 2 | Inrush Current | Peak <2A, di/dt <2 A/µs | ✓ | ⏳ PENDING |
| 3 | Brownout Warning | Logged at 2.8V drop | ✓ | ⏳ PENDING |
| 4 | Brownout Reset | Logged at 2.6V, reset <1 ms | ✓ | ⏳ PENDING |
| 5 | Thermal Warning | Logged at 85°C ±3°C | ✓ | ⏳ PENDING |
| 6 | Thermal Shutdown | RF TX disabled at 95°C ±3°C | ✓ | ⏳ PENDING |
| 7 | IPC FIFO | 100 shots in order, no loss | ✓ | ⏳ PENDING |
| 8 | Flash Persistence | 20-shot loss prevention on brownout | ✓ | ⏳ PENDING |
| 9 | M7 Latency | <5 ms sensor→enqueue (p99 <10 ms) | ✓ | ⏳ PENDING |

---

## 6. Execution Schedule

- **2026-08-21 (Prototype Day)**: 4–6 hour test session on physical hardware
- **2026-08-21 18:00 UTC**: All 9 metrics evaluated; PASS/FAIL verdict
- **Gate**: Prototype assembly proceeds only if all 9 metrics PASS
- **Escalation**: Any FAIL → root-cause analysis, firmware patch, re-test before assembly

---

## Sign-Off

| Role | Name | Date | Signature |
|------|------|------|-----------|
| Firmware Lead | TBD | 2026-08-21 | ☐ |
| Hardware Lead | TBD | 2026-08-21 | ☐ |
| Project Manager | TBD | 2026-08-21 | ☐ |

---

**Document History**:
- v1.0 (2026-08-18): Initial draft, 5 test categories, 9 acceptance criteria, schedule

