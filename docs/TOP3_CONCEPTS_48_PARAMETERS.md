# Top-3 Concepts — 48-Parameter Comparison

Three concepts selected from the 30+ catalog in
`VOLUMETRIC_DISPLAY_SPECIFICATION.md`, chosen to cover three distinct
go-to-market paths on the same shared laser+bubble core platform
(see `docs/HARDWARE_BOM.md`):

1. **Ocean Quality Volumetric Cube** — environmental/regulatory monitoring
2. **ROV Volumetric Path Planner** — subsea robotics operator HMI (reuses this
   repo's existing BlueOS/Ping sonar integration)
3. **3D Underwater Site Reconstructor** — museum/archaeology exhibit

Each is scored across the same 48-parameter framework, grouped into 8
categories of 6 parameters, so the three are directly comparable.

---

## Parameter framework

| # | Category | Parameter |
|---|---|---|
| 1 | Identity | Name |
| 2 | Identity | Category |
| 3 | Identity | Tagline / positioning |
| 4 | Identity | Problem addressed |
| 5 | Identity | Solution summary |
| 6 | Identity | Unique value proposition |
| 7 | Market | Target market |
| 8 | Market | Primary customer segment |
| 9 | Market | TAM |
| 10 | Market | SAM |
| 11 | Market | SOM (5-year) |
| 12 | Market | Key competitors / alternatives |
| 13 | Market | Competitive advantage |
| 14 | Market | Entry barriers |
| 15 | Laser tech | Laser type |
| 16 | Laser tech | Wavelength |
| 17 | Laser tech | Optical power |
| 18 | Laser tech | Scan frequency |
| 19 | Laser tech | Voxel grid resolution |
| 20 | Laser tech | Frame refresh rate |
| 21 | Medium tech | Medium type |
| 22 | Medium tech | Bubble generation method |
| 23 | Medium tech | Bubble size (resonant) |
| 24 | Medium tech | Bubble density |
| 25 | Medium tech | Stabilizing agent |
| 26 | Medium tech | Voxel/bubble lifetime |
| 27 | Hardware | Tank dimensions |
| 28 | Hardware | Water volume |
| 29 | Hardware | Tank material |
| 30 | Hardware | Primary sensors |
| 31 | Hardware | Compute platform |
| 32 | Hardware | Power consumption |
| 33 | Software | OS / framework |
| 34 | Software | ML/AI components |
| 35 | Software | Control interface |
| 36 | Software | Communication protocols |
| 37 | Software | External integrations |
| 38 | Business | Business model |
| 39 | Business | MVP development cost |
| 40 | Business | Unit sale price |
| 41 | Business | Time to market |
| 42 | Business | Team roles needed |
| 43 | Business | Funding round target |
| 44 | Business | Breakeven horizon |
| 45 | Risk/Reg | Key technical risk |
| 46 | Risk/Reg | Regulatory requirements |
| 47 | Risk/Reg | Environmental considerations |
| 48 | Risk/Reg | IP / patent potential |

---

## Comparison table

| # | Parameter | Ocean Quality Volumetric Cube | ROV Volumetric Path Planner | 3D Underwater Site Reconstructor |
|---|---|---|---|---|
| 1 | Name | Ocean Quality Volumetric Cube | ROV Volumetric Path Planner | 3D Underwater Site Reconstructor |
| 2 | Category | Environmental monitoring | Subsea robotics HMI | Museum / cultural exhibit |
| 3 | Tagline | "See water quality, not just numbers" | "Navigate by volume, not by chart" | "Raise the wreck, virtually" |
| 4 | Problem | Water-quality data lives in spreadsheets/graphs — invisible and hard to act on quickly | ROV operators fly blind through sonar text overlays and 2D chart-plotter views | Shipwrecks/sunken sites are inaccessible to the public and hard to fundraise around |
| 5 | Solution | Real-time sensor data rendered as colored voxel layers in a physical water column | Sonar/USBL feed rendered as a live 3D voxel map an operator can see in the tank next to their console | Photogrammetry/sonar site scans rendered as a rotating 3D voxel reconstruction in water |
| 6 | UVP | Only volumetric (non-screen) real-time water-quality display on the market | Spatial situational awareness without VR headset or screen fatigue | Physical, in-the-room 3D presence of an inaccessible site — no VR headset needed |
| 7 | Target market | Ports, water utilities, coastal regulators | Offshore energy, ROV survey/inspection firms | Museums, archaeology institutes, science centers |
| 8 | Primary customer | Harbor master / environmental agency | ROV operations manager | Museum exhibits director |
| 9 | TAM | ≈ $1.2B (global water-quality monitoring market) | ≈ $2.0B (subsea inspection/ROV services market) | ≈ $0.6B (museum exhibit technology market) |
| 10 | SAM | ≈ $150M (volumetric add-on to monitoring stations) | ≈ $300M (HMI/display upgrades for ROV fleets) | ≈ $80M (large-format interactive exhibits) |
| 11 | SOM (5-yr) | ≈ $8–15M | ≈ $10–20M | ≈ $5–10M |
| 12 | Competitors | Traditional dashboards (Aquarius, YSI EXO software), buoy telemetry portals | 2D chart plotters, sonar software (Hypack, QINSy), VR dive-planning tools | Static dioramas, screen-based VR/AR exhibits |
| 13 | Competitive advantage | Only physical volumetric read-out; instantly legible to non-technical inspectors | Directly reuses this platform's existing BlueOS/MAVLink/Ping sonar stack — near-zero vehicle-side integration | No headset required; multiple visitors view simultaneously, unlike VR |
| 14 | Entry barriers | Sensor calibration trust, regulatory data-accuracy requirements | Safety-critical certification for operator-facing displays | Museum procurement cycles are slow (12–24 month sales cycle) |
| 15 | Laser type | CNI MGL-III-532 DPSS, green | CNI MGL-III-532 DPSS, green | CNI MGL-III-532 DPSS, green (dual-color option for time-layer coding) |
| 16 | Wavelength | 532 nm | 532 nm | 532 nm (+ 473 nm optional for depth-layer coding) |
| 17 | Optical power | 8 W | 8 W | 8–11 W (larger museum tank) |
| 18 | Scan frequency | 20 kHz | 20 kHz | 20 kHz |
| 19 | Voxel grid resolution | 100×100×100 | 100×100×100 | 150×150×150 (larger tank, finer detail for artifacts) |
| 20 | Frame refresh rate | 5 Hz (sensor-paced) | 10–15 Hz (needs to track moving ROV) | 5 Hz (mostly static/slow-rotating reconstructions) |
| 21 | Medium type | Seawater (or calibration freshwater) | Freshwater (topside operator station, not seawater) | Seawater (thematic authenticity for exhibits) |
| 22 | Bubble generation | Acoustic (40 kHz Langevin transducers) | Acoustic (40 kHz) | Acoustic (40 kHz) |
| 23 | Bubble size (resonant) | ≈ 82 µm | ≈ 82 µm | ≈ 82 µm |
| 24 | Bubble density | 1e6 /cm³ | 1e6–1e7 /cm³ (denser for crisper moving path lines) | 1e6 /cm³ |
| 25 | Stabilizer | Food-safe SDS, 0.05% | SDS, 0.05% (non-public-facing, less critical) | Saponin-based natural surfactant, 0.05% (public-facing/child-safe) |
| 26 | Voxel/bubble lifetime | 100–500 ms | 100–300 ms (faster refresh needs shorter-lived, more frequently renewed bubbles) | 200–500 ms |
| 27 | Tank dimensions | 40×40×40 cm | 40×40×40 cm | 60×60×60 cm |
| 28 | Water volume | 64 L | 64 L | 216 L |
| 29 | Tank material | 12mm cast acrylic | 12mm cast acrylic | Tempered glass, museum finish |
| 30 | Primary sensors | YSI ProDSS multiparameter sonde | Blue Robotics Ping360 + Ping1D + Water Linked A50 USBL | Photogrammetry rig (4× GoPro) + optional multibeam sonar data import |
| 31 | Compute platform | Jetson AGX Orin (dockside, low power) | Jetson AGX Orin + ruggedized operator console PC | Workstation (RTX 4070) for Metashape processing + Jetson for live render |
| 32 | Power consumption | ≈ 350 W (laser+transducers+chiller+edge compute) | ≈ 400 W (adds console PC) | ≈ 450 W (larger tank pump/chiller + workstation) |
| 33 | OS / framework | Linux + FastAPI (klab-backend-platform's `backend/app`) | Linux + FastAPI + ROS2 bridge to BlueOS/MAVLink | Linux + FastAPI, offline pipeline in Agisoft Metashape |
| 34 | ML/AI components | Anomaly detection on sensor time-series (hypoxia/pollution spikes) | SLAM-assisted path smoothing, obstacle classification from sonar | Photogrammetry mesh generation, artifact object detection/tagging |
| 35 | Control interface | Web dashboard-free control panel (see `frontend/volumetric-display/`) for calibration only — public view is the tank itself | Operator console: path overlay + laser/bubble parameter panel | Museum touch panel: timeline scrubber + artifact picker |
| 36 | Communication protocols | Modbus/RS-485 (sonde) → REST/WebSocket | MAVLink (existing `pymavlink` stack) → REST/WebSocket | HTTP file import (photogrammetry export) → REST/WebSocket |
| 37 | External integrations | Regulatory reporting APIs (e.g., water utility SCADA) | Existing BlueOS extension ecosystem in klab-backend-platform | Museum ticketing/exhibit CMS |
| 38 | Business model | Hardware sale + annual calibration/support contract | Hardware sale + per-seat operator console license | Hardware sale (one-time) + reconstruction-as-a-service (per-site fee) |
| 39 | MVP dev cost | ≈ $80K (core platform + sonde integration) | ≈ $70K (core platform + reuse of existing BlueOS code) | ≈ $95K (core platform + museum tank + capture pipeline) |
| 40 | Unit sale price | $50K–100K | $60K–90K | $100K–250K (larger exhibit-grade install) |
| 41 | Time to market | 9–12 months | 6–9 months (less new integration work) | 12–18 months (custom content pipeline per site) |
| 42 | Team roles needed | Optics eng., embedded/firmware eng., backend eng., hydrology consultant | Optics eng., robotics/ROS eng., backend eng. (reuses existing BlueOS team) | Optics eng., backend eng., 3D/photogrammetry artist, museum exhibit designer |
| 43 | Funding round target | Seed $1.5–2.5M | Seed $1.5–2.5M | Seed $1–2M |
| 44 | Breakeven horizon | 18–24 months (2–3 units/year) | 15–20 months (existing ROV customer base to sell into) | 24–30 months (long museum sales cycles) |
| 45 | Key technical risk | Sensor drift/fouling in field deployment corrupting color-coded readout | Real-time voxel refresh must keep pace with fast-moving ROV without lag | Photogrammetry mesh-to-voxel conversion quality depends heavily on source dive footage |
| 46 | Regulatory requirements | Environmental data-reporting standards (ISO 7888, local water authority rules) | Maritime electrical/safety certification for topside equipment (IEC 60945) | Public exhibit safety (Class-1 accessible laser enclosure, ADA access) |
| 47 | Environmental considerations | Stabilizer surfactant discharge if using open dockside water — closed-loop tank avoids this | Freshwater closed system, minimal environmental exposure | Public aquarium water treatment/discharge rules if not closed-loop |
| 48 | IP / patent potential | Method for sensor-driven color-gradient voxel mapping in situ | Method for real-time sonar-to-voxel path rendering synced to vehicle telemetry | Method for photogrammetry-mesh-to-acoustic-voxel rasterization pipeline |

---

## Reading the comparison

- **Fastest to ship**: ROV Volumetric Path Planner — it inherits this
  klab-backend-platform's existing `app/modules/blueos/` MAVLink and Ping-sonar
  integration almost unchanged, so the volumetric layer is the only new
  work.
- **Largest addressable market**: ROV Volumetric Path Planner (TAM ≈ $2.0B),
  reflecting the size of the offshore inspection/ROV services industry.
- **Highest unit price / longest sales cycle**: 3D Underwater Site
  Reconstructor — museum procurement is slow but individual contracts are
  large and the same physical rig doubles as a marketing centerpiece for
  the other two products.
- **Lowest regulatory friction**: Ocean Quality Volumetric Cube in a
  closed-loop calibration setting; goes up sharply if deployed dockside
  with open water exchange (§47).
