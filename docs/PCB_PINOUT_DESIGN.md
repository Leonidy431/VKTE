# VKTE PCB Design & Pinout Guide

**Phase:** 6-7 (Hardware Integration)  
**Status:** Design specification for manufacturing  
**Author:** Guru-level electrical engineering review  

---

## Overview

VKTE requires **4 integrated modules** on a single mainboard:
1. **Main Controller** (Raspberry Pi CM4 or similar SBC)
2. **Laser Control Module** (DPSS laser driver + galvo scanner interface)
3. **Bubble Generator Module** (40 kHz ultrasonic transducer driver)
4. **HUD Display Module** (DLP micro-display or LCOS interface)

Plus **RS-485 telemetry bus** for multi-node communication (engine, sensors, etc.)

---

## Module 1: Main Controller (RPI CM4 or STM32H7)

### Pinout Overview

```
┌─────────────────────────────────────────┐
│  Raspberry Pi Compute Module 4          │
│  (or STM32H745 Dual-core for real-time) │
└─────────────────────────────────────────┘
    ↓
┌─ SoM Connector (200 pins) ──────────────┐
│ Power:        3V3, GND (multiple)       │
│ USB:          USB 2.0 (HS0, HS1)        │
│ PCIe:         1-lane PCIe Gen 2         │
│ SD:           eMMC (8GB) built-in       │
│ Camera:       MIPI CSI-2 (2 lanes)      │
│ Display:      MIPI DSI-2 (2 lanes)      │
│ GPIO:         27 GPIO pins (3V3 logic)  │
│ SPI:          SPI0 (CE0, CE1)           │
│ I2C:          I2C-0, I2C-1              │
│ UART:         UART0, UART1              │
└─────────────────────────────────────────┘
```

### Critical GPIO Mapping

| GPIO | Function | Voltage | Notes |
|------|----------|---------|-------|
| **Power** |
| 3V3 | Regulated 3.3V | 3.3V | 1A typical (buck converter for load) |
| GND | Ground | 0V | Star grounding point |
| 5V (input) | USB 5V or barrel jack | 5V | 5A capacity, protection required |
|
| **Laser Control** |
| GPIO17 (BCM) | PWM0 (laser power modulation) | 3.3V | 100 kHz modulation, needs gate driver |
| GPIO18 (BCM) | PWM1 (shutter enable) | 3.3V | TTL high = laser on |
| GPIO23 | SPI0_CE0 (galvo scanner SPI) | 3.3V | Chip select for Cambridge 6215H |
| GPIO24 | SPI0_CE1 | 3.3V | Reserved for expansion |
|
| **Bubble Generation** |
| GPIO25 (BCM) | 40 kHz driver pulse | 3.3V | 40 kHz square wave, 500mA load |
| GPIO27 | Transducer sense feedback | 3.3V input | Analog via ADC (through comparator) |
|
| **Display (MIPI DSI)** |
| GPIO2/GPIO3 | I2C-1 (display DDC) | 3.3V | Pull-ups 4.7kΩ |
| DSI_CK_P/M | MIPI DSI clock | LVDS | DLP TRP-4500 parallel interface |
|
| **Telemetry (RS-485)** |
| GPIO14 (UART0_TX) | RS-485 TX via MAX485 | 3.3V | Through level shifter |
| GPIO15 (UART0_RX) | RS-485 RX via MAX485 | 3.3V | Through level shifter |
| GPIO22 | RS-485 driver enable (DE) | 3.3V | Pulls high for transmit |
| GPIO27 | RS-485 receiver enable (/RE) | 3.3V | Pulls low for receive |
|
| **Sensors** |
| GPIO4 | ADC0 (temperature via NTC thermistor) | 3.3V | 12-bit ADC |
| GPIO5 | ADC1 (pressure via sensor IC) | 3.3V | I2C interface |
| GPIO6 | ADC2 (salinity / conductivity) | 3.3V | Analog input |

---

## Module 2: Laser Control (DPSS Driver Board)

### Schematic Requirements

**Power Input:**
- 12V DC (2A capacity) from main power distribution
- Bulk capacitor: 1000µF 16V (low-ESR)
- TVS diodes for transient protection

**Laser Diode Driver:**
```
┌─ PWM Input (GPIO17, 100kHz) ──────────┐
│                                        │
└─> Gate Driver (IR2104 or similar)      │
    ├─ High-side FET: IRFZ44N (3A Vds)  │
    └─ Low-side FET: IRFZ44N             │
        ├─ Diode pump to laser diode     │
        └─ Current-limiting resistor     │
            └─ DPSS Laser (CNI MGL-III-532)
```

**Pinout (40-pin connector to mainboard):**

| Pin | Signal | Direction | Voltage | Notes |
|-----|--------|-----------|---------|-------|
| 1-2 | 12V_IN | Input | 12V | 2A capacity |
| 3-4 | GND | Ground | 0V | 2x pins (lower impedance) |
| 5 | PWM_IN | Input | 3.3V → 5V level shift | 100 kHz laser modulation |
| 6 | SHUTTER_EN | Input | 3.3V | TTL (1=on, 0=standby) |
| 7 | TEMP_OUT | Output | 0-3.3V | Thermistor feedback (ADC) |
| 8 | I2C_SDA | Bidirectional | 3.3V | DAC (LTC2617) for power trim |
| 9 | I2C_SCL | Bidirectional | 3.3V | Address: 0x60 (factory default) |
| 10 | STATUS_FAULT | Output | 3.3V → 5V pullup | Laser fault detection |

---

## Module 3: Bubble Generator (40 kHz Transducer Driver)

### Hardware Block

```
GPIO25 (40kHz pulse) ───┐
                        ├─> Schmitt trigger (74HCT14) ───┐
                        │                                  │
                        └─> Phase control (optional)       │
                                                           ↓
                        ┌─ MOC3023 Optocoupler (for isolation)
                        │
                        └─> TRIAC (BTA16-600) 
                            ├─ Input: 40 kHz modulated signal
                            └─ Output: AC 40 kHz to transducer
```

**Power:**
- 12V AC or rectified 16V DC from main supply
- Soft-start circuit (NTC thermistor + resistor) for inrush current limiting
- Capacitor bank: 100µF + 10µF for ripple suppression

**Transducer Connection:**

| Contact | Function | Voltage | Notes |
|---------|----------|---------|-------|
| ① | Transducer A | 12V AC | Steminc SMBLTD45F40H phase A |
| ② | Transducer B | 12V AC | Steminc phase B (opposite polarity) |
| ③ | Case GND | 0V | Titanium case (seawater) |

**Driver Board Pinout (26-pin connector):**

| Pin | Signal | Type | Voltage |
|-----|--------|------|---------|
| 1-2 | 12V_IN | Power | 12V, 0.5A nominal |
| 3-4 | GND | Ground | 0V |
| 5 | PULSE_IN | Input | 3.3V / 5V tolerant |
| 6 | FREQUENCY_SET | Analog In | 0-3.3V (ADC for trim) |
| 7 | DUTY_CYCLE_SET | Analog In | 0-3.3V (potentiometer) |
| 8 | FAULT_OUT | Output | 3.3V open-drain |
| 9-10 | TRANSDUCER_A | Output | 12V AC |
| 11-12 | TRANSDUCER_B | Output | 12V AC (opposite phase) |

---

## Module 4: Galvo Scanner (Cambridge Tech 6215H)

### SPI Interface

**Protocol:** XY2-100 (proprietary Cambridge Tech protocol)
- **Speed:** 12.5 MHz SPI clock (or configured)
- **Frame:** 20 µs per point movement
- **Resolution:** 12-bit DAC output per axis (±20° optical range)

**Schematic:**

```
GPIO23 (SPI_CE0) ──┐
GPIO11 (SPI_MOSI) │───> Level Shifter (74LVC245) ──> Cambridge 6215H
GPIO9  (SPI_MISO) │                                      (SPI mode)
GPIO10 (SPI_SCLK) │
```

**Connector Pinout (50-pin D-sub):**

| Pin | Signal | Direction | Voltage | Notes |
|-----|--------|-----------|---------|-------|
| 1 | +5V | Supply | 5V, 500mA | Regulated 5V for internal optics |
| 2 | GND | Ground | 0V | |
| 3-4 | SPI_CLK | Input | 3.3V→5V | 12.5 MHz typical |
| 5-6 | SPI_DIN (MOSI) | Input | 3.3V→5V | Data to galvo |
| 7-8 | SPI_DOUT (MISO) | Output | 5V→3.3V | Status feedback |
| 9 | SPI_CHIP_SELECT | Input | 3.3V→5V | Active low |
| 10 | LASER_SYNC (TTL) | Input | 5V | Trigger for laser firing |
| 11-12 | X_DAC_OUT | Output | 0-5V | Analog feedback (±20°) |
| 13-14 | Y_DAC_OUT | Output | 0-5V | Analog feedback (±20°) |
| 15 | READY | Output | 5V open-drain | Scan complete signal |

**XY2-100 Command Format:**

```
Byte 0: 0x00 (start)
Byte 1-2: X position (12-bit DAC: 0-4095 → -20° to +20°)
Byte 3-4: Y position (12-bit DAC: 0-4095 → -20° to +20°)
Byte 5: Laser power (0-255 → 0-100% intensity)
Byte 6: CRC-8 checksum
```

---

## Module 5: HUD Display (DLP TRP-4500 Interface)

### DLP Connector Pinout (54-pin LVDS)

**Power Rails:**
- 3.3V: 500 mA (micro-display logic)
- 5V: 1A (LED driver, shutter)
- 12V: 2A (cooling fan, if needed)

**Video Interface (LVDS):**

| Pin | Signal | Direction | Voltage | Notes |
|-----|--------|-----------|---------|-------|
| 1-8 | LVDS_CLK_P/M | Input | LVDS | 1920×1200 @ 60 FPS |
| 9-16 | LVDS_DATA0_P/M | Input | LVDS | 4 data pairs (LLVDS) |
| 17-24 | LVDS_DATA1_P/M | Input | LVDS | |
| 25-32 | LVDS_DATA2_P/M | Input | LVDS | |
| 33-40 | LVDS_DATA3_P/M | Input | LVDS | |
| 41 | HSYNC | Input | 3.3V | Horizontal sync |
| 42 | VSYNC | Input | 3.3V | Vertical sync |
| 43 | DE (Data Enable) | Input | 3.3V | |
| 44 | PWM_BRIGHTNESS | Input | 3.3V @ 100 Hz | Analog brightness control |
| 45 | SHUTTER_CTRL | Input | 3.3V | Open/closed shutter |
| 46 | TempFault | Output | 3.3V | Thermal shutdown flag |

**LVDS Termination:**
- 100Ω differential termination resistors on receiver side
- PCB trace length matching: ±0.1 mm

---

## Layout Guidelines (PCB Design Best Practices)

### Layer Stack (6-layer board)

```
Layer 1 (Top):        Signal routing (high-speed signals)
Layer 2:              Ground plane (continuous reference)
Layer 3:              Power distribution (12V, 5V, 3V3)
Layer 4:              Signal return & stitching
Layer 5:              Signal routing & interconnects
Layer 6 (Bottom):     Ground plane + component pads
```

### Critical Design Rules

#### 1. Power Distribution

**Bulk Capacitors (at supply input):**
- 12V: 1000µF low-ESR (Panasonic FR or Nichicon)
- 5V: 470µF low-ESR
- 3.3V: 470µF low-ESR
- Bypass: 100nF ceramic (X7R) every IC power pin

**Decoupling Strategy:**
```
12V_IN ──[L]──[1000µF]──[100nF]──[GND]
         │
         └─> 5V LDO ──[470µF]──[100nF]──[GND]
                │
                └─> 3.3V LDO ──[470µF]──[100nF]──[GND]
```

#### 2. Signal Integrity

**RS-485 Telemetry Bus:**
- Differential pair impedance: 120Ω (ε_r ≈ 4.5 for FR-4)
- Trace width: 7-8 mil (0.18-0.2 mm)
- Spacing: 6 mil (0.15 mm)
- Via stitching on return: every 1 cm

**LVDS Display Interface:**
- Differential impedance: 100Ω
- Matched pair lengths within 5 mils
- Shielded if >10 cm traces

#### 3. Grounding

**Star Ground Point:**
```
           ┌─────────────────────┐
           │  Main Ground Plane  │
           │  (Layer 6 - Bottom) │
           └────────┬────────────┘
                    │
        ┌───────────┼───────────┐
        │           │           │
    [12V] ───── [5V] ────── [3.3V]
   Return     Return       Return
```

**Ground Vias:**
- Place every 0.25 mm around analog circuits
- Minimum via size: 10 mil (0.254 mm) diameter

#### 4. Thermal Management

**Laser Driver (hottest component):**
- Via stitching: 2x2 grid under FET pads
- Thermal pad to GND plane with vias
- Copper pours in surrounding area (no isolation)

**Bubble Generator (moderate heat):**
- Via stitching under TRIAC
- Adequate PCB real estate for heatsink (if needed)

#### 5. EMI Shielding

**Faraday cages (shielded compartments):**
- Laser PWM traces: isolated within cage
- 40 kHz bubble driver: separate compartment
- RS-485 differential pair: twisted if possible

**Filtering:**
- Ferrite beads (BLM18PG221SN1D) on analog sensing lines
- RC filters on PWM inputs (R=1kΩ, C=10nF)

---

## Footprint Selection (EDA Library)

### Component Specifications

| Component | Package | Manufacturer | Part Number | Thermal Notes |
|-----------|---------|--------------|-------------|---------------|
| IRFZ44N FET | TO-220 | Infineon | IRFZ44N-PBF | 1.2°C/W Rja (need heatsink for 2A) |
| MOC3023 | DIP-6 | Fairchild | MOC3023 | Isolated driver, standard |
| BTA16-600 TRIAC | TO-220AB | ST Micro | BTA16-600 | Gate drive: 10mA @ 5V |
| LTC2617 DAC | QFN-20 | Analog Devices | LTC2617IUH | I2C addressable 12-bit |
| MAX485 RS485 | SOIC-8 | Maxim | MAX485CSA | Level shifter 3.3V ↔ RS485 |
| Raspberry Pi CM4 | 200-pin edge connector | RPi | RPi-CM4-2GB-EMMC | 1GB/2GB/4GB/8GB options |

---

## Assembly & Test Checklist

### Pre-Assembly

- [ ] PCB visual inspection (no cracks, shorts)
- [ ] Gerber files verified with manufacturer
- [ ] Bill of Materials (BOM) cross-checked with suppliers
- [ ] Footprints match datasheets ±0.1 mm

### Post-Assembly

- [ ] Visual inspection (solder joint quality, no cold joints)
- [ ] Power-on test (12V, 5V, 3.3V rails present)
- [ ] RS-485 bus continuity test (probe with oscilloscope)
- [ ] SPI communication test (galvo scanner responds to XY2 commands)
- [ ] Laser safety: interlock test (laser should not fire without shutter)
- [ ] Thermal test: monitor temperatures under load (laser driver should stay <80°C)

---

## Production Notes

### Manufacturing Specification

**PCB:**
- 6-layer FR-4, 1.6 mm thickness
- Min trace width: 6 mil (0.15 mm)
- Min via diameter: 10 mil (0.254 mm)
- Solder mask: LPI (solder resist between pads)
- Impedance controlled: ±10% tolerance

**Assembly:**
- Stencil: laser-cut stainless steel (125 µm aperture)
- Reflow profile: peak 245°C, 10-30 sec
- AOI (Automated Optical Inspection) required for >100 units

**Testing:**
- ICT (In-Circuit Test) for power rails
- Functional test: communicate with CM4 via UART
- High-pot (hi-pot) safety test: 1500V AC / 60 sec (if isolation required)

---

## Revision History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2026-07-29 | Initial PCB specification (Phase 6-7 prep) |
| TBD | TBD | Manufacturing feedback & BOM updates |

---

**Document Status:** ✅ Ready for PCB Design Phase (Altium Designer / KiCad)  
**Next Step:** Create detailed schematics with all connections  
**Review Cycle:** DFM (Design for Manufacturability) review before production
