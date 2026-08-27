# STM32H745 Ballistic Corrector — Hardware Design Guide

**Version:** 1.0  
**Date:** 2026-07-28  
**Status:** Production-Ready Schematic & BOM  
**Target:** High-precision ballistic analysis testbench for ammunition profiling

---

## 1. Executive Summary

This document describes the complete hardware design of the STM32H745-based ballistic corrector system. The design implements a dual-core real-time embedded system with:

- **Sensor fusion:** 9-axis IMU (ICM-20689) + laser rangefinder (VL53L0X) + thermal sensors
- **Signal processing:** Kalman filtering (M7 core, 480 MHz), session logging (M4 core, 240 MHz)
- **Storage:** 16 MB QSPI Flash (session logs) + 32 KB I2C EEPROM (configuration)
- **Communication:** 921.6 kbaud UART (debug), USB HS OTG (log download), Bluetooth LE (mobile app)
- **Safety:** Independent watchdog, reverse-polarity protection, CRC32 data integrity
- **Power:** 12V input, buck converter to 5V, dual LDO regulators for 3.3V and 1.8V

**Bill of Materials:** 13 active components, 28 passive components, 1 custom PCB  
**Estimated unit cost:** $377.81 (1-piece assembly)  
**PCB size:** ~80mm × 100mm (4-layer, 1.6mm FR-4)  
**Weight:** ~45g (including connectors)

---

## 2. Architecture Overview

### 2.1 Microcontroller: STM32H745ZIT6

The **STM32H745ZI** is a dual-core Cortex-M MCU featuring:

| Parameter | M7 Core | M4 Core | Notes |
|-----------|---------|---------|-------|
| **Clock** | 480 MHz | 240 MHz | Independent PLLs; can adjust independently |
| **ITCM** | 1 MB | 64 KB | Instruction tightly coupled memory (zero-wait access) |
| **DTCM** | 128 KB | 128 KB | Data TCM (cache-coherent writes) |
| **AXI-SRAM** | 512 KB | 512 KB | Shared inter-core memory (0x08000000-0x0807FFFF) |
| **CCM (DTCM_CM4)** | — | 64 KB | M4-only core coupled memory |
| **Flash (Internal)** | 1 MB | 1 MB | Sector-based, 32 KB erase units |
| **QSPI/Dual-SPI** | Yes | — | M7 owns QSPI; M4 accesses via shared memory |

**Selection rationale:**
- Dual-core enables **real-time isolation**: M7 handles sensor fusion (1 kHz deterministic), M4 handles non-blocking I/O (Flash, USB)
- High clock rate (480 MHz M7) supports demanding Kalman filter math without overrun
- 2 MB AXI-SRAM enables circular buffering for inter-core communication (IPC ring buffer)
- Quad-SPI Flash up to 108 MHz enables high-throughput logging (4x parallel data lines)

**Variant selection:** ZIT6 (144-pin LQFP) chosen for cost/performance trade-off:
- Z = 1 MB + 1 MB Flash (adequate for dual-core firmware)
- I = Industrial temperature range (-40 to +85 °C)
- T = LQFP-144 (0.5 mm pitch BGA alternative exists but adds cost)

---

### 2.2 Power Distribution

#### 2.2.1 Input Regulation

```
12V Barrel Connector (1A max) 
    ↓ [Schottky Reverse-Polarity Diode]
    ↓ [TPS62133A Buck Converter: 12V → 5V @ 1A]
    ├─→ 100nF + 10µF Bulk Decoupling
    └─→ Secondary Rails:
        ├─→ [LM1117-3.3: 5V → 3.3V @ 500mA] → All digital logic, sensors
        └─→ [LD1117-1.8: 3.3V → 1.8V @ 200mA] → IO bank VREF
```

**Design notes:**

1. **TPS62133A Buck Converter:**
   - Input: 12V (up to 16V transient-protected)
   - Output: 5V ± 2% at 1A continuous (1.2A peak)
   - Switching frequency: 2.2 MHz (reduces EMI/noise compared to 1 MHz)
   - Efficiency: ~93% at full load (12V → 5V @ 1A)
   - Soft-start prevents inrush current surge
   - Features: Over-current, thermal shutdown, enable pin for firmware power control

2. **3.3V Linear Regulator (LM1117-3.3):**
   - Input: 5V (from buck converter)
   - Output: 3.3V ± 4% at 500mA continuous
   - Dropout: 1.2V @ 500mA (requires 5V - 1.2V = 3.8V minimum input)
   - Cost: $0.45 per unit
   - PSRR: 60 dB @ 1 kHz (rejects buck converter ripple)
   - All digital rails (MCU core, sensors, flash) powered here

3. **1.8V Linear Regulator (LD1117-1.8):**
   - Input: 3.3V (from 3.3V rail)
   - Output: 1.8V ± 4% at 200mA
   - Dropout: 0.5V @ 200mA
   - Powers: VREF (ADC reference, if used), IO bank sense circuits
   - Optional in v1.0 (can tie to GND if 1.8V peripherals not used)

#### 2.2.2 Decoupling Strategy

All power supplies decoupled per IC datasheet:

| Rail | IC | Qty | Capacitors |
|------|----|----|-----------|
| **VCC (MCU core)** | STM32H745 | 1 | 100nF (×8) + 10µF (×2) |
| **VDDA (Analog)** | STM32H745 | 1 | 100nF + 10µF + 1µF |
| **VREF** | STM32H745 | 1 | 100nF + 1µF (separate plane) |
| **VCC (IMU)** | ICM-20689 | 1 | 100nF + 10µF |
| **VCC (LRF)** | VL53L0X | 1 | 100nF + 1µF |
| **VCC (Flash)** | W25Q128JV | 1 | 100nF + 10µF |
| **VCC (EEPROM)** | AT24C256C | 1 | 100nF |
| **VCC (Temp)** | MCP9808 | 1 | 100nF |

**Placement rules:**
- Capacitors placed <5mm from IC pin (minimize loop inductance)
- 100nF ceramic (low ESR, high-frequency): closest to pins
- 10µF electrolytics (bulk charge storage): mid-distance
- Additional 1µF ceramic on sensitive supplies (VREF, VDDA)

#### 2.2.3 Reverse-Polarity Protection

Input diode: **1N5819 Schottky**
- Forward voltage: 0.35V @ 1A (vs. 0.6V for silicon diode)
- Power dissipation at 1A: 0.35W (keeps input connector cool)
- Reverse recovery time: <4ns (fast, low EMI)
- Mounted in series with 12V input

**Alternative:** P-channel MOSFET (lower loss, higher cost):
- IRF9540: Vds = 100V, Rds(on) = 0.2Ω @ 1A
- Gate driven by: voltage divider or charge pump
- Not used here to minimize complexity

---

### 2.3 Clock & Reset Architecture

#### 2.3.1 Clock Tree

```
External Crystal (8 MHz)
  ├─ PLLSAI1 (for M7)
  │  Input: 8 MHz → ÷1 → ×60 → 480 MHz (M7 Core Clock)
  │  Output: PLLSAI1_P = 480 MHz (System Clock)
  │
  └─ PLLSAI2 (for M4)
     Input: 8 MHz → ÷1 → ×60 → 480 MHz → ÷2 → 240 MHz (M4 Core Clock)
     Output: PLLSAI2_Q = 240 MHz (System Clock for M4)
```

**Crystal Specification:**
- Frequency: 8.000 MHz ± 20 ppm (0.16 Hz error over 24 hrs)
- Load capacitance: 18 pF (typical for STM32)
- Equivalent series resistance (ESR): <100Ω
- Chosen: **SiTime SiT1533AI-82-33E-40.0-80** (MEMS oscillator, ±50 ppm, -40 to +85 °C)

**Load Capacitors:**
- C_load1 = 22 pF (PH0 to GND)
- C_load2 = 22 pF (PH1 to GND)
- Tolerance: ±1% (matched pair, 0.5 pF max difference)
- Type: 0603 X7R ceramic (stable temp coefficient)

**Advantages over 16 MHz:**
- Lower power dissipation (√proportional to frequency)
- Simplicity: ×60 multiplier cleaner than ×30 or ÷2×30
- Adequate margin for derivative speeds (if needed)

#### 2.3.2 Reset Circuit

```
NRST Pin (STM32H745)
   ↑
   │ Schmitt trigger input (internal)
   │
   ├─← RC Filter (Debounce)
   │   R = 10 kΩ (to VSS)
   │   C = 100 nF (to VSS)
   │   τ = 1 ms (debounce time)
   │
   └─← Reset Sources:
       ├─ Power-on reset (POR) @ 1.62V
       ├─ Brown-out reset (BOR) @ 2.5V
       ├─ Watchdog reset (IWDG/WWDG)
       ├─ Software reset (NVIC_SystemReset)
       └─ Manual reset button (SW1)
```

**Reset timing:**
- POR/BOR → MCU reset in <10 µs
- Boot sequence → Flash load → RTOS start: ~500 ms total
- Watchdog timeout: 5 seconds (strobed by sensor_fusion_task @ 10 Hz)

#### 2.3.3 Watchdog Configuration

Two independent watchdogs provide defense-in-depth:

1. **IWDG (Independent Watchdog):** Hardware counter, independent of MCU clock
   - Clock: LSI (internal 32 kHz oscillator, ±30% accuracy)
   - Timeout: 5 seconds (reload counter every 100 ms in firmware)
   - Trigger: Automatic NRST if not reloaded in time
   - Cannot be disabled by software (only by hardware strapping)

2. **WWDG (Window Watchdog):** Software-controlled, with window of validity
   - Window: Allows reload only in a specific time window
   - Timeout: 1-4 seconds (configurable)
   - Trigger: NRST or interrupt (firmware-selectable)

**Usage:**
- M7 sensor_fusion_task reloads IWDG every 100 ms (during main loop)
- If M7 hangs/deadlock: Watchdog fires, triggers NRST, system resets to safe state
- M4 logging task monitored separately via WWDG (window: 500-1000 ms)

---

## 3. Sensor Interface Design

### 3.1 IMU: ICM-20689 (SPI Interface)

**Function:** 9-axis motion tracking (3-axis accelerometer, 3-axis gyroscope, 3-axis magnetometer)

```
                STM32H745       ICM-20689 (LQFP-24)
                --------        -----------
SPI1_CLK    → PA5/PB3        → SCL
SPI1_MOSI   → PA6/PB4        → SDA
SPI1_MISO   → PA7/PB5        → AD0
SPI1_CS     → PB0            → CS/NCS
INT         → PB1 ← Interrupt output (falling edge)
GND         → VSS            → VSS
VCC_3V3     → VCC_3V3        → VDD
```

**Configuration:**
- SPI speed: 10 MHz (max for ICM-20689)
- Mode: SPI mode 0 (CPOL=0, CPHA=0)
- Data: 16-bit samples, MSB-first
- Sample rate: 1000 Hz (configurable down to 8 Hz)
- Interrupt: Data-ready (INT) on falling edge → triggers DMA transfer

**Signal Conditioning:**
- Series resistors (50Ω) on CLK/MOSI/MISO for EMI suppression
- Pull-up on CS (internal: 20kΩ)
- Decoupling: 100nF + 10µF on VDD

**Sampling strategy:**
- Timer interrupt at 1 kHz (SysTick or dedicated timer)
- DMA reads SPI FIFO → internal buffer
- Kalman filter processes buffer once per interrupt
- CPU latency: <100 µs (interrupt priority 0)

---

### 3.2 Laser Rangefinder: VL53L0X (I2C Interface)

**Function:** Time-of-flight distance measurement (30-1200 mm)

```
                STM32H745       VL53L0X (10-pin LGA)
                --------        -------
I2C1_SCL    → PB8            → SCL
I2C1_SDA    → PB9            → SDA
GPIO_INT    → PB7 ← Interrupt (open-drain)
GND         → VSS            → VSS/GND
VCC_3V3     → VCC_3V3        → VDD

I2C Address: 0x29 (default, can change via XSHUT pin)
Accuracy: ±25 mm @ 1200 mm
Update rate: 50 Hz (20 ms integration)
```

**I2C Pull-ups:**
- 10 kΩ resistors on SCL and SDA (to VCC_3V3)
- Bus capacitance: <100 pF (short PCB traces)
- Rise time: <300 ns (I2C Fast mode compatible)

**Power sequencing:**
- VDD ramp-up time: >10 ms (soft-start via regulator)
- Settling time: <50 ms before first measurement
- Device ID register read confirms correct part (0xEE for VL53L0X)

---

### 3.3 Barrel Temperature: MCP9808 (I2C Interface)

**Function:** High-resolution thermometer for thermal drift analysis

```
                STM32H745       MCP9808 (SOIC-8)
                --------        -------
I2C1_SCL    → PB8            → SCL
I2C1_SDA    → PB9            → SDA
A2, A1, A0  → GND            → Address pins (addr = 0x18)
CRIT        → NC             → Critical temperature output
GND         → VSS            → VSS
VCC_3V3     → VCC_3V3        → VDD

Update rate: 1 Hz (1 second measurement cycle)
Resolution: 0.0625°C (16-bit)
Accuracy: ±0.5°C @ 25°C
Range: -20 to +100°C
```

**Configuration:**
- Address: 0x18 (A2=0, A1=0, A0=0)
- Update mode: Continuous (default)
- Alert mode: Disabled (not used in this design)
- Resolution: 0.25°C minimum, set to full 16-bit (0.0625°C)

---

### 3.4 Ambient Conditions: BMP390 (SPI Interface)

**Function:** Barometric pressure and ambient temperature (for ballistic models)

```
                STM32H745       BMP390 (LGA-8)
                --------        ------
SPI1_CLK    → PA5/PB3        → SCK
SPI1_MOSI   → PA6/PB4        → SDI
SPI1_MISO   → PA7/PB5        → SDO
SPI1_CS2    → PC4            → CS (separate CS line from IMU)
INT         → PB10 ← Interrupt (optional)
GND         → VSS            → GND
VCC_3V3     → VCC_3V3        → VDDIO, VDDC

SPI Mode: Mode 3 (CPOL=1, CPHA=1)
Speed: 10 MHz
Update rate: 10 Hz
```

**I2C Alternative (if pin-constrained):**
- Address: 0x77 (default) or 0x76 (SDO tied to GND)
- Can share I2C1 bus with VL53L0X and MCP9808
- Latency: <10 ms for single measurement

---

## 4. External Memory Design

### 4.1 QSPI Flash: W25Q128JV (16 MB)

**Function:** Persistent storage for ballistic session logs (1-10 million shots per card)

```
STM32H745              W25Q128JV (SOIC-16)
--------               -----------
QSPI_CLK → PB2       → CLK
QSPI_D0  → PD11      → DO/DQ0 (Quad mode)
QSPI_D1  → PD12      → DQ1
QSPI_D2  → PE2       → DQ2
QSPI_D3  → PD13      → DQ3
QSPI_CS  → PA3       → CS (active low)
GND      → VSS       → GND
VCC_3V3  → VVDD      → VDD/VDDIO
NC       → WP        → /WP (tied to VDD: write-protected disabled)
```

**Performance:**
- Capacity: 128 Megabits (16 MB)
- Speed: 104 MHz SPI clock (Quad I/O mode: 4x parallelism)
- Effective throughput: 52 MB/s (4 data lines @ 104 MHz / 8 bits)
- Page size: 256 bytes
- Erase block: 4 KB / 32 KB / 64 KB sectors
- Typical erase time: 50 ms (4 KB), 200 ms (32 KB)

**Memory map (16 MB total):**
```
0x00000000 - 0x00FFFFFF (16 MB)
├─ 0x00000000 - 0x00FDFFFF (16256 KB) → Session logs (64M shots @ 4 bytes/shot)
├─ 0x00FE0000 - 0x00FEFFFF (64 KB)    → Configuration + session metadata
└─ 0x00FF0000 - 0x00FFFFFF (64 KB)    → Firmware backup (OTA update)
```

**Command set:**
- **0x03**: Read data (SPI mode, up to 25 MHz)
- **0x0B**: Fast read (SPI mode, up to 104 MHz)
- **0xEB**: Quad output fast read (QO mode, up to 104 MHz)
- **0x02**: Page program (256 bytes max per operation)
- **0x20**: Sector erase (4 KB)
- **0xD8**: Block erase (64 KB)
- **0xC7**: Chip erase (whole device, ~50 seconds)
- **0x05**: Read status register (polling for write-in-progress)

**Write endurance:**
- Min. 100,000 P/E cycles per cell (100k full-chip erases)
- Wear leveling via firmware: rotate write pointer to distribute wear
- Expected lifetime: 10+ years @ 100 shots/day

---

### 4.2 I2C EEPROM: AT24C256C (32 KB)

**Function:** Non-volatile configuration storage, calibration data, firmware recovery

```
STM32H745              AT24C256C (SOIC-8)
--------               -----------
I2C1_SCL → PB8       → A0 (addr pin 0)
I2C1_SDA → PB9       → A1 (addr pin 1)
GND      → VSS       → A2 (addr pin 2), GND
VCC_3V3  → VCC_3V3   → VDD
NC       → WP        → Write-protect pin (tied to GND: unprotected)
```

**Address:** 0xA0 (7-bit: 0x50, read-modify-write at 0xA0-0xAF)

**Memory map (32 KB total):**
```
0x0000 - 0x007F (128 bytes)   → System configuration (sampling rates, thresholds)
0x0080 - 0x00FF (128 bytes)   → Calibration data (IMU offsets, thermal baseline)
0x0100 - 0x7FFF (32512 bytes) → Firmware backup + recovery state
```

**Write cycle time:** 5 ms (internal write timer, then ready for next operation)
**Write endurance:** 1 million P/E cycles (adequate for firmware development)
**Access pattern:** I2C @ 400 kHz (Fast mode)

---

## 5. Communication Interfaces

### 5.1 UART Debug: CP2102N USB-to-UART Bridge

**Function:** Serial debug output and firmware reprogramming

```
STM32H745              CP2102N (QFN-28)
--------               -------
PA2 (UART2_TX) → UART TX → RXD
PA3 (UART2_RX) → UART RX ← TXD
GND            → VSS     → GND
VCC_3V3        → VDD3    → VDD

USB Side:
USB_D+ → DP
USB_D- ← DM
GND    → GND
VDD    → VDD (powered by USB host)
```

**Baud rate:** 921600 (ASY bit rate, no handshake)

**Features:**
- Single-chip solution (no discrete crystal required on CP2102N)
- USB 2.0 Full-speed (12 Mbps, 1 ms latency)
- Hardware flow control: Not used (software-based via CTS/RTS)
- Integrated voltage regulator: 3.3V output (pins 3, 23)
- Isolation: None (use optical isolator if noise issues arise)

**Driver support:**
- Windows 7+: Built-in CDC driver (mfg = "Silicon Labs", model = "CP2102N")
- macOS: Silicon Labs driver (silabs.com)
- Linux: Built-in (kernel 3.0+)
- Arduino IDE: Selectable as "CP2102N" board

**Typical debug output format:**
```
=== STM32H745 Shotgun Testbench v1.4.0 ===
M7 Core: 480 MHz, M4 Core: 240 MHz
Initializing sensors...
  ✓ IMU initialized
  ✓ LRF initialized
Hardware initialization complete.
Initializing FreeRTOS...
RTOS initialization complete.
System ready (STATE_IDLE)
```

---

### 5.2 USB HS OTG: Dual-Role Device/Host

**Function:** High-speed log download and firmware updates (future v2.0+)

```
STM32H745              USB Connector (Micro-B)
--------               -------
PA11 (OTG_FS_DM) → USB D-
PA12 (OTG_FS_DP) → USB D+
GND              → GND
VBUS             → +5V (from host)
```

**Configuration (v1.4.0):**
- **Device mode** (firmware default): MSC (Mass Storage Class)
  - Emulates USB flash drive (session logs accessible from PC)
  - No driver required (USB mass storage built-in to OS)
  - Write protection: Hardware switch (future)
- **Host mode** (future): Planned for v2.0
  - Connect external sensors or debug probes
  - HID (Human Interface Device) for wireless gamepad-based targeting assist

**Transceiver:** Integrated into STM32H745 (no external PHY needed)

**Design notes:**
- No series resistors required (MCU has built-in 45Ω termination)
- Schottky pull-down on USB_D- (weak 15kΩ for SE0 detection)
- Pull-up on USB_D+ (1.5kΩ, provides signaling after VBUS detection)
- Polyfuse on VBUS input (16V, 500mA Schottky: 1N5819)

---

### 5.3 Bluetooth LE (Optional, v1.5+)

**Function:** Mobile app integration for wireless data streaming and control

**Candidate module:** Qorvo BT121-A (20-pin WLCSP)
- Integrated antenna on die
- 2 Mbps throughput
- -97 dBm sensitivity (100m range in open space)
- Power: 5 µA sleep, 10 mA TX @ 0 dBm
- Cost: $18.50/unit

**Not included in v1.4.0 schematic** (BOM line item shows $0.00)
- Requires additional I/O: UART4 (RX/TX + RTS/CTS)
- Adds complexity: GAP/GATT profile, advertising, connection handling
- Deferred to v1.5 after core firmware validation

---

## 6. Protection & Safety Features

### 6.1 Electrical Protection

| Feature | Component | Spec | Purpose |
|---------|-----------|------|---------|
| **Reverse polarity** | 1N5819 Schottky diode | 0.35V drop @ 1A | Protects against 12V reversed input |
| **Overvoltage** | Zener or TVS on VBUS | 5.25V clamping | Protects USB from surge (next version) |
| **ESD (Static)** | Built-in MCU clamps | ±4 kV IEC 61000-4-2 | Protection on all I/O pins |
| **EMI filtering** | Ferrite bead on UART TX | BLM18KG471SN1D | Attenuates high-frequency noise |
| **Watchdog** | IWDG + WWDG | 5 sec + 1-4 sec | Automatic recovery from MCU hangs |

### 6.2 Software Protection

| Feature | Implementation | Details |
|---------|-----------------|---------|
| **CRC32** | session_log.c | Protects all Flash writes; detected/rejected on mismatch |
| **MPU (Memory Protection)** | firmware/src/main.c | Stack overflow → automatic fault handler → safe halt |
| **Stack canary** | FreeRTOS | Task stack overflow detection (optional) |
| **Anomaly detection** | anomaly_detector.c | Over-pressure/thermal runaway → operator warning |

---

## 7. Manufacturing & Assembly

### 7.1 PCB Specifications

| Parameter | Specification |
|-----------|---------------|
| **Layer count** | 4-layer |
| **Substrate** | FR-4 (standard epoxy glass) |
| **Thickness** | 1.6 mm ± 0.15 mm |
| **Copper weight** | 2 oz (70 µm) on all layers |
| **Surface finish** | HASL (Hot Air Solder Leveling) or ENIG |
| **Trace width** | 0.15 mm (6 mil) minimum, 0.2 mm preferred |
| **Trace spacing** | 0.15 mm (6 mil) minimum |
| **Pad size** | 0.5 mm (via), 0.8 mm (solder) |
| **Via count** | >4 per high-current node (GND/VCC) |
| **Impedance control** | 50Ω ± 5% for QSPI traces (differential not needed) |

### 7.2 Assembly Process

1. **Design for manufacturing (DFM) review:**
   - No panelization (single board per order)
   - Minimum component spacing: 0.5 mm from edge
   - Fiducial markers: 3x on PCB corners (1 mm diameter)
   - Test pads: Multimeter points for voltage/current verification

2. **Solder paste application:**
   - Stencil: 0.125 mm thickness (standard for 0603 passives, LQFP-144)
   - Type: Lead-free SAC305 (Sn/Ag/Cu)
   - Print velocity: 50 mm/s (controlled for uniform deposition)

3. **Pick & place (P&P):**
   - Placement tolerance: ±0.1 mm (0.25 mm IPC-A-610 class 2 requirement)
   - Rotation: ±5° maximum
   - Skip list: Test pads, mounting holes

4. **Reflow soldering:**
   - Oven profile: Ramp to 180°C in 60-120 sec, peak 245-260°C for 30 sec
   - Cool-down: Ramp to <150°C in 30 sec (slow ramp prevents voids)
   - Atmosphere: Nitrogen (optional, improves joint quality for commercial)

5. **Inspection:**
   - Automated optical inspection (AOI): Checks solder joint bridges, opens, volume
   - X-ray (optional): Verifies solder coverage under LQFP pins and BGA (if used)
   - Functional test: See section 7.3

### 7.3 Functional Verification (Go-No-Go Test)

```c
// Production test sequence (firmware image burned to MCU during programming)
typedef struct {
    uint32_t vcc_3v3_ok   : 1;   // ADC @ PA0 reads 3.2-3.4V
    uint32_t vcc_1v8_ok   : 1;   // ADC @ PA1 reads 1.7-1.9V
    uint32_t imu_present  : 1;   // SPI read IMU ID register (0x00 → 0x71)
    uint32_t lrf_present  : 1;   // I2C read LRF model ID (0x01 → 0xEE)
    uint32_t flash_ok     : 1;   // QSPI read identity (0x9F → 0xEF, 0x40, 0x18)
    uint32_t eeprom_ok    : 1;   // I2C write/read loopback 32 bytes
    uint32_t uart_ok      : 1;   // Loopback self-test @ 921600 baud
    uint32_t watchdog_ok  : 1;   // Trigger IWDG, verify NRST pulse
} TestResults;

// Expected: TestResults = 0xFF (all bits set = PASS)
```

---

## 8. Bill of Materials (BOM) Summary

**Total components:** 41 (13 active + 28 passive)  
**Total cost (1-piece):** $377.81  
**Cost breakdown:**
- MCU + buck converter: $49.25 (13%)
- Sensors (IMU + LRF + thermal): $36.70 (10%)
- Memory (Flash + EEPROM): $4.05 (1%)
- Communication (UART bridge): $4.80 (1%)
- Passives (caps, resistors, inductors): $2.76 (<1%)
- PCB + assembly: $205.00 (54%)
- Contingency (10%): $70.85 (19%)

**Cost reduction strategies (future):**
1. Higher volume (100+ units) → MCU bulk pricing (-30%)
2. Simplified sensors: Dual-axis gyro instead of 9-axis (save $10)
3. Remove EEPROM (store config in Flash): Save $1.20
4. On-chip oscillator (no external crystal): Save $0.95
5. Integrated USB-UART on MCU (skip CP2102N): Save $4.80, but requires USB HS implementation

---

## 9. Design Validation Checklist

### 9.1 Electrical Validation

- [ ] Power supply regulation: 3.3V ± 5% measured under load
- [ ] Current consumption: <200 mA @ full sensor load (M7 + M4 + all sensors)
- [ ] Transient overshoot: <4.0V on any rail (VCAP = 100nF protects at 1V)
- [ ] Noise floor: <50 mV peak-peak on 3.3V (Kalman filter robustness)
- [ ] Clock frequency: 480 MHz ± 0.1% (verified via RTC counter)
- [ ] Reset timing: <10 µs NRST pulse duration

### 9.2 Sensor Validation

- [ ] IMU: Self-test passes (SELF_TEST register), offset <0.5°/s
- [ ] LRF: Ranging at 50 Hz, accuracy ±25 mm @ 1m
- [ ] Barrel temp: Reads within ±1°C of thermocouple reference
- [ ] Ambient temp: Reads within ±2°C
- [ ] Pressure: Absolute ±5% (not critical for ballistic model)

### 9.3 Memory Validation

- [ ] Flash: Read/write at 104 MHz, no bit errors over 1000 cycles
- [ ] EEPROM: I2C loopback, data retention >100 years @ 85°C
- [ ] CRC32: All session logs protected, no false positives

### 9.4 Communication Validation

- [ ] UART: 921600 baud loopback, no parity errors
- [ ] USB: Enumeration, VID/PID correct, mass storage class works
- [ ] SWD: ST-Link connects, can program/erase Flash

### 9.5 Firmware Integration

- [ ] Sensor fusion task: <2 ms loop time (target: 1 ms)
- [ ] Kalman filter convergence: <5 seconds after power-on
- [ ] Shot detection: Latency <20 ms from recoil peak to logging
- [ ] Watchdog: Strobe observed every 100 ms in oscilloscope capture
- [ ] Anomaly detection: Warnings trigger correctly at 2.5× baseline

---

## 10. Design Improvements (Future Versions)

### v1.1 (Q3 2026)

- [ ] Integrated USB-UART in firmware (skip CP2102N external chip)
- [ ] USB HS OTG mass storage (log download without external adapter)
- [ ] Bluetooth LE mobile app (wireless live-view)
- [ ] Altium layout files (PCB design automation)

### v1.2 (Q4 2026)

- [ ] Improved noise immunity: Onboard analog multiplexer for 4× temperature channels
- [ ] Power management: Configurable sleep modes, variable sampling rates
- [ ] Redundant sensors: Dual-IMU (compare for anomaly detection)

### v2.0 (2027)

- [ ] Higher integration: BGA package (reduce board size to 60×80 mm)
- [ ] Performance: GDDR6 on-board (swap external Flash for faster training data collection)
- [ ] Environmental: IP67 conformal coating, connector waterproofing

---

## References

1. **STM32H745 Datasheet** (ST RM0399)
   - https://www.st.com/resource/en/reference_manual/

2. **ICM-20689 IMU** (InvenSense PS-ICM-20689-00 v1.1)
   - https://invensense.tdk.com/products/motion-tracking/9-axis/

3. **VL53L0X Laser Rangefinder** (ST AN4688)
   - https://www.st.com/resource/en/datasheet/vl53l0x.pdf

4. **W25Q128JV QSPI Flash** (Winbond W25Q128JV-IM)
   - https://www.winbond.com/hq/product/code-storage-flash-memory/

5. **TPS62133A Buck Converter** (Texas Instruments SLVS901E)
   - https://www.ti.com/lit/ds/symlink/tps62133a.pdf

6. **CP2102N USB-UART** (Silicon Labs CP2102N-A01-GQFR)
   - https://www.silabs.com/documents/public/data-sheets/cp2102n-datasheet.pdf

7. **IEC 61000-4-2 ESD** (International Electrotechnical Commission)
   - Static discharge immunity testing

8. **PCB Design Standards** (IPC-A-610G, IPC-7095)
   - Circuit assembly quality standards

---

**Document revision:** 1.0  
**Last updated:** 2026-07-28  
**Status:** Ready for prototype manufacturing  
**Next step:** Submit PCB & BOM to fabrication house; parallel: implement USB mass storage in firmware
