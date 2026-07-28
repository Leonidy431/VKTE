# STM32H745 Ballistic Corrector - Manufacturing & Assembly Procedures

**Document Version:** 1.0  
**Date:** 2026-07-28  
**Phase:** 3 - Prototype Fabrication & Assembly  
**Scope:** 5-board prototype batch  
**Standards:** IPC-A-610G, IPC-A-600, IEC 61000-4-2  
**Status:** Ready for implementation

---

## Executive Summary

This document defines the manufacturing and assembly procedures for prototyping the STM32H745 Ballistic Corrector. The process includes:
- **PCB arrival inspection** (visual, electrical, dimensional)
- **Component preparation** (sorting, verification, ESD handling)
- **Solder paste stencil & pick-and-place programming**
- **Reflow oven soldering** (SAC305 lead-free profile)
- **Post-reflow inspection** (AOI, manual, X-ray for BGA-like density)
- **Functional test** (manufacturing PAT - Power, Analog, UART tests)
- **Debug & bringup** (serial terminal, SWD debugging)
- **Environmental stress testing** (temperature cycling, humidity)

**Timeline:** 2-3 weeks from PCB delivery to functional prototype

**Yield target:** >95% first-pass on 5 boards (4-5 working units expected)

---

## Part 1: Pre-Assembly Preparation

### 1.1 PCB Receiving Inspection

**Upon PCB delivery (from Sierra Circuits, JLCPCB, or equivalent):**

```
Bare PCB Quality Checklist:
├─ Visual Inspection
│  ├─ [ ] Board dimensions: 100mm × 80mm ±0.2mm
│  ├─ [ ] No visible cracks, delamination, or discoloration
│  ├─ [ ] Silk screen legible (all reference designators visible)
│  ├─ [ ] Solder mask uniform (no bare copper islands)
│  ├─ [ ] Board edges: No burrs, 0.3mm chamfer acceptable
│  └─ [ ] Plating: Golden finish (HASL) or silver (immersion)
│
├─ Electrical Verification
│  ├─ [ ] Continuity test: All traces connected (multimeter Ohm mode)
│  ├─ [ ] Isolation test: GND-to-VCC measured >10 MΩ (open)
│  ├─ [ ] Via plating: Check random vias for continuity (probe with multimeter)
│  └─ [ ] No solder bridges visible (inspect under 10× magnifier)
│
└─ Documentation Review
   ├─ [ ] Fabrication report: Trace widths, layer uniformity confirmed
   ├─ [ ] Impedance verification: 5 test coupons, all 50Ω ±5% for QSPI
   ├─ [ ] Drill file validation: All holes present (visual count sample)
   └─ [ ] Packing slip: Qty (5), part number, date code recorded
```

**Acceptance criteria:**
- Zero cracks or delamination
- All traces continuous
- Impedance report confirms 50Ω ±7% (within margin)
- Proceed to component preparation if all checks pass

**Rejection criteria:**
- Any visible cracks or delamination → Contact fab, request replacement
- Impedance >55Ω or <47Ω → Work with fab on process improvement (non-critical for prototype)
- Missing copper or solder bridges → Scrapped (cost: ~$10-50 per board)

---

### 1.2 Component Verification & ESD Handling

**Before assembly, verify all components:**

**Main Processor:**
```
U1: STM32H745ZIT6
├─ [ ] Package: LQFP-144 (verify pin count, footprint)
├─ [ ] Date code: Within 2 years of current date (avoid aged stock)
├─ [ ] No visible damage to leads (visual under 10× magnifier)
├─ [ ] Baked before soldering (2 hours @ 125°C in oven if date code >6 months old)
└─ [ ] ESD-safe: Store in conductive bag, use wrist strap during handling
```

**Sensors (I2C/SPI):**
```
U7-U11: (ICM-20689, VL53L0X, MCP9808, BMP390, optional BT121-A)
├─ [ ] Package marking matches BOM (verify part number)
├─ [ ] No bent pins or damaged solder pads
├─ [ ] Baked if date code >6 months old
└─ [ ] Store in anti-static containers
```

**Memory chips:**
```
U5: W25Q128JV (QSPI Flash)
├─ [ ] Package: SOIC-8-W (verify narrow-body)
├─ [ ] Voltage rating: Confirm 3.3V I/O compatible
└─ [ ] Baked before soldering

U6: AT24C256C (I2C EEPROM)
├─ [ ] Package: SOIC-8
└─ [ ] Baked before soldering
```

**Passive component verification:**
```
Capacitors (MLCCs):
├─ [ ] Count: 28 units (4×100µF, 8×10µF, 16×100nF)
├─ [ ] Voltage rating: All ≥10V minimum
├─ [ ] Temperature coefficient: X7R or NP0 (no Y5V)
└─ [ ] No visible cracks or damage

Resistors:
├─ [ ] Count: 4× 10kΩ pull-up resistors
├─ [ ] Tolerance: 1% or better
└─ [ ] Wattage: 1/16W adequate

Inductor:
├─ [ ] Part: SPM6530T-4R7M (4.7µH)
├─ [ ] DC resistance: <0.5Ω (verify with multimeter if available)
└─ [ ] Check for corrosion or damage
```

**ESD precautions:**
- [ ] Establish ESD-safe work area (conductive mat, grounding strap)
- [ ] All personnel wear wrist strap connected to ground
- [ ] Anti-static bags for all components
- [ ] Avoid plastic tweezers (use conductive stainless steel)
- [ ] Verify mat continuity: <1 MΩ to ground

---

## Part 2: Solder Paste Stencil & Setup

### 2.1 Solder Paste Stencil Fabrication

**Obtain stencil from:** OSH Stencils, Laser Stencil, or similar

**Stencil specifications:**
```
Solder Paste Stencil
├─ Frame: 200×150mm aluminum (standard)
├─ Mesh: 
│  ├─ Material: Stainless steel (austenitic 304)
│  ├─ Thickness: 0.1mm (100µm, laser-etched)
│  ├─ Emulsion: Polymer or solvent-based
│  └─ Registration: ±0.05mm to PCB CAD
│
└─ Aperture sizing (by area):
   ├─ Ball pads (0.65mm diameter): 0.3mm opening (47% area)
   ├─ IC pads (0.4mm width): 0.2×0.3mm opening (50% area)
   ├─ Via pads (0.3mm diameter): 0.15mm opening (25% area)
   └─ Passive pads (0.6mm×1.2mm): Full-size opening (100% area)
```

**Order details:**
- Gerber input: `STM32H745_Ballistic_Corrector-F_SilkS.gbr` (top layer)
- Frame: Standard 200×150mm aluminum
- Shipping: 3-5 days (standard)
- Cost: $40-60 per stencil
- Quantity: 1 stencil (sufficient for 5-10 boards with cleaning)

---

### 2.2 Solder Paste Preparation

**Solder paste specifications:**
- **Alloy:** SAC305 (96.5% Sn, 3% Ag, 0.5% Cu) - lead-free
- **Flux type:** No-clean organic (low-halide, no flux residue cleanup required)
- **Particle size:** T4 (25-45 µm spheres) or T3 (15-25 µm)
- **Viscosity:** 5-8 Pa·s @ 25°C
- **Storage:** Refrigerated 2-8°C, <12 months shelf life

**Recommended product:** Kester R708LT87-5L (5 lbs/2.3 kg, $100-150)

**Paste conditioning (before use):**
- Remove from fridge 1-2 hours before use (allow to warm to 25°C)
- Stir gently with wooden stick (avoid foam)
- Verify consistency: Should flow smoothly without clumping
- Discard if date code >12 months old or visible oxidation

---

### 2.3 Solder Paste Stencil Printing

**Equipment needed:**
- Solder paste stencil (from 2.1 above)
- Printing squeegee (65° angle, rubber/polyurethane blade)
- PCB fixturing jig (vacuum hold-down or mechanical clamp)
- Paste applicator (fork or spatula)

**Printing procedure (manual method for prototype):**

```
Step 1: Setup
├─ [ ] Place PCB in fixturing jig (align top-left corner with registration mark)
├─ [ ] Position stencil over PCB (verify alignment via pin holes or fiducials)
├─ [ ] Secure stencil to prevent shifting (clamp or tape edges)
└─ [ ] Apply 2-3 cm³ of solder paste at one end of stencil

Step 2: Paste application
├─ [ ] Hold squeegee at 65° angle (60-75° acceptable)
├─ [ ] Apply downward pressure ~5 lbs (hand feel)
├─ [ ] Push paste across stencil in single smooth motion (speed: 2-4 cm/sec)
├─ [ ] Excess paste accumulates at far end (collect for re-use)
└─ [ ] Lift stencil carefully (avoid dragging on pads)

Step 3: Inspection
├─ [ ] All pad areas filled with paste (visual inspection)
├─ [ ] No voids or incomplete coverage on large pads
├─ [ ] Paste height ~0.2mm (consistent across board)
├─ [ ] No paste bridges between adjacent pads
├─ [ ] Silk screen areas clear of paste (wipe if needed)
└─ [ ] Remove test chip: Place small PCB or scrap area under stencil to verify paste transfer
```

**Common printing defects & fixes:**

| Issue | Cause | Fix |
|-------|-------|-----|
| Incomplete paste coverage | Stencil misalignment | Re-register stencil, re-print |
| Paste smearing across board | Squeegee angle too shallow | Increase angle to 70° |
| Paste clumping on pads | Paste too thick or old | Remix paste, refresh batch |
| Bridging between pads | Over-application | Reduce squeegee pressure, verify clearances |
| Paste drying | Time delay >30 min | Use fresh paste, print all 5 boards within 15 min |

---

## Part 3: Pick-and-Place (Component Placement)

### 3.1 Manual Pick-and-Place (For 5-board prototype)

**Equipment:**
- Stereo microscope (10-40× magnification)
- Fine-tip tweezers (anti-static, stainless steel)
- Solder paste with pasted PCB (from section 2.3)
- Component tray (organized by reference designator)
- ESD mat and wrist strap

**Component placement procedure:**

**Priority 1: Large IC (MCU) - U1: STM32H745ZIT6**
```
U1: STM32H745ZIT6 (LQFP-144, 20×20mm)
├─ [ ] Under magnifier, identify pin 1 marking (white dot/triangle)
├─ [ ] Align pin 1 with pad 1 on PCB (verify orientation)
├─ [ ] Place component perpendicular to board
├─ [ ] Gently press down until all pins contact paste
├─ [ ] Verify all pads received solder paste (check under microscope)
└─ [ ] Repeat for any LQFP packages (only MCU in this design)

Typical issues:
- Misalignment: Will be obvious during reflow (adjacent pads will have bridges)
- Cold solder joints: Ensure all pins wet by paste (inspect before reflow)
```

**Priority 2: Sensors (medium packages - U5-U11)**
```
U5: W25Q128JV (SOIC-8-W)
├─ [ ] Locate pads on stenciled area (4 pads each side)
├─ [ ] Position package parallel to board
├─ [ ] Press down gently until pads contact paste
└─ [ ] Verify all 8 pads moistened by paste

U7-U11: Other sensors (repeat same procedure)
├─ ICM-20689 (SPI IMU, LFBGA or QFN package - verify exact package!)
├─ VL53L0X (LGA-12 or TFBGA package)
├─ MCP9808 (SOIC-8)
├─ BMP390 (LGA-8)
└─ (Defer BT121-A Bluetooth module to v1.1)
```

**Priority 3: Power management (U2-U4, U1 inductor, diode)**
```
U2: TPS62133ADBVT (TSOT-23-6)
├─ [ ] Tiny package; use magnifier
├─ [ ] Align marker dot with position 1
└─ [ ] Press down to seat paste

U3, U4: NCP1117 LDOs (SOT-223, 4 pins)
└─ [ ] Larger than TSOT; easier to place

D1: Schottky 1N5819 (SOD-123, 2-pad diode)
├─ [ ] Verify orientation: Cathode stripe toward pad 2
└─ [ ] Place carefully (can shift easily with tweezers)

L1: Inductor 4.7µH (1206 package)
├─ [ ] Large pad; hard to misalign
└─ [ ] Press firmly to seat paste
```

**Priority 4: Passive components (C, R)**
```
Bulk capacitors (100µF, 10µF, 100nF)
├─ [ ] Use 1206 or 0805 packages (easier to handle than 0603)
├─ [ ] Place sequentially across board
├─ [ ] No polarity concern (multilayer ceramic)
└─ [ ] Press firmly

Resistors (10kΩ)
├─ [ ] Tiny (0603 or 0402); use magnifier
├─ [ ] Place near MCU/sensor headers (pull-up locations)
└─ [ ] No polarity

Crystal capacitors (20pF)
├─ [ ] Near X1 crystal oscillator
├─ [ ] Pair them close together
└─ [ ] Place near MCU crystal pins
```

**Placement timeline:**
- Manual placement of 5 boards: ~1-2 hours total (experienced technician)
- First board slowest; subsequent boards faster with muscle memory

---

### 3.2 Component Placement Verification (Pre-Reflow)

**Before reflow, inspect each board:**

```
Placement QA Checklist (per board):
├─ Visual inspection (no magnifier needed):
│  ├─ [ ] All ICs centered on pad areas (not shifted)
│  ├─ [ ] No component bridges between adjacent pads
│  ├─ [ ] All visible solder paste shiny (not dull/oxidized)
│  ├─ [ ] No missing components (cross-check BOM)
│  └─ [ ] Silk screen visible around components
│
└─ Magnified inspection (10× stereoscope):
   ├─ [ ] MCU pins wetted by solder paste (all 144 pads visible)
   ├─ [ ] Sensors: Pads aligned with component edges, paste present
   ├─ [ ] Passives: No offset or rotation from indicated position
   ├─ [ ] Inductors: Mounted flat with full pad contact
   └─ [ ] No solder bridges (between adjacent pads)
```

**Fix issues before reflow:**
- Missing component: Pick from component tray, re-place
- Misaligned IC: Use tweezers to nudge into position (paste still tacky)
- Shifted solder paste: Solder paste can reflow; inspect after reflow
- Bridge between pads: Drag tweezers between pads to remove excess paste

---

## Part 4: Reflow Soldering

### 4.1 Reflow Oven Setup & Profile

**Oven specification:** Small benchtop reflow oven (Taiga, RayPak, or equivalent)
- Temperature range: 25-300°C capability
- Ramp rate control: 0.5-2°C/sec adjustable
- Chamber size: Minimum 12"×8" (fits 100mm×80mm PCB)
- Estimated cost: $1,000-3,000 (or use local makerspace)

**SAC305 Lead-Free Solder Reflow Profile:**

```
Temperature Profile (IPC-A-610G Standard)
─────────────────────────────────────────────

     Liquid
      ↑
 270°C ├─────────────────┐
       │   Peak zone     │ Hold: 10-30 sec @ 245-260°C
 245°C ├────╱─────────╲──┤ (this is above Tl, solder liquid)
       │   ╱           ╲ │
 217°C ├──╱─────────────╲┤ Reflow zone: 217-260°C
       │ ╱               ╲│ Ramp-up: 1-3°C/sec (50-90 sec)
       ├───────────────────┤
       │ Preheat zone      │ 150-200°C for 60-120 seconds
 150°C ├───────────────────┤
       │                   │
  25°C ├───────────────────┘ Cool-down: Natural (0.5°C/sec)
       │                   │ Total time: 3-4 minutes
       └───────────────────→ Time (minutes)

Ramp 1: 25→150°C @ 1.5°C/sec  = 83 seconds (preheat start)
Hold:   150→200°C @ 0.5°C/sec = 100 seconds (preheat)
Ramp 2: 200→245°C @ 2°C/sec   = 22 seconds (ramp-up)
Peak:   245→260°C hold         = 20 seconds (reflow window)
Cool:   260→25°C @ 0.5°C/sec  = 470 seconds (natural cooling)

Total cycle time: ~9-10 minutes per board
```

**Oven calibration:**
- [ ] Verify oven temperature with thermometer (±5°C accuracy)
- [ ] Place temperature probe in center of chamber
- [ ] Test run with empty PCB (no components) to verify profile
- [ ] Adjust heating elements if peak temperature differs by >10°C

---

### 4.2 Reflow Soldering Procedure

**Setup:**
```
1. Preheat oven to 100°C (warm-up, prevents thermal shock)
2. Load pasted PCB onto oven shelf (ensure level positioning)
3. Set reflow profile (use manual control or programmed profile)
4. Start reflow cycle
```

**During reflow (monitor visually):**
```
Phase 1: Preheat (0-100 sec)
├─ Solder paste appears dull, starts to flow
├─ Flux activates, component shift possible (watch carefully)
└─ Any component misalignment will show here

Phase 2: Ramp-up (100-130 sec)
├─ Solder paste becomes shiny (tin melting)
├─ Components may shift slightly due to flux activity
└─ Watch for component tombstoning (lifting on one end) - common with 0603+ passives

Phase 3: Reflow window (130-160 sec)
├─ Solder fully liquid, components floating on paste
├─ Flux fuming (normal - vapor creates outgassing)
├─ DO NOT OPEN OVEN (thermal shock, empty reflow)
└─ This is critical moment - let thermal mass work

Phase 4: Cool-down (160+ sec)
├─ Oven cooling, solder solidifying
├─ Components settle into final position
├─ Don't remove until <100°C (risk of cracking)
└─ Natural air cooling okay (faster than oven shutdown)
```

**Post-reflow (cool to room temp):**
```
1. Wait for oven to cool to <50°C before opening
2. Carefully remove PCB (use tweezers, avoid touching leads)
3. Inspect visually for solder joint quality
4. Look for bridges, cold joints, or component shifts
```

---

### 4.3 Post-Reflow Inspection (Critical QA)

**Visual inspection under magnifier (10-20× magnification):**

```
Solder Joint Quality Criteria (IPC-A-610G):

Good joint:
├─ Shape: Cone or dome (45-55° wetting angle)
├─ Coverage: Solder wets 95%+ of pad
├─ Fillet: Smooth transition from pad to component lead
├─ Color: Shiny silver or slightly dull (not dull gray or black)
└─ No voids or pinholes

Acceptable but suboptimal:
├─ Dull finish (flux residue still present - can clean)
├─ Slight wetting angle >60° (still acceptable if no bridges)
└─ Minor voiding <10% of joint area

Reject (rework required):
├─ Cold joint: Grainy, dull appearance, weak mechanical bond
├─ Insufficient wetting: <60% pad coverage
├─ Bridge: Solder bridge between adjacent pads (shorts)
├─ Lifted component: Part not seated, visible gap below pads
└─ Missing solder: No visible solder on joint
```

**Defect mitigation:**

| Issue | Cause | Inspection | Fix |
|-------|-------|-----------|-----|
| Cold joint | Low paste volume or stencil misalignment | Grainy texture, dull | Rework with soldering iron |
| Bridge | Excessive paste or pad spacing too small | Shorted adjacent pads | Drag solder away with iron + solder wick |
| Lifted component | Uneven paste or thermal shock | Visible gap under part | Re-solder entire pad array |
| Void | Air bubble in paste | Dark spots in fillet area | Accept if <10%; rework if >20% |
| Tombstone | Uneven wetting on two-pad component | Component stands on end | Reflow again; check thermal balance |

**Acceptance criteria (first-pass yield):**
- 5 boards fabricated
- Target: 4-5 working boards (80-100% yield)
- Acceptable: 3/5 working (60% yield)
- Unacceptable: <3/5 working (requires root-cause analysis)

---

## Part 5: Manufacturing PAT (Production Acceptance Test)

### 5.1 Power-On Self-Test (POSH)

**Equipment needed:**
- Multimeter (DC voltage measurement)
- USB 5V power supply (or 12V barrel jack connector)
- Current clamp or ammeter (measure inrush current)
- Lab power supply with current limit (if available)

**Power-on procedure (CRITICAL - prevent short-circuit):**

```
Step 1: Preliminary checks (before power)
├─ [ ] Visual inspection: No solder bridges or cold joints
├─ [ ] Multimeter continuity: GND-to-VCC measured >10 MΩ (open circuit)
├─ [ ] No loose components or fragments on board
└─ [ ] Connector orientation verified (barrel jack polarity correct)

Step 2: Power-up (current-limited)
├─ [ ] Set lab PSU to 5V, current limit to 500mA (prevents destructive short)
├─ [ ] Connect PSU to PCB barrel jack (or USB 5V if connector present)
├─ [ ] Watch ammeter during first 10 seconds:
│   ├─ Expected: <100mA steady-state (buck converter + decaps charging)
│   ├─ Warning: 100-200mA (possible issue, investigate)
│   └─ STOP: >300mA (short circuit; disconnect immediately, check PCB)
│
├─ [ ] After 10 sec stable, remove current limit
├─ [ ] Measure voltage rails with multimeter:
│   ├─ VCC_5V: Should read 4.75-5.25V (buck output)
│   ├─ VCC_3V3: Should read 3.15-3.45V (LDO output)
│   └─ VCC_1V8: Should read 1.71-1.89V (analog LDO)
│
└─ [ ] Record values on test log
```

**Expected current draw (steady-state):**
```
MCU (idle, clocks running):      ~50 mA
Flash (standby):                 <1 mA
Sensors (SPI/I2C idle):          ~5 mA
LDOs (quiescent):                ~2 mA
───────────────────────────────────────
Total idle power:               ~50-60 mA @ 5V
                                 ~10W dissipation in buck converter

At full load (M7+M4 running, sensors active, QSPI writing):
Expected peak: 150-200 mA @ 5V (buck converter at rated current)
```

**Failure modes & diagnostics:**

| Symptom | Cause | Diagnosis | Fix |
|---------|-------|-----------|-----|
| >300mA on power-up | Short circuit | Inspect PCB under microscope; check for solder bridges on power planes | Reflow affected area or replace PCB |
| VCC_5V reads 3V | Buck converter not starting | Measure inductor current; check gate drive to MOSFETs | Check TPS62133A presence & orientation |
| VCC_3V3 reads 5V | LDO feedback failure | Check LDO output directly; measure divider network | Likely solder bridge or cold joint |
| VCC_3V3 >5V | Regulator shorted | Disconnect immediately | Replace U3 LDO (rework IC) |

---

### 5.2 Clock Verification

**Equipment:** Oscilloscope (1GHz+ bandwidth, or frequency counter)

**Test procedure:**
```
Measurement 1: MCU clock output (PLL running at 480 MHz)
├─ [ ] Probe: TP8 (QSPI_CLK), or directly at W25Q128JV pin 1
├─ [ ] Expected: 104 MHz square wave (QSPI clock divider from 480 MHz PLL)
│   └─ Frequency: 104.0 ± 1 MHz (1% tolerance)
│   └─ Amplitude: 3.3V peak-to-peak
│   └─ Duty cycle: 50% ± 5%
│
├─ [ ] Record oscilloscope screenshot for documentation
└─ [ ] If frequency off: Check crystal X1 (8 MHz)

Measurement 2: Crystal frequency (8 MHz input)
├─ [ ] Probe: TP6 (XTAL_IN crystal oscillator input)
├─ [ ] Expected: 8.000 MHz ± 20 ppm
│   └─ Amplitude: 0.5-1.0V peak-to-peak
│   └─ No clipping (sine wave, not saturated)
│
└─ [ ] If 0V or missing: Check crystal X1 and load caps

Measurement 3: M4 clock (240 MHz, half of M7 rate)
├─ [ ] Indirect: Measure UART clock accuracy (M4 drives UART @ 921.6k baud)
└─ [ ] See section 5.3 (UART loopback test)
```

**Acceptance criteria:**
- QSPI clock: 104 ± 2 MHz (2% tolerance acceptable for prototype)
- Crystal: 8.000 ± 0.002 MHz (present and running)
- Both clocks present: MCU PLL is locked successfully

---

### 5.3 UART Debug Communications Test

**Equipment:**
- USB-to-UART cable or built-in CP2102N (USB Mini-B connector)
- Terminal software: PuTTY, TeraTerm, or minicom
- Baud rate: 921.6k bits/sec (non-standard; verify in firmware)

**Test procedure:**
```
Step 1: Connect UART
├─ [ ] Connect USB cable to J2 (UART debug connector) or J1 (USB Mini-B via CP2102N)
├─ [ ] Open terminal: 921,600 baud, 8N1 (8 bits, no parity, 1 stop bit)
├─ [ ] Expected: Startup message or boot menu from firmware

Step 2: Loopback test
├─ [ ] Type character in terminal (e.g., "A")
├─ [ ] Expected response: Character echoed back ("A" appears on screen)
├─ [ ] Repeat with multiple characters
├─ [ ] Expected: <100 ms round-trip latency

Step 3: Firmware diagnostic menu
├─ [ ] If firmware loaded: Navigate menu with arrow keys
├─ [ ] Query command: "?", expected output:
│   ├─ Firmware version
│   ├─ System uptime
│   ├─ Sensor status (IMU, LRF, temp sensors)
│   └─ Memory status (Flash capacity, EEPROM contents)
│
└─ [ ] Record output for documentation
```

**Expected firmware startup message (example):**
```
STM32H745 Ballistic Corrector v1.4.0
Build: 2026-07-28 12:34:56 UTC
M7 Core: 480 MHz, M4 Core: 240 MHz
System ready. Type '?' for help.
>
```

**Failure modes:**

| Issue | Cause | Fix |
|-------|-------|-----|
| No output on terminal | UART not running (firmware crash or not loaded) | Check SWD debug connection; reprogram via ST-Link |
| Garbled characters | Baud rate mismatch | Verify 921.6k in firmware; try 115.2k as fallback |
| Output once, then freezes | Watchdog timeout (firmware infinite loop) | Reprogram firmware; check for bugs in M7 task |
| Echo doesn't work | TX/RX pins swapped | Check J2 connector pinout vs. schematic |

---

### 5.4 SWD Debug Connection Test

**Equipment:**
- ST-Link V2 debugger (or SEGGER J-Link if available)
- USB cable to computer
- IAR Embedded Workbench or STM32CubeIDE (free development environment)

**Test procedure:**
```
Step 1: Connect debugger
├─ [ ] Connect ST-Link to J3 (10-pin SWD debug header)
├─ Pinout:
│   ├─ Pin 1: VCC (3.3V)
│   ├─ Pin 2: SWDIO (data)
│   ├─ Pin 4: GND
│   ├─ Pin 6: SWCLK (clock)
│   └─ Pin 10: nRESET (reset)
│
└─ [ ] Launch STM32CubeIDE or IAR Workbench

Step 2: Connect to MCU
├─ [ ] Click: Debug → Connect
├─ [ ] Expected: MCU identified as STM32H745 dual-core
├─ [ ] Verify: Two core threads appear (M7 + M4)
└─ [ ] Set breakpoint in firmware main() function

Step 3: Verify program execution
├─ [ ] Press: Debug → Run
├─ [ ] Expected: Program halts at breakpoint
├─ [ ] Read memory: Dump stack pointer, check for corruption
├─ [ ] Single-step through a few instructions
└─ [ ] Verify registers update correctly
```

**Success criteria:**
- MCU identified (device name, revision shown)
- Dual cores visible in debugger (Cortex-M7, Cortex-M4)
- Code execution visible (breakpoint halts program)
- Registers readable (memory access works)

---

### 5.5 Sensor Communication Tests

**Equipment needed:**
- Oscilloscope (for signal inspection)
- Multimeter (voltage checks)
- I2C/SPI protocol analyzer (optional; can use logic analyzer)

**Test 1: I2C Sensor Discovery**
```
Sensors on I2C bus (address 0x68, 0x29, 0x60, 0x77):
├─ [ ] ICM-20689 IMU @ 0x68/0x69 (SPI only in our design - SKIP)
├─ [ ] VL53L0X LRF @ 0x29
├─ [ ] MCP9808 Temp @ 0x60 (or 0x61-0x67 with address pins)
├─ [ ] BMP390 Pressure @ 0x77

Procedure:
├─ [ ] Load firmware with I2C scanner program
├─ [ ] Query command via UART: "scan" or "i2c_scan"
├─ [ ] Expected output:
│   I2C devices found:
│   - 0x29: VL53L0X (rangefinder)
│   - 0x60: MCP9808 (temperature)
│   - 0x77: BMP390 (pressure)
│
├─ [ ] Measure SCL/SDA lines with oscilloscope:
│   ├─ SCL should clock at ~400 kHz
│   ├─ SDA should have clean rise/fall (not ringing)
│   └─ No excessive loading (capacitance <100pF)
│
└─ [ ] If sensor missing: Check I2C pullups (10kΩ from VCC_3V3)
```

**Test 2: SPI IMU (ICM-20689)**
```
IMU on SPI1 bus (PA5/PA6/PA7 + PA4 chip select):
├─ [ ] Firmware command: "imu_status" or equivalent
├─ [ ] Expected output: Gyro/accel data streaming
│   Example:
│   IMU ID: 0x68 (verified)
│   Accel: X=0.1g Y=0.0g Z=1.0g (gravity)
│   Gyro: X=0 dps Y=0 dps Z=0 dps
│   Temperature: 25.3°C
│
├─ [ ] Oscilloscope inspection (optional):
│   ├─ QSPI_CLK: 10 MHz clock (not QSPI_CLK, SPI1_CLK)
│   ├─ MOSI: Data transitions at clock edges
│   └─ MISO: Data valid on read phase
│
└─ [ ] Manual check: Tap PCB (move accelerometer), verify data changes
```

**Test 3: QSPI Flash Memory (W25Q128JV)**
```
QSPI Flash test:
├─ [ ] Firmware command: "flash_id" or equivalent
├─ [ ] Expected output: JEDEC ID 0xEF4018 (Winbond W25Q128JV)
├─ [ ] Query firmware: "flash_capacity"
├─ [ ] Expected: 16 MB (134217728 bytes)
├─ [ ] Write/read test:
│   ├─ Write test pattern to address 0x100000
│   ├─ Read back and verify pattern matches
│   └─ Expected: No errors
│
├─ [ ] Clock verification:
│   ├─ Probe QSPI_CLK (should run at 104 MHz)
│   ├─ Check for ringing or reflections
│   └─ Amplitude: Full logic level swing (0V to 3.3V)
│
└─ [ ] If JEDEC read fails: Check CS pin, reset signal
```

**Test 4: I2C EEPROM (AT24C256C)**
```
EEPROM test:
├─ [ ] Firmware command: "eeprom_test" or manual read
├─ [ ] Expected: 32 KB (32768 bytes) accessible
├─ [ ] Write pattern to first 256 bytes:
│   ├─ Write: 0x00-0xFF at addresses 0x00-0xFF
│   ├─ Read back and verify
│   └─ Expected: Exact match
│
└─ [ ] If failure: Check I2C address (default 0x60)
```

**Acceptance criteria (all sensors):**
- [ ] I2C sensors respond to address queries (0x29, 0x60, 0x77)
- [ ] SPI IMU returns valid JEDEC ID (0x68)
- [ ] QSPI Flash returns JEDEC (0xEF4018)
- [ ] EEPROM read/write functional
- [ ] All data values change with physical stimulus (tap, rotation)

---

### 5.6 Watchdog Timer Test

**Equipment:** Timer or stopwatch (or measure time elapsed in UART output)

**Test procedure:**
```
Step 1: Trigger watchdog (force firmware into infinite loop)
├─ [ ] Via UART command: "watchdog_test"
├─ [ ] Expected: MCU halts (watchdog timer expires)
├─ [ ] Observe: NRST (reset) pin goes low briefly
└─ [ ] MCU boots up again (UART shows startup message)

Step 2: Measure reset time
├─ [ ] Time from watchdog trigger to UART output: Should be <5 seconds
├─ [ ] Hardware watchdog (IWDG): ~30 sec timeout (independent of software)
├─ [ ] Software watchdog (WWDG): ~1 sec timeout (checked by M4 core)
└─ [ ] Expected: Software watchdog triggers first (M4 monitors M7)

Step 3: Verify recovery
├─ [ ] After reset: System back to normal operation
├─ [ ] Re-run sensor tests (section 5.5)
├─ [ ] Expected: All sensors still functional (no damage from reboot)
└─ [ ] Record watchdog event in logs (proves recovery works)
```

**Acceptance criteria:**
- [✓] Watchdog triggers and resets MCU
- [✓] Reset recovery < 5 seconds
- [✓] Firmware resumes normally
- [✓] All functionality preserved after reboot

---

## Part 6: Post-Assembly & Thermal Checks

### 6.1 Thermal Imaging (IR camera)

**Equipment:** Thermal camera (FLIR, Seek Thermal, or equivalent)

**Test procedure (if equipment available):**
```
Thermal test under full load:
├─ [ ] Run firmware stress test: Max clock + all sensors active
├─ [ ] Capture IR image after 5 minutes continuous operation
├─ [ ] Expected temperatures:
│   ├─ MCU (STM32H745): <70°C (absolute max 100°C)
│   ├─ Flash (W25Q128JV): <60°C
│   ├─ Sensors: <55°C
│   └─ Inductor (buck): <80°C (highest expected)
│
├─ [ ] Hotspot analysis:
│   ├─ Identify warmest component (should be buck inductor)
│   ├─ Measure peak temperature
│   └─ Accept if <80°C; rework thermal path if >90°C
│
└─ [ ] Record thermal image for documentation
```

**If thermal camera unavailable:**
- Use touch test: Gently touch MCU and inductor after full-load run
- Acceptable: Components warm to touch, not hot enough to burn skin
- Unacceptable: Cannot hold finger on component for >1 second

---

### 6.2 Continuity & Isolation Verification

**Equipment:** Multimeter (Ohm mode)

**Final verification (before packaging):**
```
Continuity checks:
├─ [ ] GND-to-GND: All GND points <1Ω
├─ [ ] VCC_5V: All connected pads <0.5Ω
├─ [ ] VCC_3V3: All connected pads <0.5Ω
└─ [ ] VCC_1V8: All connected pads <0.5Ω

Isolation checks (all powered off):
├─ [ ] VCC_5V to GND: >10 MΩ (megohms)
├─ [ ] VCC_3V3 to GND: >10 MΩ
├─ [ ] VCC_1V8 to GND: >10 MΩ
└─ [ ] Any other rail to GND: >10 MΩ
```

---

## Part 7: Environmental Stress Testing (Phase 4)

### 7.1 Temperature Cycling

**Equipment needed:**
- Temperature-controlled chamber (-10 to +85°C)
- Or: freezer (-10°C) + oven (+85°C) for manual cycling

**Test procedure:**
```
Temperature cycling: 10 cycles (-10°C to +85°C)
├─ Cycle timing: 2 hours per cycle
│  ├─ Phase 1: Ramp to -10°C (30 min)
│  ├─ Phase 2: Hold at -10°C (20 min)
│  ├─ Phase 3: Ramp to +85°C (60 min, slow to allow thermal equilibrium)
│  ├─ Phase 4: Hold at +85°C (20 min)
│  └─ Phase 5: Ramp back to +25°C (30 min)
│
├─ [ ] Between cycles: Power on, run functional test (section 5)
├─ [ ] Expected: All tests pass after each cycle
├─ [ ] Watch for: Solder cracks, component delamination
└─ [ ] After 10 cycles: Inspect under magnifier for solder cracks

Acceptance criteria:
├─ [ ] All 10 cycles completed without thermal runaway
├─ [ ] No visible solder cracks
├─ [ ] Functional tests pass after final cycle
└─ [ ] No excessive component shift
```

---

## Part 8: Documentation & Traceability

### 8.1 Manufacturing Log

**For each PCB assembly, record:**

```
Board Serial: STM32H745-PROTO-001
Date manufactured: 2026-09-25
Assembled by: [Name]
Reviewed by: [Name]

Component traceability:
├─ U1 MCU: S/N 45926BDJ (from supplier lot 2026Q3-A)
├─ U5 Flash: Part code 201906 (date code: June 2019, acceptable)
├─ Crystal X1: S/N ECS201526 (verified 8.000 MHz ±0.5 ppm)
└─ All other passives: Standard commercial stock

Assembly notes:
├─ Solder paste: Kester R708LT87 (mixed 2026-08-15)
├─ Reflow profile: SAC305, 9:30 min total time
├─ Yield: 4/5 boards passed PAT on first attempt
└─ Rework: U2 TPS62133A re-soldered (cold joint on pin 3)

Functional test results:
├─ Power-on test: PASS
├─ Clock @ 104 MHz: PASS
├─ UART loopback @ 921.6k: PASS
├─ I2C sensor discovery: PASS (all 3 sensors)
├─ SPI IMU ID: PASS (0x68)
├─ QSPI Flash ID: PASS (0xEF4018)
├─ Watchdog reset: PASS
└─ Thermal check: <60°C after 5 min load

Notes:
├─ Minor solder bridge between U3 pins 2-3 (cleaned with solder wick)
├─ Component tombstoning on C14 (reflowed, now correct)
└─ All issues resolved before bringup phase
```

---

## Appendix A: IPC Standards Reference

### IPC-A-610G (Assembly Quality)

**Solder joint classifications:**
- **Level 1 (Class 1):** General electronics (acceptable defects)
- **Level 2 (Class 2):** Dedicated industrial use (fewer defects allowed)
- **Level 3 (Class 3):** Military/aerospace (zero-defect expectation)

**Our prototype:** Target Level 2 (Class 2) - acceptable for field validation, not production

### IPC-A-600 (PCB Acceptability)

**Trace width tolerance:** ±0.1 mm from design
**Via plating:** Minimum 0.05 mm copper thickness
**Solder mask clearance:** 0.1 mm minimum on vias
**Silk screen legibility:** 0.5 mm minimum text height

---

## Appendix B: Component Care & Storage

**Temperature & humidity storage:**
- Temperature: 20-25°C (avoid condensation during temperature changes)
- Humidity: 45-55% RH (below 60% to prevent corrosion)
- Duration: Passives indefinite; ICs max 12 months (reseal bags if exposed)

**Baking procedure (for components >6 months old):**
- Temperature: 125°C ±5°C
- Duration: 2 hours in dry-box oven
- Cool to room temp before use (prevents moisture absorption during soldering)
- Critical for: BGAs, fine-pitch QFNs, flex-substrate packages

---

**Document Status:** Ready for manufacturing  
**Next Phase Gate:** Prototype Bring-Up & Field Validation  
**Prepared by:** Manufacturing Engineering (AI Assistant)  
**Approval Required:** Quality Assurance, Manufacturing Manager
