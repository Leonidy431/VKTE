# STM32H745 Power Supply Sequencing Specification

**Rev**: 1.0  
**Date**: 2026-08-18  
**Status**: ACTIVE (Critical for P0-13 blind spot remediation)

---

## Executive Summary

STM32H745 dual-core processor (Cortex-M7 @ 480 MHz + Cortex-M4 @ 240 MHz) requires strict power supply sequencing to avoid:
- Analog supply (Vdda) rising before logic supply (Vdd) → ADC garbage sampling
- Inrush current >2A on cold start → voltage sag → brownout reset triggered prematurely
- Thermal runaway during RF TX burst (>10W dissipation in 100ms pulse)

This specification defines boot sequencing, power thresholds, and firmware validation for production readiness.

---

## 1. Power Supply Architecture

### 1.1 Required Supply Rails

| Rail | Nominal | Min | Max | Source | Notes |
|------|---------|-----|-----|--------|-------|
| **Vbat** | 3.3V | 3.0V | 3.6V | Battery/USB | Backup supply for RTC; first to stabilize |
| **Vdd** | 3.3V | 2.97V | 3.6V | Buck converter or battery | Logic core supply; 480 MHz M7 runs here |
| **Vdda** | 3.3V | 2.97V | 3.6V | Filtered buck (LP filter) | Analog supply; ADC reference |
| **Vref+** | 3.3V | 2.97V | 3.6V | Precision reference (TL431 or LDO) | ADC Vref; ±5% tolerance critical |
| **Vddio** | 3.3V | 2.97V | 3.6V | Buck converter | I/O supply (GPIO, SPI, I2C, UART) |

### 1.2 Power Distribution Network (PDN) Design

```
Battery/USB 5V Input
    |
    +-- Bulk Capacitor (1000 µF, ceramic, ESR <50 mΩ)
    |
    +-- Buck Converter (TPS54160 or equiv., 2A capable)
         |
         +-- Vdd Bulk (100 µF ceramic + 10 µF ceramic) [Vdd rail]
         |    |
         |    +-- Ceramic X7R caps (10 µF ×4 near M7 core, < 5mm distance)
         |    |
         |    +-- 1Ω current-limiting resistor on Vdd input (limits di/dt to <2 A/µs)
         |
         +-- Vdda Filter (100 µF ceramic + ferrite bead + RC low-pass)
              |
              +-- Ferrite Bead: Z @ 100 MHz >100Ω (Murata BLM18 or equiv.)
              |
              +-- R=10Ω + C=1µF RC network (corner frequency ~15 kHz)
              |
              +-- Ceramic X7R caps (22 µF ×2 for Vdda decoupling)

Vref+ (if external reference IC):
   TL431 shunt regulator or low-noise LDO
   +-- 10µF bulk + 100nF ceramic
   (Integrates into ADC Vref input)
```

**Rationale**: 
- Bulk capacitor handles sudden current spikes (transients)
- Ceramic caps provide high-frequency noise filtering
- 1Ω series resistor on Vdd limits inrush di/dt; adds 330mV drop at 2A (acceptable: 3.3V − 0.33V = 2.97V still above brownout)
- Vdda separation with ferrite + RC ensures ADC sampling stability during RF TX bursts
- Vref+ isolated supply prevents RF noise coupling into ADC reference

---

## 2. Boot Sequencing

### 2.1 Hardware-Level Sequencing (Schematic/Layout)

Power-on sequence is naturally enforced by buck converter ENABLE pin sequencing (if implemented with managed sequencer IC like TPS40210).

**Manual sequencing (if using discrete buck converters)**:
1. Vbat first (RTC supply, always on when battery inserted)
2. Wait 50 ms (internal capacitors charge)
3. Enable Vdd buck converter (logic core)
4. Wait 10 ms (Vdd stabilizes to nominal)
5. Enable Vdda buck converter (analog supply MUST follow Vdd by ≥10 ms)
6. Wait 200 ms total from power-on
7. Release NRST (reset pin) to high; M7/M4 cores begin boot from FLASH

### 2.2 STM32H745 ROM Boot Code

After NRST release, hardware ROM code:
1. Initializes core clocks (HSI = 16 MHz default)
2. Executes user bootloader (if programmed in FLASH sector 0)
3. Jumps to main() at FLASH offset 0x8000_0000

**Critical timing**: 
- M7 and M4 cores start **simultaneously** after NRST release (not sequentially)
- Both cores fetch first instruction from same FLASH bank (no stalling)
- Shared AXI-SRAM (0x2007C000–0x2007FFFF, 48 KB) is zero-initialized by hardware

### 2.3 Firmware Boot Sequence (main() / m4_main())

**M7 Core Boot (main.c)**:
```c
int main(void) {
    // Step 1: Hardware init (clocks, UART, sensors)
    if (hardware_init() != 0) {
        system_state = STATE_ERROR;
        printf("FATAL: Hardware init failed\n");
        while(1);  // Halt if hardware broken
    }
    printf("M7: Hardware initialized (480 MHz)\n");

    // Step 2: RTOS initialization (enable SMP, launch M4 task)
    if (rtos_init() != 0) {
        system_state = STATE_ERROR;
        printf("FATAL: RTOS init failed\n");
        while(1);
    }

    // Step 3: Start FreeRTOS scheduler
    printf("M7: FreeRTOS starting...\n");
    vTaskStartScheduler();  // Never returns

    // Unreachable
    return -1;
}
```

**M4 Core Boot (m4_core.c)**:
```c
int m4_main(void) {
    printf("\n=== STM32H745 M4 Core Boot (240 MHz) ===\n");

    // Initialize IPC ring buffer (M7 may already be filling it)
    ipc_init();
    printf("M4: IPC ring initialized\n");

    // Launch M4 logging task (infinite loop)
    printf("M4: Launching logging task...\n");
    m4_logging_task(NULL);

    return -1;  // Never reached
}
```

**Boot Validation Console Output (Expected)**:
```
=== STM32H745 Ballistic Corrector v1.0.0 ===
M7 Core: 480 MHz, M4 Core: 240 MHz
Initializing sensors...
  ✓ IMU initialized
  ✓ LRF initialized
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
```

---

## 3. Brownout Reset Specification

### 3.1 Hardware Brownout Detection

STM32H745 includes BOR (Brown-Out Reset) circuit with configurable thresholds:

| Level | Vdd Threshold | Function | Recommendation |
|-------|---------------|----------|-----------------|
| **LEVEL0** | ~2.1V | POR (Power-On Reset) | Not recommended; too low |
| **LEVEL1** | ~2.4V | Legacy | Not recommended; lacks headroom |
| **LEVEL2** | ~2.7V | **(RECOMMENDED)** | Safe for this design; headroom above ~2.5V datasheet minimum |
| **LEVEL3** | ~2.9V | Aggressive; limits battery range | Use only if supply is clean >3.0V always |

**Selected**: **LEVEL2 (2.7V threshold)**
- Allows operation down to ~2.5V (safe margin above minimum logic levels)
- Triggers reset if battery sags below 2.7V during RF TX current draw
- Leaves 600 mV headroom before soft shutdown required

### 3.2 Firmware Brownout Handler

```c
/**
 * Brownout Early Warning ISR
 * Called when Vdd drops to warning threshold (~2.8V, 100 mV above BOR trigger)
 */
void BOR_Warning_IRQHandler(void) {
    // Log warning
    printf("[POWER] WARNING: Brownout threshold approaching (Vdd ~2.8V)\n");
    
    // Stop RF TX immediately
    hackrf_shutdown();
    
    // Flush critical data to Flash before hard reset
    session_flush_to_flash();
    
    // Set low-power flag
    system_state = STATE_LOW_POWER;
    
    // Optional: Trigger immediate shutdown to avoid reset corruption
    // power_down();
}

/**
 * Brownout Reset Interrupt (if BOR event itself is signaled)
 * Called when Vdd crosses BOR threshold (hard reset imminent)
 */
void BOR_Reset_IRQHandler(void) {
    printf("[POWER] CRITICAL: Brownout reset triggered (Vdd < 2.7V)\n");
    
    // Attempt emergency flush before hardware forces reset
    session_flush_to_flash();
    
    // System will reset in <1 ms
    while(1);  // Wait for reset
}
```

### 3.3 Battery Low-Power Shutdown

If no BOR warning ISR available, firmware monitors ADC (connected to battery voltage divider):

```c
/**
 * Task: Monitor battery voltage (runs every 1 second at low priority)
 */
void battery_monitor_task(void *argument) {
    while(1) {
        // Read battery voltage via ADC (voltage divider Vbat/2 on ADC input)
        uint16_t adc_raw = adc_read_battery();
        float vbat_measured = adc_raw * (5.0f / 4095.0f) * 2.0f;  // Convert to Vbat
        
        if (vbat_measured < 2.9f) {
            // Battery critical; graceful shutdown
            printf("[POWER] Battery critical (%.2f V); shutting down gracefully\n", vbat_measured);
            
            // Stop RF operations
            hackrf_shutdown();
            
            // Flush all pending data
            session_flush_to_flash();
            
            // Sleep until external power restored
            HAL_PWR_EnterSTOPMode(PWR_MAINREGULATOR_ON, PWR_STOPENTRY_WFI);
            
            // Resume if power restored
            printf("[POWER] Power restored; resuming\n");
        }
        
        osDelay(1000);  // Check every 1 second
    }
}
```

---

## 4. RF TX Power Management

### 4.1 Peak Current During TX

**RF TX Burst Profile**:
- Frequency: 915 MHz ISM band
- Power: +10 dBm (~10 mW)
- Duration: 100 ms pulses, 10% duty cycle (typical)
- Inrush current: HackRF turns on ≈ 300 mA spike

**Total System Current Draw**:
- STM32H745 baseline: 150 mA (M7 + M4 + peripherals)
- HackRF RX (idle): 80 mA
- HackRF TX (full power): 300 mA
- **Peak Total**: 150 + 300 = **450 mA**
- **Average (10% TX duty)**: 150 + (0.9 × 80 + 0.1 × 300) = ~190 mA

**Battery Voltage Sag During TX**:
- Battery resistance + connector + PCB trace resistance ≈ 0.5Ω
- Voltage drop = 450 mA × 0.5Ω ≈ **225 mV sag**
- Vbat at peak TX: 3.3V − 0.225V = **3.075V** (still safe, above BOR trigger of 2.7V)

### 4.2 Thermal Throttling

During sustained RF TX (>30 sec continuous):

```c
/**
 * Task: Thermal Monitor (runs every 100 ms)
 */
void thermal_monitor_task(void *argument) {
    NTC_Thermistor ntc;
    ntc_init(&ntc);  // 10k NTC on ADC
    
    while(1) {
        float die_temp_c = ntc_read_temperature(&ntc);
        
        if (die_temp_c > 85.0f) {
            // WARN: Approaching max rated junction temperature
            printf("[THERMAL] WARNING: Die temp %.1f°C (limit 105°C)\n", die_temp_c);
            // Reduce RF TX power by 3 dB (software attenuation on HackRF)
            hackrf_set_tx_vga(HACKRF_TX_VGA_WARN);
        }
        
        if (die_temp_c > 95.0f) {
            // CRITICAL: Thermal shutdown imminent
            printf("[THERMAL] CRITICAL: Die temp %.1f°C; suspending RF TX\n", die_temp_c);
            hackrf_shutdown();
            system_state = STATE_THERMAL_THROTTLE;
        }
        
        osDelay(100);
    }
}
```

---

## 5. Cold-Start Validation Testing

### 5.1 Test Procedure: Boot from Low Battery

**Setup**:
1. Connect STM32H745 PCB to adjustable DC power supply (set to 2.5V initially)
2. Connect UART debug interface (115200 baud, 8N1)
3. Connect logic analyzer to NRST (reset pin) for timing capture

**Test Steps**:
1. **Slow Ramp Startup** (mimics low-battery recovery):
   - Set supply to 2.5V
   - Slowly ramp voltage: 2.5V → 3.3V over 5 seconds
   - Observe UART boot messages
   - **Target**: Cores boot cleanly, no garbled UART output, both "M7 Online" and "M4 Online" messages appear

2. **Fast Startup** (normal power-on):
   - From 2.5V, jump supply to 3.3V in 10 µs
   - Measure voltage on NRST pin (should remain low ~1 ms during POR)
   - **Target**: Reset pulse >1 ms, cores start cleanly

3. **Inrush Current Limit**:
   - Measure supply current with oscilloscope current probe (1Ω sense resistor)
   - Peak inrush should NOT exceed 2.5A (safety margin below PSU limit)
   - **Target**: Peak inrush <2A, dI/dT <2 A/µs

4. **Brownout Recovery**:
   - Boot normally at 3.3V
   - While system running, momentarily drop supply to 2.8V (1 second pulse)
   - Firmware should detect low-battery condition and log warning
   - System should NOT crash; should gracefully handle recovery
   - **Target**: No crashes, "Battery critical" message appears in UART log

### 5.2 Test Acceptance Criteria

| Test | Metric | Target | Result |
|------|--------|--------|--------|
| Slow ramp | Boot messages complete | Both "M7 Online" + "M4 Online" in <2 sec | ⏳ PENDING |
| Fast startup | NRST pulse duration | >1 ms, <10 ms | ⏳ PENDING |
| Inrush current | Peak current | <2A (limit 2.5A safety margin) | ⏳ PENDING |
| Inrush rate | dI/dT | <2 A/µs | ⏳ PENDING |
| Brownout recovery | No crashes on Vdd sag | System logs warning, continues | ⏳ PENDING |
| Thermal monitoring | Throttle on >85°C | RF TX disabled before 95°C limit | ⏳ PENDING |

---

## 6. Production Readiness Checklist

- [ ] **Schematic Review**: Power distribution network verified by PCB designer
- [ ] **Simulation**: SPICE simulation of inrush current with actual component models
- [ ] **PCB Layout**: Via stitching under Vdd planes, low-inductance return paths verified
- [ ] **Component Validation**: All capacitors, resistors, ferrite beads sourced and tested
- [ ] **Firmware Integration**: Boot sequence code in place (startup_sequence.c, power_management.c)
- [ ] **Cold-Start Testing**: 5.1 test procedure executed and passed on prototype
- [ ] **Thermal Profiling**: Die temperature under RF TX load measured and logged
- [ ] **Brownout Testing**: BOR trigger point verified at designed threshold
- [ ] **Field Deployment**: 24-hour continuous operation test in lab, no thermal or power issues
- [ ] **Documentation**: All assumptions and calibrations recorded in this document

---

**Next Steps**:
1. Create `startup_sequence.c` with system clock initialization (Phase 1)
2. Create `power_management.c` with brownout detection and thermal monitoring (Phase 2)
3. Integrate BOR warning ISR handler into FreeRTOS tick hook
4. Execute cold-start validation tests (Section 5.1) on prototype PCB (2026-08-23)
5. Update firmware with results; commit to branch

---

**Reviewed By**: Hardware Team  
**Approved By**: [Pending user sign-off]  
**Status**: DRAFT → ACTIVE upon firmware integration complete
