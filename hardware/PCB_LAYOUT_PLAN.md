# STM32H745 Ballistic Corrector - PCB Layout Plan & Strategy

**Document Version:** 1.0  
**Date:** 2026-07-28  
**Phase:** 2 - PCB Layout & Simulation  
**Target Completion:** 2026-08-11 (2 weeks)  
**Status:** In Progress

---

## Executive Summary

This document defines the PCB layout strategy for the STM32H745 Ballistic Corrector prototype. The design targets:
- **4-layer PCB** (1.6mm FR-4, 2oz copper)
- **Impedance-controlled QSPI traces** (50Ω ±5%)
- **Thermal performance:** No component >60°C @ full load
- **Manufacturing compliance:** IPC-A-600 Class 2
- **Lead time:** 2-3 weeks from finalized Gerbers

---

## Part 1: PCB Stackup & Layer Definition

### Layer Stack-up (4-layer, 1.6mm)

```
Layer 1: Signal + Power (Top)
  - PCB thickness: 0.1mm (copper: 35µm/2oz)
  - Core: FR-4 1.35mm
  
Layer 2: GND Plane (continuous)
  - PCB thickness: 0.035mm (copper: 35µm/1oz)
  
Layer 3: POWER PLANE (for main rails: VCC_5V, VCC_3V3)
  - PCB thickness: 0.035mm (copper: 35µm/1oz)
  
Layer 4: Signal + Power (Bottom)
  - PCB thickness: 0.1mm (copper: 35µm/2oz)

Total stackup: 0.1 + 1.35 + 0.035 + 0.035 + 0.1 = 1.62mm ✅
```

### Impedance Control Targets

**QSPI Clock & Data Lines (50Ω differential or single-ended):**
- Clock (QSPI_CLK): 50Ω single-ended, 108 MHz
- Data lines (QSPI_D0-D3): 50Ω single-ended, 104 MHz
- Control signals (CS_QSPI, WP, HOLD): 50Ω single-ended

**SPI1 (10 MHz, less critical but controlled):**
- Clock, MOSI, MISO: 50Ω, routed on top layer
- Chip select (CS_IMU): Non-critical

**I2C (400 kHz, no impedance control needed):**
- SCL, SDA: Routed together with pull-ups (10k typical)

### Via Specifications

**Signal vias:**
- Pad diameter: 0.3mm
- Drill size: 0.15mm
- Aspect ratio: 1.62mm / 0.15mm = 10.8 (acceptable for manufacturing)

**High-current vias (VCC/GND):**
- Pad diameter: 0.4mm
- Drill size: 0.2mm
- Minimum via count per power node:
  - VCC_12V input: 4 vias
  - VCC_5V (buck output): 6 vias
  - VCC_3V3: 4 vias
  - VCC_1V8: 2 vias
  - GND (return path): 8+ vias per major node

---

## Part 2: Component Placement Strategy

### Placement Hierarchy

**Priority 1: Power Distribution & Thermal (Bottom-left corner)**
```
┌─────────────────────────────┐
│ TPS62133A Buck Converter     │ ← Lowest thermal, smallest footprint
│ L1 (4.7µH inductor)          │
│ C_bulk (10µF input)          │
│ C_output (10µF output)       │
│ D1 Schottky (1N5819)         │
└─────────────────────────────┘
```

**Priority 2: MCU & Critical Clock (Center)**
```
┌────────────────────────────────┐
│ U1: STM32H745ZIT6 (LQFP-144)   │ ← Largest component, central location
│ X1: 8 MHz Crystal              │ ← Near XTAL pins
│ C_xtal: 20pF caps              │
│ C_bypass: 100nF × 8 (decaps)   │ ← Around MCU pins
│ C_bulk: 10µF × 2               │
└────────────────────────────────┘
```

**Priority 3: Sensors (Distributed, minimize trace length)**
```
Top-left:    ICM-20689 (SPI IMU @ 10 MHz)
Top-right:   VL53L0X (I2C rangefinder)
Bottom-left: MCP9808 (I2C barrel temp)
Bottom-right: BMP390 (I2C pressure/ambient)
```

**Priority 4: Memory (Right side)**
```
┌─────────────────────────┐
│ U6: W25Q128JV (QSPI)    │ ← High-speed, 16-pin SOIC-8-W
│ U7: AT24C256C (EEPROM)  │ ← I2C, small footprint
└─────────────────────────┘
```

**Priority 5: Communication (Top edge)**
```
J1: USB HS Mini-B (top-left corner)
U8: CP2102N UART (top-center)
J2: UART debug connector (top-right)
J3: SWD debug connector (right edge)
```

### Thermal Placement Considerations

**Heat-generating components** (dissipation estimate):
- TPS62133A buck: ~200 mW (1.5 A @ 5V output)
- STM32H745 (dual core @ full load): ~530 mW
- ICM-20689 (SPI @ 10 MHz): ~50 mW
- Flash W25Q128JV (QSPI @ 104 MHz): ~80 mW

**Cooling strategy:**
- Separate buck converter from MCU by >2cm
- Place MCU center with balanced decap distribution
- No components directly over MCU (leave top free for heatsink if needed)
- Ground plane immediately under MCU for thermal spreading
- Thermal vias (0.3mm × 8) under MCU PAD to ground plane

### Component Placement DFM Constraints

**Minimum spacing:**
- Edge clearance: >0.5mm from board edge (for routing clearance)
- Component-to-component: >0.2mm (pick & place machine minimum)
- Connectors to edge: >1mm (manual soldering access)

**Silk screen clearance:**
- 0.2mm minimum from component pads (for legibility + mask precision)
- Component reference designators (U1, C1, etc.) positioned ≥2mm from pads

**Solder mask:**
- 0.1mm minimum clearance around via pads
- 0.15mm minimum between adjacent pads (prevent solder bridges)
- Non-plated holes: 0.3mm minimum spacing

---

## Part 3: Routing Strategy

### Critical Nets (Impedance-Controlled)

**QSPI traces (highest priority):**
```
Trace width calculation:
Zo = 50Ω target
For microstrip on 4-layer stackup (signal on layer 1, GND on layer 2):
- PCB thickness to GND: ~1.35mm
- Trace width: ~0.25mm (adjust via simulation)
- Trace thickness: 35µm

All QSPI traces routed on TOP LAYER only:
├── QSPI_CLK (from PA6)
├── QSPI_D0 (from PA7, output data)
├── QSPI_D1 (from PA8, output data)
├── QSPI_D2 (from PA9, output data)
├── QSPI_D3 (from PA10, output data)
├── QSPI_NCS (from PA11, chip select)
├── QSPI_WP (from PB2, write protect)
└── QSPI_HOLD (from PB1, hold)

Routing rules:
- Length matching: CLK ± 50mm to all data lines (104 MHz ≤ 0.1ns skew)
- No routing under W25Q128JV IC
- Return path on GND plane immediately below traces
- Serpentine spacing for length matching: 0.5mm parallel spacing
- No vias until termination pads on Flash IC
```

**SPI1 traces (medium priority):**
```
SPI1_CLK:  PA5 → U4 (ICM-20689)  [10 MHz, routed on top]
SPI1_MOSI: PA7 → U4             [routed on top]
SPI1_MISO: PA6 → U4             [routed on top]
SPI1_CS:   PA4 → U4             [routed on top]

Routing rules:
- No length matching required (<100 ns)
- Clock separate from data lines (>0.2mm spacing to prevent crosstalk)
- Ground return path via GND plane
```

**I2C traces (low priority):**
```
SCL_SENSORS: PB10 → U5, U6, U7, U10, U11 (400 kHz)
SDA_SENSORS: PB11 → U5, U6, U7, U10, U11

Pull-ups: 10k from VCC_3V3 to SCL/SDA
Routing:
- Can route on top or bottom layer
- No impedance control needed (low-frequency)
- Routed together with >0.2mm spacing (capacitive coupling)
```

### Power Distribution Routing

**12V Input rail:**
```
VCC_12V: J_power → D1 (reverse-polarity) → L1 inductor → TPS62133A input
- Trace width: 0.5mm (supports >5A)
- Return path: Ground plane (8-pin via array directly under TPS62133A)
```

**5V output (buck):**
```
TPS62133A_OUT → L1 → C_out (10µF) → VCC_5V_rails
- Trace width: 0.4mm (supports 1.5A continuous)
- Heavy via array (6+ vias) at buck output
- Distributed decap (4× 10µF, 8× 100nF) across 5V rails
```

**3.3V LDO:**
```
VCC_5V → U3_LDO_IN → C_input (1µF) → VCC_3V3
- Return directly to ground plane
- Decap: 8× 100nF + 2× 10µF distributed around MCU
```

**1.8V LDO:**
```
VCC_5V → U4_LDO_IN → C_input (1µF) → VCC_1V8
- Supplies flash memory (W25Q128JV I/O voltage)
- Decap: 2× 100nF local to IC
```

### Ground Plane Strategy

**Layer 2: Continuous GND plane**
- No cutouts (solid ground reference for all signals)
- Thermal vias under MCU: 8× 0.3mm vias connecting top layer GND to plane
- Star grounding: All connectors ground to single point on PCB edge

**Return path optimization:**
- Power return (GND) uses plane; signal return uses bottom copper fill
- Via stitching along QSPI signal boundaries to prevent loop area

---

## Part 4: Thermal Analysis Requirements

### Thermal FEA Simulation Targets

**Software:** ANSYS Thermal (or equivalent: COMSOL, Altium DXL)

**Boundary conditions:**
- Ambient temperature: 25°C still air
- Full load: M7 @ 480 MHz + M4 @ 240 MHz + QSPI @ 104 MHz
- Power dissipation map:
  ```
  TPS62133A: 200 mW (distributed as Joule heating in inductor)
  STM32H745: 530 mW (concentrated under MCU center)
  ICM-20689: 50 mW (localized)
  W25Q128JV: 80 mW (localized)
  Total: ~860 mW thermal load
  ```

**Thermal design targets:**
- No component >60°C (max operating temp for passive components)
- MCU junction <85°C (STM32H7 absolute max 100°C)
- Flash memory <70°C (W25Q128JV rated to 85°C)
- Sensor ICs <70°C (ICM-20689 rated to 85°C)

**Simulation setup:**
1. Import PCB stackup (4-layer, 1.6mm, FR-4 κ~0.3 W/m·K)
2. Define component thermal models:
   - STM32H745: ~5×5mm die area, 530 mW dissipation
   - Buck converter inductor: ~0.3 W concentrated in core
   - Package thermal resistances (from datasheets)
3. Mesh: 0.5mm elements near hot spots, coarser away
4. Convection boundary: 5 W/m²·K (natural convection, still air)
5. Run transient simulation: 0-300 seconds (steady-state @ 5 min)

**Expected outcome:** Temperature contour map showing all components <60°C

**Mitigation if needed:**
- Add thermal vias (via array) under MCU
- Consider thermal pad on bottom layer
- Increase copper pour around high-dissipation components
- Future: Add small heatsink on bucket converter if needed

---

## Part 5: Signal Integrity Analysis Requirements

### QSPI Timing Analysis

**Target:** Verify 108 MHz operation with <0.1ns skew

**Analysis tool:** HyperLynx (SI), Altium DXL, or ADS Momentum

**Signal characteristics:**
- Clock: QSPI_CLK @ 108 MHz, ~2 ns rise/fall time
- Data: QSPI_D0-D3 @ 104 MHz, ~2 ns rise/fall time
- PCB propagation delay: ~5 ps/mm (microstrip on FR-4)

**Simulation setup:**
1. Import Gerber files (after layout complete)
2. Define driver impedance (MCU output ~50Ω)
3. Define trace impedance (50Ω target)
4. Define receiver impedance (Flash IC input ~50Ω)
5. Define PCB dielectric loss (tan δ ≈ 0.02 for FR-4)

**Analysis checklist:**
- [ ] Rise/fall time at receiver: >1.5 ns (avoid overshoot)
- [ ] Skew between data lines: <0.1 ns (bits must arrive together)
- [ ] Clock-to-data skew: <0.15 ns (timing window)
- [ ] Ringing amplitude: <200 mV (10% of 2V logic level)
- [ ] No reflections at impedance discontinuities

**Expected result:** All traces meet timing requirements with 50Ω ±5% impedance

### EMI / Crosstalk Analysis

**High-speed noise sources:**
- QSPI @ 108 MHz: ~4th harmonic at 432 MHz
- SPI1 @ 10 MHz: negligible EMI
- USB HS OTG (future v1.1): 480 Mbps, requires differential pair control

**Analysis approach:**
1. Calculate loop inductance for QSPI clock/data returns
2. Verify GND plane continuity under all high-speed traces
3. Check for coupling between:
   - QSPI traces and analog sensor inputs (ICM-20689 accelerometer)
   - Clock lines and power supply rails
   - I2C lines and high-speed digital signals

**Mitigation:**
- Ground plane underneath all high-speed traces
- 0.2mm minimum spacing between QSPI and analog signals
- Series ferrite filter on clock line to MCU if needed (added to BOM: Würth 742 792 series)
- Separate analog ground (star point) from digital ground

---

## Part 6: Manufacturing Design Rules Check (DRC)

### Fabrication Constraints (4-layer, 1.6mm)

**Trace width & spacing:**
```
Minimum trace width:  0.15 mm (for <1A signals)
Minimum spacing:      0.1 mm (between traces)
Power traces (>1A):   0.3 mm minimum width

Trace current capacity (1oz copper):
- 0.15 mm width: 0.5 A
- 0.25 mm width: 0.8 A
- 0.5 mm width: 1.5 A
- 1.0 mm width: 3.0 A

Buck converter output (1.5A) → 0.4 mm width minimum
```

**Via specifications:**
```
Signal via: 0.15 mm drill, 0.3 mm pad (min spacing 0.2mm)
Power via: 0.2 mm drill, 0.4 mm pad (min spacing 0.15mm)

Via hole via: Not recommended (aspect ratio >12)
```

**Clearance requirements:**
```
Trace to pad:           0.1 mm
Trace to via:           0.1 mm
Pad to pad:             0.15 mm (solder bridge prevention)
Edge to trace:          0.5 mm (prevent delamination)
Edge to via:            0.3 mm (structural integrity)
Silk screen to pad:     0.2 mm (legibility + mask accuracy)
```

### Layer Stackup DRC

**Impedance tolerance stack:**
```
Target: 50Ω ±5% (47.5-52.5Ω)

Tolerance contributors:
- Trace width: ±0.025 mm (tolerance: ±5%)
- PCB thickness: ±0.1 mm (tolerance: ±6%)
- Dielectric constant: ±2% (ε_r = 4.3 ± 0.09)

Simulation: Account for manufacturing tolerance stack
- Run field solver with +10/-10% trace width variation
- Verify all variations stay within 50Ω ±10% (worst-case)
```

---

## Part 7: Testing Points & Debug Features

### Test pads (for manufacturing PAT)

**Power rails test points:**
```
TP1: VCC_12V_IN    (before Schottky diode) → Multimeter check
TP2: VCC_5V_OUT    (buck converter output) → Voltage regulator verification
TP3: VCC_3V3       (LDO output) → Reference voltage check
TP4: VCC_1V8       (analog LDO output) → Flash memory bias
TP5: GND_REF       (signal ground reference) → Common return
```

**Clock & timing test points:**
```
TP6: XTAL_IN       (crystal input, before buffer)
TP7: XTAL_OUT      (crystal output)
TP8: QSPI_CLK      (high-speed clock output)
TP9: SPI1_CLK      (IMU clock output)
```

**Sensor communication test points:**
```
TP10: SPI1_MOSI    (IMU data input)
TP11: SPI1_MISO    (IMU data output)
TP12: I2C_SCL      (sensor bus clock)
TP13: I2C_SDA      (sensor bus data)
TP14: QSPI_D0      (Flash data bit 0, quad output)
```

**Debug connectors (existing):**
- J3: SWD (10-pin ARM debug, ST-Link compatible)
- J2: UART debug @ 921.6k baud (TX/RX/GND 3-pin header)

### In-Circuit Test (ICT) Nodes

**Bed-of-nails test fixture points:**
- All test pads (TP1-TP14) must be accessible from bottom side
- Via array under connectors for probe access
- Minimum 0.5mm pad diameter for standard ICT probes

---

## Part 8: Gerber File Generation & Quality Assurance

### Gerber File Set (from KiCad)

**Standard Gerber files:**
```
STM32H745_Ballistic_Corrector-F_Cu.gbr    (Top copper layer)
STM32H745_Ballistic_Corrector-In1_Cu.gbr  (Layer 2 GND plane)
STM32H745_Ballistic_Corrector-In2_Cu.gbr  (Layer 3 power plane)
STM32H745_Ballistic_Corrector-B_Cu.gbr    (Bottom copper layer)

STM32H745_Ballistic_Corrector-F_SilkS.gbr (Top silk screen)
STM32H745_Ballistic_Corrector-B_SilkS.gbr (Bottom silk screen)

STM32H745_Ballistic_Corrector-F_Mask.gbr  (Top solder mask)
STM32H745_Ballistic_Corrector-B_Mask.gbr  (Bottom solder mask)

STM32H745_Ballistic_Corrector-Edge_Cuts.gbr (Board outline)
STM32H745_Ballistic_Corrector-Drill.xln    (Drill file, Excellon format)
STM32H745_Ballistic_Corrector-Drill_NPTH.xln (non-plated holes, mounting)
```

**Supplementary files:**
```
STM32H745_Ballistic_Corrector_BOM.csv       (Bill of Materials)
STM32H745_Ballistic_Corrector_DesignRules.txt (PCB specs: thickness, layer stack)
STM32H745_Ballistic_Corrector_IPC_Notes.txt (IPC-A-600 Class 2 acceptance criteria)
```

### Pre-manufacturing DFM Review Checklist

- [ ] Trace width verification: All traces ≥0.15 mm (or wider per current requirements)
- [ ] Via check: All vias 0.15 mm min drill (or 0.2 mm for power)
- [ ] Clearance check: Traces/vias to edges >0.5 mm
- [ ] Plane continuity: GND plane has no cutouts; power plane only contains required paths
- [ ] Component clearance: No components overlap; min spacing 0.2 mm
- [ ] Silk screen legibility: All reference designators visible, >0.5mm text
- [ ] Solder mask: No mask over pads; 0.1 mm clearance on vias
- [ ] Copper fill: No isolated copper islands (connect to GND or power plane)
- [ ] Test point accessibility: All TP pads accessible for ICT probes (0.5 mm min)
- [ ] Connector placement: No traces under connectors (for rework access)
- [ ] Design rule check report: Zero errors; all warnings reviewed

---

## Part 9: Fabrication Supplier Selection & RFQ

### Supplier Candidates (PCB Fabrication)

| Supplier | Lead Time | Min Order | Cost/Unit (1-2) | Capabilities | Notes |
|----------|-----------|-----------|-----------------|--------------|-------|
| **Sierra Circuits** | 5-7 days | 1 | $45-60 | 4-layer, 50Ω control, flex | Premium quality, DFM review |
| **JLCPCB** | 10-14 days | 5 | $8-15 | 4-layer, impedance, assembly | Best value, slower |
| **OSH Park** | 14-21 days | 3 | $35-50 | 4-layer, no impedance control | Open source friendly |
| **Oshpark 3-day** | 3 days | 3 | $80-120 | 4-layer standard | Emergency turnaround |
| **Advanced Circuits** | 7-10 days | 1 | $40-70 | 4-layer, 50Ω, high volume ready | Small volume friendly |

### Request for Quote (RFQ) Template

```
Project: STM32H745 Ballistic Corrector - Prototype PCB
Quantity: 5 pieces (prototype batch)
Delivery: Target 2026-08-20 (2 weeks max)

PCB Specifications:
├─ Material: FR-4, IPC-4101 Class 2.4
├─ Thickness: 1.6 mm ±0.1 mm
├─ Layers: 4-layer
├─ Copper weight: 2oz (top/bottom), 1oz (internal)
├─ Surface finish: HASL (lead-free)
├─ Solder mask: Black (top + bottom, oil-based or LPI)
├─ Silk screen: White (top + bottom, epoxy)
├─ Impedance control: 50Ω ±5% for QSPI traces (document: PCB_LAYOUT_PLAN.md)

Deliverables:
├─ 5 × bare PCBs (vacuum-packed, anti-static bag)
├─ Fabrication report (trace widths, layer uniformity)
├─ Impedance verification report (5 test coupons)
├─ Drill file validation (Excellon format)

Special Requirements:
├─ DFM review before fabrication start
├─ Edge treatment: 0.3 mm chamfer (tool marks acceptable)
├─ Panelization: Single PCB per panel recommended (tool marks away from edges)
├─ RoHS compliant? YES

Budget: ~$40-80 total (5 PCBs)
```

### Assembly Partner Selection

**Options:**
1. **Self-assembly:** Manual pick & place + reflow (2-3 weeks, learn curve)
2. **Local assembly house:** ~$500 NRE setup + $80-120/board labor (smaller run <10)
3. **Industrial partner:** JLCPCB assembly, $50-80/board (10+ minimum)

**Recommendation for prototype:**
- Fabricate 5 PCBs (bare boards only)
- Assemble 2-3 boards in-house (manual pick & place, small reflow oven)
- Use 2 boards as spares for rework/testing

---

## Part 10: Next Steps & Timeline

### Phase 2 Deliverables Checklist

- [ ] **Week 1 (2026-07-28 - 2026-08-04):**
  - [ ] Complete KiCad PCB layout (schematic to layout export)
  - [ ] Route critical QSPI traces (50Ω impedance control)
  - [ ] Complete power distribution routing
  - [ ] Perform DRC verification (zero design errors)
  - [ ] Generate initial Gerber files for review

- [ ] **Week 2 (2026-08-04 - 2026-08-11):**
  - [ ] Signal integrity simulation (QSPI @ 108 MHz)
  - [ ] Thermal FEA analysis (component placement optimization)
  - [ ] DFM review with manufacturing partner
  - [ ] Finalize Gerber files and BOM for production
  - [ ] RFQ submission to fabrication partners
  - [ ] Component purchase orders (long-lead items)

### Critical Path

```
2026-07-28: Start PCB layout
2026-08-04: Routing complete, first Gerber generation
2026-08-08: Signal integrity & thermal analysis complete
2026-08-11: Final Gerber files locked, RFQ submitted ←─ Phase 2 Gate
2026-08-18: PCB fabrication starts (lead time: 2-3 weeks)
2026-09-08: PCB delivery (5 boards) ←─ Phase 3 Gate: Prototype Assembly
```

### Risk & Mitigation

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|-----------|
| QSPI impedance > ±10% | Medium | High | Use HyperLynx SI; work with manufacturer on impedance |
| Thermal > 60°C @ full load | Low | Medium | Add thermal vias under MCU; iterate design if needed |
| DFM issues found late | Medium | Medium | Schedule DFM review with supplier early (week 1) |
| Component long-lead shortage | Low | High | Dual-source MCU alternatives; place orders immediately |
| PCB delivery delay | Low | Medium | Use 3-day expedited option if needed (Sierra Circuits) |

---

## Appendix A: Reference Material

### PCB Design Standards
- **IPC-A-600G:** Acceptability of Printed Boards
- **IPC-2220B:** Generic Standard on Printed Board Design
- **IPC-7095:** Design and Assembly Process Implementation for BGAs
- **IEC 61249-2-21:** Copper clad laminated sheets for PCBs

### Thermal Analysis References
- **JEDEC JEP106:** Component thermal models and junction temperature
- **MIL-HDBK-217F:** Reliability predictions of electronic equipment

### Signal Integrity References
- **Bogatin, "Signal Integrity - Simplified"** (2nd ed.)
- **Johnson & Graham, "High-Speed Digital Design"**
- **Altera/Intel SI Design Guide for high-speed interfaces**

### Manufacturing References
- **STM32H745 datasheet:** Operating conditions, thermal characteristics
- **W25Q128JV datasheet:** QSPI timing specifications, operating temperature
- **TPS62133A datasheet:** Efficiency curves, thermal models

---

## Appendix B: Key Contacts

| Role | Responsibility | Notes |
|------|---|---|
| Hardware Design Lead | PCB layout, schematic finalization | TBD - Internal owner |
| Manufacturing Engineer | DFM, supplier management | TBD - Coordinate with fab partner |
| SI/PI Engineer | Signal integrity, thermal analysis | TBD - External contractor if needed |
| Quality Assurance | Test procedure development | TBD - Prepare ICT fixture specs |

---

**Document Status:** Ready for review  
**Next Phase Gate:** PCB Fabrication Quote & Component Procurement  
**Prepared by:** Embedded Systems Design (AI Assistant)  
**Approval Required:** Hardware Design Lead, Manufacturing Engineer
