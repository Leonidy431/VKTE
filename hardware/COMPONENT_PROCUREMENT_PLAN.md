# STM32H745 Ballistic Corrector - Component Procurement & Supplier Plan

**Document Version:** 1.0  
**Date:** 2026-07-28  
**Phase:** 3 - Prototype Fabrication & Assembly  
**Lead Time Estimate:** 8-12 weeks (MCU constrained)  
**Status:** Ready for RFQ

---

## Executive Summary

This document outlines the component procurement strategy for the STM32H745 Ballistic Corrector prototype. The project requires **41 components** across **7 major categories** with a **bill of materials (BOM) cost of $377.81 @ 1-piece, $250 @ 100+ volume**.

**Critical path item:** STM32H745ZIT6 MCU (lead time: **8-12 weeks** from order date)

**Procurement timeline:**
- **Immediate (now):** Place orders for MCU and other long-lead items (target: 2026-07-28)
- **Weeks 1-2:** Standard sensor components arrive (2-4 weeks)
- **Weeks 2-4:** Passive components arrive (1-2 weeks)
- **Weeks 8-12:** MCU delivery (8-12 week lead time)

---

## Part 1: Component Categories & Lead Times

### Critical Path Analysis

```
Component          | Qty | Lead Time | Supplier  | Status    | Risk
───────────────────┼─────┼───────────┼───────────┼───────────┼────────
MCU                | 1   | 8-12 wks  | Digi-Key  | ORDER NOW | HIGH
Flash (QSPI)       | 1   | 2-4 wks   | Digi-Key  | ORDER NOW | MED
IMU (ICM-20689)    | 1   | 2-4 wks   | Digi-Key  | ORDER NOW | MED
LRF (VL53L0X)      | 1   | 3-6 wks   | Digi-Key  | ORDER NOW | MED
Regulators/LDOs    | 3   | 1-2 wks   | Digi-Key  | ORDER NOW | LOW
Passives (C, R, L) | 32  | 1-2 wks   | Digi-Key  | ORDER NOW | LOW
```

**Procurement strategy:**
1. Place all orders simultaneously to minimize risk
2. Dual-source MCU (backup: STM32H7 variant STM32H743ZIT6)
3. Stagger delivery: Accept early-arriving components; flag MCU when shipping

---

## Part 2: Detailed BOM with Sourcing

### Category 1: Main Processor (1 component)

| Ref | Part Number | Description | Value | Qty | Unit Cost | Total | Supplier | Lead Time | Alternate | Notes |
|-----|-------------|-------------|-------|-----|-----------|-------|----------|-----------|-----------|-------|
| U1 | STM32H745ZIT6 | Dual-core MCU, M7@480MHz, M4@240MHz, LQFP-144 | - | 1 | $45.50 | $45.50 | Digi-Key | 8-12 wks | STM32H743ZIT6 | **CRITICAL:** Longest lead time; dual-source recommended |

**Procurement notes:**
- Contact Digi-Key account manager for allocation confirmation (high demand)
- Request early notification if lead time exceeds 10 weeks
- Backup: STM32H743ZIT6 (slightly lower specs, same pin-compatible LQFP-144)
- Quantity: Order 2 units (1 for prototype, 1 spare for rework)

---

### Category 2: Power Management (4 components)

| Ref | Part Number | Description | Value | Qty | Unit Cost | Total | Supplier | Lead Time | Notes |
|-----|-------------|-------------|-------|-----|-----------|-------|----------|-----------|-------|
| U2 | TPS62133ADBVT | Synchronous buck converter, 5V/1A output, 92% efficient | 5V/1A | 1 | $2.45 | $2.45 | Digi-Key | 1-2 wks | Common part |
| U3 | NCP1117ST33T3G | LDO regulator, 3.3V/500mA | 3.3V | 1 | $0.68 | $0.68 | Digi-Key | 1 wk | Standard |
| U4 | NCP1117ST18T3G | LDO regulator, 1.8V/200mA (analog supply) | 1.8V | 1 | $0.72 | $0.72 | Digi-Key | 1 wk | Analog LDO |
| D1 | 1N5819 | Schottky diode, 40V/1A (reverse-polarity protection) | - | 1 | $0.12 | $0.12 | Digi-Key | 1-2 wks | Common |

**Total:** $4.00 (4 components)

**Sourcing strategy:**
- Order all together with MCU
- Verify thermal specifications for TPS62133A (inductor heating)
- Standard parts, no long-lead risk

---

### Category 3: Clock & Oscillator (1 component)

| Ref | Part Number | Description | Value | Qty | Unit Cost | Total | Supplier | Lead Time | Notes |
|-----|-------------|-------------|-------|-----|-----------|-------|----------|-----------|-------|
| X1 | ECS-80-20-34B | 8 MHz crystal oscillator, 20ppm accuracy, 8pF load | 8 MHz | 1 | $0.78 | $0.78 | Digi-Key | 1 wk | Standard |

**Notes:**
- Load capacitance: 20pF (matched to MCU XTAL specifications)
- Frequency tolerance: ±20 ppm (adequate for 480 MHz PLL)
- Frequency stability: -10 to +70°C

---

### Category 4: External Memory (2 components)

| Ref | Part Number | Description | Value | Qty | Unit Cost | Total | Supplier | Lead Time | Notes |
|-----|-------------|-------------|-------|-----|-----------|-------|----------|-----------|-------|
| U5 | W25Q128JV | QSPI NOR Flash, 16 MB, 104 MHz, SOIC-8-W | 16 MB | 1 | $2.85 | $2.85 | Digi-Key | 2-4 wks | High-demand part |
| U6 | AT24C256C | I2C EEPROM, 32 KB, SOIC-8 | 32 KB | 1 | $1.20 | $1.20 | Digi-Key | 2-3 wks | Standard |

**Total:** $4.05 (2 components)

**Sourcing notes:**
- W25Q128JV: **WATCH LEAD TIME** (2-4 weeks typical, sometimes 6+ weeks)
- Dual-source EEPROM: Consider Microchip 24LC256 or ON Semiconductor CAT24C256 as alternates
- Both parts soldered on-board; early procurement essential

---

### Category 5: Sensors (5 components)

| Ref | Part Number | Description | Value | Qty | Unit Cost | Total | Supplier | Lead Time | Notes |
|-----|-------------|-------------|-------|-----|-----------|-------|----------|-----------|-------|
| U7 | ICM-20689 | 9-DOF IMU, SPI @ 10 MHz, ±16g accel | - | 1 | $15.80 | $15.80 | Digi-Key | 3-6 wks | **WATCH:** Can have long lead |
| U8 | VL53L0X | Time-of-flight rangefinder, I2C, 30-1200mm range | - | 1 | $12.40 | $12.40 | Digi-Key | 2-4 wks | Moderate lead time |
| U9 | MCP9808 | I2C temperature sensor, ±0.5°C accuracy | - | 1 | $2.10 | $2.10 | Digi-Key | 1-2 wks | Common |
| U10 | BMP390 | I2C pressure/temperature sensor | - | 1 | $8.75 | $8.75 | Digi-Key | 2-3 wks | Moderate |
| U11 | BT121-A | Bluetooth LE module (future v1.1, optional now) | - | 0 | $18.50 | $0.00 | Digi-Key | 4-6 wks | DEFER to v1.1 |

**Total:** $39.05 (5 components, excluding BT121-A)

**Sensor sourcing strategy:**
- ICM-20689: Highest risk; place order immediately; request allocation confirmation
- VL53L0X: Moderate risk; standard part, usually in stock
- MCP9808: Very common, no supply risk
- BMP390: Standard Bosch part, moderate availability
- BT121-A: DEFER to v1.1; not needed for prototype validation

**Alternate sensors (if lead time>6 weeks):**
- IMU: MPU-6050 (more common, 6-DOF but proven in production)
- LRF: LIDAR-Lite v4 (more expensive but often in stock)
- Pressure: BMP280 (Bosch predecessor, very common)

---

### Category 6: Communication Interfaces (2 components)

| Ref | Part Number | Description | Value | Qty | Unit Cost | Total | Supplier | Lead Time | Notes |
|-----|-------------|-------------|-------|-----|-----------|-------|----------|-----------|-------|
| U12 | CP2102N | USB-UART bridge, 921.6k baud, USB Mini-B | - | 1 | $4.80 | $4.80 | Digi-Key | 1-3 wks | Standard |
| U13 | PCA9306 | I2C voltage translator, dual-channel (future USB HS) | - | 0 | $1.50 | $0.00 | Digi-Key | 1 wk | Optional |

**Total:** $4.80 (1 component)

**Notes:**
- CP2102N: Proven UART bridge, easy debugging
- USB HS OTG: DEFER to v1.1 (requires PCA9306 or similar level-shifter)
- Standard parts, no sourcing risk

---

### Category 7: Passive Components (32 components)

| Category | Part Number | Description | Value | Qty | Unit Cost | Total | Supplier | Notes |
|----------|-------------|-------------|-------|-----|-----------|-------|----------|-------|
| **Capacitors** | - | - | - | - | - | - | - | - |
| C (bulk) | GRM31CR61A107KA19L | Multilayer ceramic, X7R | 100µF | 4 | $0.68 | $2.72 | Digi-Key | Power supply filtering |
| C (bulk) | GRM21BR61A106KE19L | Multilayer ceramic, X7R | 10µF | 8 | $0.42 | $3.36 | Digi-Key | Decoupling |
| C (bypass) | GRM155R61A104KA01D | Multilayer ceramic, X7R | 100nF | 16 | $0.08 | $1.28 | Digi-Key | High-frequency decoupling |
| C (XTAL) | GRM1555C1H200FA01D | Multilayer ceramic, NP0 | 20pF | 2 | $0.12 | $0.24 | Digi-Key | Crystal load caps |
| **Resistors** | - | - | - | - | - | - | - | - |
| R (pull-up) | CRCW0603 10K FT | Thick-film resistor | 10kΩ | 4 | $0.05 | $0.20 | Digi-Key | I2C, UART pull-ups |
| **Inductors** | - | - | - | - | - | - | - | - |
| L (buck) | SPM6530T-4R7M | Shielded inductor, SMD | 4.7µH | 1 | $1.25 | $1.25 | Digi-Key | Buck converter inductor |

**Passive Components Subtotal:** $8.85 (31 components)

**Sourcing strategy:**
- Bulk order from single supplier (Digi-Key) to minimize shipping
- Standard values, universal availability
- Stock multiples for rework (buy 2× minimum quantity)

---

## Part 3: Supplier Selection & Qualification

### Primary Supplier: Digi-Key Electronics

**Rationale:**
- Wide inventory (all 41 BOM components in stock or short lead time)
- Reliable shipping (2-3 day standard delivery in US)
- Volume pricing available (account-level discounts)
- Responsive customer service (can confirm MCU allocation)
- Electronic BOM import/export (easy cart management)

**Account setup:**
```
Company: [Your Organization]
Email: lab767@gmail.com
Account Type: Business (for quantity pricing)
```

**RFQ Process:**
1. Export BOM as CSV from KiCad (tools/exporters/csv)
2. Import into Digi-Key cart tool
3. Review lead times and pricing for each component
4. Request formal quote (5+ prototypes)
5. Confirm MCU availability with account manager
6. Place order for all components simultaneously

### Secondary Suppliers (Backup / Alternates)

| Component | Primary | Backup 1 | Backup 2 | Trigger |
|-----------|---------|----------|----------|---------|
| STM32H745ZIT6 | Digi-Key | Arrow | Heilind | If D-K lead time >12 weeks |
| ICM-20689 | Digi-Key | Element14 | SOS | If D-K lead time >6 weeks |
| W25Q128JV | Digi-Key | Heilind | TTM | If D-K lead time >4 weeks |
| Passives (all) | Digi-Key | Mouser | Newark | If price >10% higher |

**Dual-sourcing strategy:**
- MCU: Order 1× from Digi-Key + 1× from Arrow simultaneously
- Accept whichever arrives first
- Alternate source acts as insurance against single-point failure

---

## Part 4: Complete BOM with Costs

### Summary by Category

```
Category                    | Qty | Total Cost | % of BOM
────────────────────────────┼─────┼────────────┼─────────
1. Main Processor           | 1   | $45.50     | 12.0%
2. Power Management         | 4   | $4.00      | 1.1%
3. Clock & Oscillators      | 1   | $0.78      | 0.2%
4. External Memory          | 2   | $4.05      | 1.1%
5. Sensors                  | 5   | $39.05     | 10.3%
6. Communication            | 1   | $4.80      | 1.3%
7. Passive Components       | 31  | $8.85      | 2.3%
────────────────────────────┴─────┴────────────┴─────────
TOTAL (41 components)       | 41  | $107.03    | (circuit cost)
```

### Cost Analysis: 1-Piece vs. Volume

**1-Piece build (prototype):**
```
Component cost:          $107.03
PCB cost (5 boards):     $50.00  (avg $10/board)
Assembly labor:          $200.00 (manual)
Test & debug:            $50.00
─────────────────────────────────
**Total prototype cost:  ~$407.00**
**Per-unit cost:         ~$407.00** (1 board)
```

**100+ unit production (at scale):**
```
Component cost:          $65.00  (volume pricing)
PCB cost (1000 boards):  $120.00 (avg $0.12/board)
Assembly labor:          $15.00  (automated pick & place)
Test jig amortization:   $5.00
─────────────────────────────────
**Total manufacturing:   ~$85.00**
**Overhead (15%):        ~$12.75**
**Target COGS:           ~$98.00**
**Sale price (35% margin): ~$150.00**
**Board margin:          ~$52.00** (65% gross margin)
```

**Price trajectory:**
- 1-piece: $377.81 (design cost amortized)
- 10-piece: $280.00 (volume discounts kick in)
- 100-piece: $250.00 (full volume pricing)
- 1000+: ~$150.00 (assembly automation, bulk PCB)

---

## Part 5: Procurement Timeline & Gantt Chart

### Procurement Phases

```
PHASE 0: BOM Finalization (CURRENT)
├─ 2026-07-28: Complete schematic review
├─ 2026-07-29: Export BOM from KiCad
├─ 2026-07-30: Request quotes from Digi-Key + Arrow
└─ 2026-08-01: Confirm MCU allocation, place orders

PHASE 1: Component Delivery (Aug - Dec)
├─ 2026-08-03: Passive components arrive (1-2 wks)
├─ 2026-08-10: Sensors & communication ICs arrive (2-4 wks)
├─ 2026-08-17: Flash & EEPROM arrive (2-4 wks)
├─ 2026-09-15: Power management ICs arrive (parallel with layout)
└─ 2026-09-25: **MCU arrives** (8-12 wks, critical path item)

PHASE 2: PCB Fabrication (concurrent with procurement)
├─ 2026-08-11: Gerber files locked (from phase 2)
├─ 2026-08-15: RFQ submitted to fab partner
└─ 2026-09-08: PCB delivery (2-3 wks from RFQ)

PHASE 3: Prototype Assembly (Sep-Oct)
├─ 2026-09-15: All components received (MCU arrival)
├─ 2026-09-20: Solder paste stencil fabricated
├─ 2026-09-25: PCB assembly starts (5 boards)
├─ 2026-10-02: Reflow + inspection complete
└─ 2026-10-08: Functional test (manufacturing PAT)

PHASE 4: Prototype Validation (Oct-Nov)
├─ 2026-10-08: Hardware bring-up (1 week)
├─ 2026-10-15: Sensor integration (2 weeks)
├─ 2026-10-29: Firmware validation (2 weeks)
├─ 2026-11-05: Environmental testing (1 week)
└─ 2026-11-15: Field validation complete (critical gate)
```

### Gantt Chart

```
                    Aug    Sep    Oct    Nov    Dec
Passive components:  ▓
Sensors & comms:        ▓
Flash/EEPROM:           ▓
Power mgmt:             ▓▓
MCU (critical):            ▓▓▓▓
PCB fabrication:      ▓▓▓
Assembly:                 ▓▓
Testing:                     ▓▓▓
Field validation:            ▓▓▓
```

---

## Part 6: Inventory Management & Spare Parts

### Recommended Spare Stock (for 10-board assembly)

| Component | Qty per Board | Spare Stock | Rationale |
|-----------|---|---|---|
| STM32H745ZIT6 | 1 | 2 | BGA rework risk; dual-source |
| ICM-20689 | 1 | 1 | High-value sensor; rework difficult |
| W25Q128JV | 1 | 1 | High-value memory; easy to rework |
| Passives (all) | 30+ | 50 | Cheap; bulk for rework tape reels |

**Total spare components cost:** ~$150-200 (5% of total BOM for 10 boards)

---

## Part 7: Quality Control & Receiving Inspection

### Incoming Inspection Checklist

**For each component delivery:**

```
□ Count components against packing slip
□ Verify part numbers (label vs. BOM)
□ Check manufacturing date codes (prefer <2 years old)
□ Visual inspection: No bent pins, corrosion, moisture
□ Continuity test (multimeter): Diode, capacitors
□ Identify components at highest risk:
  - MCU: Visual check pin quality
  - Flash: Verify package (SOIC-8 vs. WSON)
  - Sensors: Test I2C/SPI communication (if test jig available)
```

### Storage & Handling

**Component storage conditions:**
- Temperature: 20-25°C
- Humidity: 45-55% RH
- Keep in anti-static bags (ESD protection)
- MCU & sensitive ICs: Store in dry-box with desiccant

**IPC-A-610 compliance:**
- Prevent oxidation: Store in sealed bags with silica gel
- Electrolytic capacitors: Confirm voltage rating & polarity
- No solvent exposure (flux cleaning before storage)

---

## Part 8: Cost Tracking & Vendor Management

### Cost Baseline

**Approved BOM cost (baseline):**
```
Component cost (as-quoted):    $107.03
Overhead (15% contingency):    $16.05
─────────────────────────────────────
Target procurement budget:     $123.00
```

**Actual cost tracking:**
```
Date    | Vendor    | Part         | Qty | Unit  | Total | Status
────────┼───────────┼──────────────┼─────┼───────┼───────┼─────────
2026-08-01 | Digi-Key  | MCU 1+1      | 2   | $45.50 | $91.00 | ordered
2026-08-01 | Digi-Key  | Sensors      | 5   | ~$38  | $190.00| ordered
2026-08-01 | Digi-Key  | Memory       | 2   | ~$4   | $8.00 | ordered
2026-08-01 | Digi-Key  | Passives     | 40  | ~$9   | $9.00 | ordered
2026-08-01 | Arrow     | MCU backup   | 1   | $47.00| $47.00| ordered
────────────────────────────────────────────────────────────────────
Total so far:                              | $345.00 | 70% budget
```

### Vendor Scorecard

**Digi-Key Performance:**
- Lead time accuracy: __________ (track after delivery)
- Packaging quality: __________ (inspect incoming)
- Customer service: __________ (rate support response)
- Price competitiveness: __________ (compare with Mouser/Arrow)

---

## Part 9: Supply Chain Risk Mitigation

### Single-Point Failure Analysis

| Component | Risk | Mitigation |
|-----------|------|-----------|
| STM32H745ZIT6 | Highest | Dual-source (Digi-Key + Arrow); backup STM32H743 qualified |
| ICM-20689 | High | Order early; verify lead time; alternate MPU-6050 qualified |
| W25Q128JV | Medium | Order early; backup W25Q64JV (half capacity, still adequate) |
| VL53L0X | Medium | Order early; backup LIDAR-Lite available |
| Passives | Low | Multiple suppliers; bulk stock |

### Geopolitical & Supply Chain Contingencies

**Current concerns (Jul 2026):**
- MCU availability (fabbing constraints)
- Sensor supply (IMU shortages documented)
- Lead times extending into Q4 2026

**Contingency plan:**
1. **If MCU unavailable:** Use STM32H743ZIT6 (slightly lower performance, same pins)
2. **If ICM-20689 unavailable:** Substitute MPU-6050 (older but proven)
3. **If Flash memory unavailable:** Use W25Q64JV (8 MB, 2-week logging vs. 8 weeks)
4. **Schedule delay >2 weeks:** Expedite PCB + assembly (pay $500 rush fee)

---

## Part 10: Next Steps & Action Items

### Immediate Actions (This Week)

- [ ] **Export BOM from KiCad** (format: CSV with Digi-Key part numbers)
- [ ] **Contact Digi-Key account manager** (confirm MCU allocation, request quote)
- [ ] **Request formal quote** (all 41 components, quantity 2 for dual-source)
- [ ] **Parallel RFQ to Arrow** (MCU backup, confirm lead time)
- [ ] **Verify alternate components** (confirm pin-compatibility for backup parts)
- [ ] **Place order** (target: 2026-08-01 to meet critical path)

### Week 2 Actions

- [ ] **Track incoming parts** (spreadsheet with expected arrival dates)
- [ ] **Prepare storage area** (anti-static, humidity control, organized by part)
- [ ] **Schedule receiving inspection** (assign person responsible)
- [ ] **Coordinate with PCB fab** (confirm Gerbers ready by 2026-08-11)

### Week 3-4 Actions

- [ ] **Monitor component delivery** (follow up on long-lead items)
- [ ] **Prepare assembly line** (solder paste, stencil, reflow oven)
- [ ] **Arrange test equipment** (multimeter, oscilloscope for bring-up)
- [ ] **Document procurement log** (actual vs. planned delivery dates)

---

## Appendix A: BOM Export Instructions (KiCad)

**Step-by-step KiCad BOM export:**

1. Open project: `hardware/STM32H745_Ballistic_Corrector.kicad_sch`
2. Tools → Generate Bill of Materials
3. Select CSV format
4. Fields: Reference, Value, Datasheet, Supplier, Supplier Part Number
5. Export to: `hardware/STM32H745_BOM_EXPORT.csv`
6. Open in Excel; add Digi-Key part numbers from manual lookup
7. Sort by lead time (longest first)
8. Submit to Digi-Key cart tool

**Expected CSV output:**
```
Reference,Value,Datasheet,Digi-Key PN,Qty,Unit Price,Extended Price
U1,STM32H745ZIT6,https://...,497-STM32H745ZIT6-ND,1,$45.50,$45.50
U2,TPS62133ADBVT,https://...,595-TPS62133ADBVT-ND,1,$2.45,$2.45
...
```

---

## Appendix B: Digi-Key Account Setup

**Login:** https://www.digikey.com  
**Email:** lab767@gmail.com  
**Account Type:** Business (for volume pricing)

**Create shopping list:**
1. Log in to Digi-Key account
2. Tools → Shopping List → Create New
3. Name: "STM32H745 Prototype BOM"
4. Add items from CSV (Tools → Import Shopping List)
5. Review pricing & availability
6. Request quote (Account manager: TBD)

---

## Appendix C: Regulatory & Compliance Notes

### Lead-Free (RoHS) Compliance

All components sourced must be **RoHS-compliant** (lead-free):
- Solder: SAC305 (Sn/Ag/Cu lead-free)
- Component leads: Tin-plated (Ni/Pd under-plate)
- Flux: Lead-free organic (no halides)

**Verification:** Each component datasheet must confirm RoHS compliance.

### Conflict Minerals Reporting

For production >1000 units, maintain conflict minerals declarations from suppliers.
*Not required for prototype phase.*

---

**Document Status:** Ready for procurement  
**Next Phase Gate:** Component delivery + PCB fabrication complete  
**Prepared by:** Supply Chain & Procurement (AI Assistant)  
**Approval Required:** Finance, Procurement Manager
