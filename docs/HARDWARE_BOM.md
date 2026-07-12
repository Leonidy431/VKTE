# Hardware Bill of Materials — Volumetric Ocean Interface

Concrete, sourceable hardware for the three concepts selected in
`docs/TOP3_CONCEPTS_48_PARAMETERS.md`. Split into a **shared core platform**
(laser + bubble rig, common to all three) and **per-concept add-ons**
(sensors/sonar/compute specific to each application).

Prices are indicative street/list prices in USD, single-unit quantity,
as of early 2025 — expect 20–40% discount at 5+ unit volume.

---

## 1. Shared core platform

### 1.1 Laser subsystem

| Component | Model | Spec | Qty | Est. price |
|---|---|---|---|---|
| Green DPSS laser module | CNI Laser MGL-III-532, 8 W | 532 nm, CW, TTL/analog modulation | 1 | $6,500 |
| Blue DPSS laser module (seawater-optimized variant) | CNI Laser MBL-III-473, 3 W | 473 nm, CW | 1 (optional) | $4,200 |
| XY galvanometer scanner pair | Cambridge Technology 6215H + servo driver | ±20° optical, <500 µs step response | 1 set | $3,800 |
| Scan control board | Beam Dynamics / SCANahead LMC controller | XY2-100 protocol, USB/Ethernet host | 1 | $1,200 |
| Laser safety interlock + enclosure | Custom Class-4 enclosure, interlocked door switch | IEC 60825-1 compliant | 1 | $900 |

**Laser subsystem subtotal: ≈ $16,600** (with blue module) / **≈ $12,400** (green only)

### 1.2 Bubble generation subsystem

| Component | Model | Spec | Qty | Est. price |
|---|---|---|---|---|
| Submersible Langevin transducer | Steminc SMBLTD45F40H | 40 kHz, bolt-clamped, 60 W | 4 | $1,000 |
| Ultrasonic driver/amplifier | Steminc SMDRV1000-1 or equivalent 40 kHz driver board | 0–100% duty cycle control, 0–10V input | 4 | $1,600 |
| Function generator (frequency sweep, calibration) | Siglent SDG1032X | 20 kHz–80 kHz sweep, dual channel | 1 | $350 |
| Stabilizer dosing pump | Peristaltic dosing pump, 0.01–2 mL/min | Food-safe SDS/saponin surfactant metering | 1 | $180 |

**Bubble subsystem subtotal: ≈ $3,130**

### 1.3 Tank / optical enclosure

| Component | Spec | Qty | Est. price |
|---|---|---|---|
| Acrylic display tank, custom | 40×40×40 cm, 12 mm cast acrylic, anti-scratch coated | 1 | $1,800 |
| Circulation/filtration pump | 500 L/h submersible, seawater-rated | 1 | $220 |
| Salinity/temperature stabilization chiller | 1/10 HP aquarium chiller | 1 | $650 |
| Anti-reflective viewing panel (front face) | AR-coated acrylic insert | 1 | $400 |

**Tank subsystem subtotal: ≈ $3,070**

### 1.4 Compute (shared render/control host)

| Component | Spec | Qty | Est. price |
|---|---|---|---|
| Edge compute module | NVIDIA Jetson AGX Orin 64GB | 275 TOPS, real-time voxel mapping + galvo timing | 1 | $2,000 |
| Host workstation (lab/dev tier) | Ryzen 9 / RTX 4070, 64 GB RAM | Development, ML training, offline rendering | 1 | $2,400 |
| Real-time I/O interface | Ethernet/EtherCAT bridge to galvo + transducer drivers | 1 | $500 |

**Compute subsystem subtotal: ≈ $4,900** (both tiers) / **≈ $2,500** (edge-only deployment)

### Core platform total: **≈ $23,700 – $27,700**

---

## 2. Per-concept add-ons

### 2.1 Ocean Quality Volumetric Cube (environmental monitoring)

| Component | Model | Spec | Qty | Est. price |
|---|---|---|---|---|
| Multiparameter water-quality sonde | YSI ProDSS | Temp, salinity, DO, pH, turbidity, depth | 1 | $8,500 |
| PFAS/heavy-metal spot sensor (optional) | In-Situ Aqua TROLL 600 + ISE probes | Extended chemistry package | 1 | $6,000 |
| Data logger / telemetry modem | RBR Coda logger or cellular IoT gateway | Field deployment, remote sites | 1 | $1,200 |
| Weatherproof enclosure (dockside unit) | NEMA 4X, IP66 | 1 | $700 |

**Add-on subtotal: ≈ $16,400** (with optional PFAS package) / **≈ $10,400** (core sonde only)

**Concept total: ≈ $34,100 – $44,100**

---

### 2.2 ROV Volumetric Path Planner (subsea robotics HMI)

Reuses the BlueOS/MAVLink integration already implemented in the klab-backend-platform repo
(klab-backend-platform `app/modules/blueos/`), so no new vehicle-side integration work is
required — only operator-station display hardware.

| Component | Model | Spec | Qty | Est. price |
|---|---|---|---|---|
| Scanning sonar | Blue Robotics Ping360 | 360° mechanical scan, 2–50 m range, already supported via BlueOS `/ping` endpoints | 1 | $2,900 |
| Altimeter/single-beam sonar | Blue Robotics Ping1D (Ping Sonar) | Distance-to-bottom/obstacle | 1 | $700 |
| USBL/positioning reference | Water Linked A50 | Underwater GPS-equivalent, feeds ROV coordinates into voxel mapper | 1 | $9,500 |
| Operator console PC | Ruggedized panel PC, 15" touchscreen | Bridge/topside control station | 1 | $2,200 |

**Add-on subtotal: ≈ $15,300**

**Concept total: ≈ $39,000 – $43,000**

---

### 2.3 3D Underwater Site Reconstructor (museum / archaeology exhibit)

| Component | Model | Spec | Qty | Est. price |
|---|---|---|---|---|
| Photogrammetry capture kit | GoPro Hero 12 (×4) + underwater housing rig | Site photo/video capture for 3D reconstruction | 1 set | $2,800 |
| Photogrammetry software license | Agisoft Metashape Professional | Point cloud / mesh generation from dive footage | 1 | $3,500 |
| Museum-grade tank enclosure | 60×60×60 cm tempered glass, museum finish | Public-facing exhibit unit (larger than core 40³ tank) | 1 | $4,500 |
| Interactive touch panel | 21" capacitive touchscreen, vandal-resistant | Visitor timeline/artifact selector | 1 | $1,400 |
| Multi-visitor audio system | Directional speaker array | Narration synced to reconstruction playback | 1 | $1,100 |

**Add-on subtotal: ≈ $13,300** (excludes core tank, replaced by museum tank)

**Concept total: ≈ $33,700** (core platform laser/bubble/compute + museum tank replacing standard tank + capture/software/exhibit hardware)

---

## 3. Summary comparison

| Concept | Core platform | Add-ons | Total (approx.) | Primary buyer |
|---|---|---|---|---|
| Ocean Quality Volumetric Cube | $23,700–27,700 | $10,400–16,400 | **$34,100–44,100** | Ports, water utilities, research stations |
| ROV Volumetric Path Planner | $23,700–27,700 | $15,300 | **$39,000–43,000** | ROV operators, offshore energy, survey firms |
| 3D Underwater Site Reconstructor | $23,700–27,700* | $13,300 | **$33,700** | Museums, archaeology institutes |

\* Museum tank ($4,500) replaces the standard 40³ cm tank ($1,800) in the core
platform line item; net effect included in the concept total above.

Notes:
- All three concepts share the identical laser + bubble generation core,
  so a single R&D/manufacturing line serves all three go-to-market paths —
  only the sensor/compute/enclosure layer changes per vertical.
- The ROV concept has the lowest integration risk when built on klab-backend-platform
  specifically, since Blue Robotics Ping sonar support already exists in
  klab-backend-platform's `app/modules/blueos/client.py` (`get_ping_devices`, `get_ping_distance`).
