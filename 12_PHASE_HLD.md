# 12-Phase High-Level Design (HLD)
## Volumetric Laser-Bubble Ocean Display System

**Project Overview:** A real-time 3D volumetric display driven by acoustic cavitation in seawater, controlled via laser scanning (532 nm green) with integrated oceanographic sensor feedback. The system renders ocean water-column metrics (temperature, salinity, dissolved oxygen, pH, turbidity) as animated voxels within a transparent tank.

**Decision Framework:** All technical decisions follow the **48-Parameter Consensus Protocol** (see `claude.md`). When algorithm selection, technology choice, or design trade-off is ambiguous, the decision is made by parametric evaluation across 8 categories (6 parameters each) with reference to domain expert consensus from 32 specialists per problem domain.

---

## Phase 1: Requirements & Architecture Review (Foundation)

**Objective:** Validate volumetric display requirements against real-world ocean deployment constraints.

**Deliverables:**
- Physics envelope document (depth limits, pressure ratings, optical absorption models)
- Sensor drift and noise budget (±0.3 ppm for DO, ±0.1°C for temp)
- Latency budget: target <200 ms end-to-end (sensor→voxel render→display)
- Safety & environmental compliance checklist (CITES, marine biota protection, electrical codes)

**Scientific Literature Mapping:**
- Acoustic cavitation in seawater: Brennen, C. E. (2013). *Cavitation and Bubble Dynamics*. Oxford University Press.
  - Stabilizes understanding of stable vs. inertial regimes
  - Surface tension & nucleation site dependencies
- Optical propagation in seawater: Smith & Baker (1981), JOSA 71(11), applies to 450–490 nm window validation
- Sensor drift characterization: YSI ProDSS technical specs on long-term sensor calibration intervals

**Phase Outcomes:**
- [ ] Physics envelope validated against 3 real oceanographic datasets (coastal, open ocean, estuary)
- [ ] Sensor noise floor characterized: <0.5 ppm std dev for DO under dynamic flow
- [ ] Optical absorption model verified: 450 nm transmission >95% in test tank
- [ ] Safety sign-off from marine lab director

**Owner:** Systems Engineer + Marine Physicist

---

## Phase 2: Scientific Literature Deep-Dive (Domain Grounding)

**Objective:** Build a curated, annotated scientific knowledge base for bubble physics, laser scanning, and volumetric rendering in opaque media.

**Deliverables:**
- Annotated bibliography: 50+ peer-reviewed papers on acoustic cavitation, bubble dynamics, volumetric displays, seawater optics
- Parameter extraction tables: map published bubble size/frequency/density relationships to our design space
- Cross-reference matrix: link lab experiments (Phase 1) to theoretical predictions in literature
- Decision basis document: for each Phase 3–12 algorithm, cite supporting research

**Key Literature Sources:**
1. **Acoustic Cavitation & Bubble Generation:**
   - Gallego-Juárez et al. (2010). "Power ultrasonic transducers with radiating plate specially designed..." Ultrasonics 50(2)
   - Leighton, T. G. (1994). *The Acoustic Bubble*. Academic Press
   - Yasui, K. et al. (2008). Microscale bubble oscillations and cavitation dynamics. Nature Physics

2. **Volumetric Display Technology:**
   - Favalora, G. E. et al. (2002). "100-million-voxel volumetric display." SPIE Proc. 4733
   - Willemsen et al. (2014). "Display of digital terrain models with volumetric techniques."

3. **Seawater Optics & Laser Propagation:**
   - Mobley, C. D. (1994). *Light and Water*. Academic Press
   - Smith & Baker (1981). "Optical properties of the clearest natural waters (200–800 nm)." Applied Optics 20(2)

4. **Real-time 3D Rendering & WebGL:**
   - Three.js Documentation & Examples
   - Akenine-Möller et al. (2018). *Real-Time Rendering* (4th ed.). CRC Press

**Phase Outcomes:**
- [ ] Annotated bibliography committed to `docs/SCIENTIFIC_REFERENCES.md`
- [ ] Parameter extraction spreadsheet with bubble size vs. frequency (4 mediums × 20 frequencies)
- [ ] Literature-to-design traceability matrix (every algorithm choice has 2+ supporting papers)
- [ ] Decision basis statements for 5 critical algorithm selections ready for Phase 3

**Owner:** Research Lead + Domain Expert Panel

---

## Phase 3: Physics & Algorithm Core (Acoustic Cavitation, Minnaert, Duty Cycle)

**Objective:** Implement and validate core bubble physics: Minnaert resonance frequency, salinity-corrected duty cycle, bubble lifetime prediction, and coalescence prevention.

**Deliverables:**
- `bubble_generator.py` module with `resonant_bubble_radius_microns()` (Minnaert formula)
- Duty-cycle algorithm with salinity correction factor: duty = f(target_density, acoustic_freq, salinity, stabilizer_conc)
- Bubble lifetime model: predict voxel persistence time (100–500 ms for 50 µm bubbles)
- Coalescence prevention formula: stabilizer concentration as function of bubble density and medium
- Unit tests with 8+ scenarios (seawater vs freshwater, density sweep, frequency sweep)

**Algorithm Decisions (48-Parameter Framework):**
1. **Minnaert vs. Blake Threshold**: Compare resonant frequency prediction accuracy across 5–80 kHz.
   - Decision: Minnaert (simpler, validated for stable cavitation regime)
   - Reference: Leighton *et al.* on regime boundaries
2. **Salinity Duty-Cycle Correction**: 
   - Decision: Inverse proportionality to (salinity / REFERENCE_SALINITY); factor ~0.97 for 35 PSU
   - Reference: Surface tension data (seawater 0.0679 vs freshwater 0.0728 N/m)
3. **Stabilizer Selection**: 
   - Decision: Sodium Dodecyl Sulfate (SDS) vs Saponin vs Tween-80
   - Criteria: toxicity, cost, bubble persistence, microbial interaction
   - Winner: SDS (0.5–2% by mass) balances cost, persistence, safety

**Phase Outcomes:**
- [ ] Minnaert formula validated against experimental bubble sizing (ultrasonic camera imagery)
- [ ] Duty-cycle model tested in 4 media (air, freshwater, seawater, gel) across 3 stabilizer concentrations
- [ ] Bubble lifetime model error <15% vs measured rise velocity (Stokes drag)
- [ ] Coalescence prevention algorithm prevents >90% coalescence up to 1e7 bubbles/cm³
- [ ] All unit tests passing; 100% code coverage for core physics

**Owner:** Fluid Dynamics Specialist + Acoustics Engineer

---

## Phase 4: Laser Control & Galvanometric Scanning (Hardware Integration)

**Objective:** Implement laser control interface and galvo deflection commands to scan voxels across 3D space.

**Deliverables:**
- `laser_control.py` class: connect, configure, scan_voxels(), wavelength validation
- Galvo driver communication protocol (USB/serial to Cambridge or Thorlabs scanner)
- Wavelength validation: 450–490 nm for seawater, 532 nm (green) default
- Power ramp safety: slow ramp-up (100 ms per watt) to prevent beam flicker
- Voxel scan timing: target 60 voxels/sec at 8 W, 1 ms dwell per voxel
- Thermal model: laser duty cycle vs. heat sink temperature (target <80°C sustained)

**Algorithm Decisions (48-Parameter Framework):**
1. **Scan Pattern**: Raster vs. Z-Order curve vs. proximity-based ordering
   - Decision: Z-Order (Morton) curve for cache efficiency and reduced galvo settling time
   - Criteria: minimize total galvo travel distance, reduce settling time jitter
   - Reference: Graphics literature on spatial locality

2. **Power Control**: Fixed vs. PWM vs. Analog modulation
   - Decision: Analog modulation (proportional power supply) for smooth intensity gradients
   - Criteria: voxel color fidelity, no aliasing artifacts

3. **Dwell Time Optimization**:
   - Decision: Adaptive dwell based on bubble density: longer dwell → brighter voxel (pump more photons)
   - Reference: Favalora et al. on brightness budgets in volumetric displays

**Phase Outcomes:**
- [ ] Laser safety interlocks confirmed (beam cut-off on lose of galvo sync)
- [ ] Galvo calibration completed: ±0.5 mm positional accuracy across 100×100 mm scan field
- [ ] Thermal stress test: 8-hour sustained operation, thermal limit not exceeded
- [ ] Wavelength validation: 532 nm confirmed seawater-optimal per Smith & Baker
- [ ] Voxel brightness uniformity: ±10% across entire scan volume

**Owner:** Optics Engineer + Controls Specialist

---

## Phase 5: Bubble Generation & Stabilization (Acoustic Transducer System)

**Objective:** Deploy and characterize piezo transducer array for consistent bubble nucleation across full tank volume.

**Deliverables:**
- Transducer array topology: 4-element phased array (40 kHz center frequency)
- Excitation waveform design: sine + chirp envelope for continuous bubble stream
- Bubble size distribution measurement: optical particle sizing (5–500 µm range)
- Stabilizer dosing system: automated metering pump (SDS solution, 0.5–2% by mass)
- Real-time density feedback: sonar attenuation proxy for bubble count estimation
- Seawater adaptation protocol: calibration sequence for salinity-dependent nucleation

**Algorithm Decisions (48-Parameter Framework):**
1. **Transducer Frequency**: 20 kHz vs. 40 kHz vs. 80 kHz
   - Decision: 40 kHz (Minnaert optimum for 82 µm bubbles; good seawater penetration; commercial availability)
   - Trade-off: lower freq → larger bubbles (better scatter), higher freq → smaller bubbles (better voxel resolution)
   - Winner: 40 kHz balances both criteria

2. **Stabilizer Dosing Strategy**: Open-loop (fixed) vs. closed-loop (sonar-feedback) vs. adaptive
   - Decision: Closed-loop sonar feedback (acoustic attenuation → density estimate → stabilizer adjustment)
   - Criteria: maintain target density (1e6–1e7 /cm³) despite transducer drift, temperature changes
   - Reference: Bloom et al. (2011) on sediment acoustic backscatter for concentration estimation

3. **Bubble Injection Point**: Single bottom port vs. distributed array vs. rising column
   - Decision: Distributed array (4-element, 90° spacing) for uniform field
   - Criteria: eliminate density dead-zones, reduce convective stirring during operator experiments

**Phase Outcomes:**
- [ ] Transducer characterization complete: frequency response ±3 dB, 20–80 kHz
- [ ] Bubble size distribution: median 82 µm, σ <25 µm (geometric std dev <1.3)
- [ ] Stabilizer dosing system tested: accuracy ±0.05% by mass, response time <2 sec
- [ ] Sonar density feedback validated: error <10% vs. optical particle counter
- [ ] Seawater salinity calibration: verified for 0–40 PSU range

**Owner:** Acoustics Engineer + Fluid Mechanics Specialist

---

## Phase 6: Sensor Integration & Calibration (Water Column Telemetry)

**Objective:** Integrate multiparameter water-quality sensor and establish real-time data pipeline.

**Deliverables:**
- `sensor_reader.py` class: connect to YSI ProDSS probe, poll at 5 Hz, cache latest reading
- Modbus protocol handler: serial communication, error detection, timeout management
- Sensor drift characterization: establish calibration interval (24 hours vs. weekly vs. monthly)
- Pre-processing pipeline: outlier rejection (3σ filter), moving average (1–10 sec window)
- Sensor placement optimization: depth profile vs. single-point vs. thermocline tracking
- Optical DO sensor stability: assess sensor fouling rate in continuous seawater flow

**Algorithm Decisions (48-Parameter Framework):**
1. **Outlier Rejection**: Simple 3σ threshold vs. isolation forest vs. Kalman filter
   - Decision: 3σ threshold with moving median backup (simpler, deterministic)
   - Criteria: reject sensor spikes without lag, maintain real-time responsiveness
   - Edge case: handle rapid salinity changes (e.g., freshwater input pulse)

2. **Sensor Fusion**: Single probe vs. redundant sensors vs. ensemble voting
   - Decision: Single YSI ProDSS (cost, space) with redundant temp sensor for cross-validation
   - Criteria: ocean deployment constraints (limited payload), cost (<$20k)

3. **Calibration Interval**: Fixed vs. adaptive vs. continuous self-calibration
   - Decision: Adaptive (observe drift rate; prompt recalibration when error >2%)
   - Criteria: minimize manual intervention in field deployments

**Phase Outcomes:**
- [ ] YSI ProDSS probe physically mounted and tested in saltwater tank
- [ ] Modbus protocol validated: 100 consecutive reads with 0 errors, <50 ms latency
- [ ] Sensor drift model established: quantify drift rate (ppm/day per parameter)
- [ ] Outlier filter tested: reject 95% of artificial spikes (±5 ppm DO injection), <0.5% false positive
- [ ] Sensor location optimization: depth vs. horizontal position trade-off study completed

**Owner:** Environmental Engineer + Systems Integration Lead

---

## Phase 7: Voxel Mapping & Color Gradients (Data-to-Visual Translation)

**Objective:** Design voxel grid resolution, implement color gradients (DO, temp, turbidity), and validate visual fidelity.

**Deliverables:**
- `voxel_mapper.py` class: frame_from_water_column(), frame_from_point_cloud()
- Grid resolution selection: 50×50×50 vs. 100×100×100 vs. 200×200×200 (trade-off: voxel density vs. refresh rate)
- Color gradient functions:
  - DO (dissolved oxygen): red (hypoxic, <1 ppm) → yellow (borderline, 1–5 ppm) → green (normal, >5 ppm)
  - Temperature: blue (cold, <10°C) → green (temperate, 10–20°C) → red (warm, >20°C)
  - Turbidity: clear → amber → opaque (0–20 NTU range)
- Voxel persistence: frame lifetime 100–500 ms (based on bubble rise kinematics)
- Brightness mapping: sensor value → voxel alpha (transparency gradient)

**Algorithm Decisions (48-Parameter Framework):**
1. **Grid Resolution**: 50³ vs. 100³ vs. 200³ vs. adaptive
   - Decision: 100³ default; adaptive to available laser bandwidth
   - Criteria: 100 ms refresh target at 60 voxels/sec → ~600 voxels/frame → feasible
   - Trade-off: 100³ gives ~1 cm resolution in 100 cm tank; sufficient for ocean stratification features

2. **Color Model**: Linear RGB vs. HSV vs. Perceptually-uniform (Lab) color space
   - Decision: Linear RGB with gamma correction for display matching
   - Criteria: fast computation, intuitive (blue, green, red), matches operator intuition
   - Reference: Favalora et al. on perceptual fidelity in volumetric displays

3. **Gradient Interpolation**: Piecewise linear vs. spline vs. lookup table
   - Decision: Piecewise linear with 3 color stops (simplicity; fast)
   - Criteria: avoid aliasing, minimize GPU memory on WebGL frontend

**Phase Outcomes:**
- [ ] Grid resolution performance tested: 100³ voxel frame renders in <50 ms on target hardware
- [ ] Color gradient functions validated against YSI sensor output (live test tank data)
- [ ] Voxel persistence timing verified: bubbles stay within same voxel for 100–200 ms (measured)
- [ ] Color perceptual validation: 5 human observers confirm intuitive mapping (red=bad, green=good)
- [ ] Brightness uniformity: voxel intensity ±5% across grid (no bright corner artifacts)

**Owner:** Visualization Specialist + Domain Expert

---

## Phase 8: Backend Service Architecture (Async Real-Time Pipeline)

**Objective:** Build FastAPI service orchestrating laser, bubbles, sensors, and voxel rendering with async WebSocket broadcast.

**Deliverables:**
- `volumetric_service.py`: lifespan manager, render loop (5 Hz default), client registry
- Dependency injection pattern: laser, bubbles, sensor, mapper instances
- DisplayStatus schema: laser config + bubble config + latest sensor reading + frame ID
- WebSocket broadcast: 5 Hz tick, all connected clients receive status update
- Error handling: sensor timeout fallback, laser disconnection recovery, graceful shutdown
- Monitoring: telemetry on render loop latency, frame drop rate, WebSocket client count

**Algorithm Decisions (48-Parameter Framework):**
1. **Render Loop Frequency**: 2 Hz vs. 5 Hz vs. 10 Hz
   - Decision: 5 Hz (200 ms per frame; matches bubble rise timescale)
   - Criteria: balance latency (sensor → display) vs. hardware load
   - Trade-off: higher freq → more power, lower freq → less responsive feel

2. **Client Broadcast Strategy**: Multicast vs. fan-out unicast vs. pub-sub middleware
   - Decision: Simple asyncio fan-out (fast for <100 clients)
   - Criteria: low complexity; adequate for lab / small deployment
   - Future: upgrade to Redis pub-sub if scale requires (>1000 clients)

3. **Graceful Degradation**: Sensor timeout handling
   - Decision: Cache last valid reading for up to 10 sec; display "STALE" indicator
   - Criteria: prevent false zero readings, maintain operator awareness

**Phase Outcomes:**
- [ ] Render loop latency measured: 50–80 ms (sensor poll → voxel frame → WebSocket frame)
- [ ] Frame drop rate: 0% under nominal load, <1% under fault injection (sensor timeout)
- [ ] WebSocket stress test: 50 simultaneous clients, zero dropped frames
- [ ] Service startup/shutdown: clean lifespan transitions, no resource leaks
- [ ] Telemetry logged and accessible: /metrics endpoint returns render loop stats

**Owner:** Backend Engineer + DevOps Lead

---

## Phase 9: Frontend UI & Real-Time Visualization (Three.js + Control Panel)

**Objective:** Build immersive browser-based 3D preview + control surface (laser/bubble sliders, sensor readout).

**Deliverables:**
- `index.html`: full-bleed canvas + floating control panel (NOT a dashboard)
- `app.js`: Three.js scene with 4000 particle points representing voxel cloud
- Particle animation: bubble rise simulation (upward drift + random horizontal walk)
- Color animation: real-time DO gradient applied to all particles based on sensor reading
- Control inputs: wavelength, power, laser-enable (laser section); medium, frequency, density (bubble section)
- Sensor readout: 5-parameter display (temp, salinity, DO, pH, turbidity)
- WebSocket connection + polling fallback (2-sec retry on disconnect)
- Responsive design: works on desktop, tablet, mobile (touch slider support)

**Algorithm Decisions (48-Parameter Framework):**
1. **Particle Representation**: Geometry buffer points vs. individual meshes vs. instanced rendering
   - Decision: BufferGeometry points with dynamic color updates
   - Criteria: 4000 particles render at 60 FPS; dynamic color update every 200 ms
   - Reference: Three.js performance guides

2. **Input Debouncing**: Immediate vs. 100 ms vs. 250 ms vs. 1 sec
   - Decision: 250 ms debounce on laser/bubble sliders (user gesture batching)
   - Criteria: prevent API flood; balance responsiveness vs. backend load

3. **Fallback Strategy**: WebSocket-only vs. polling-only vs. hybrid
   - Decision: Hybrid (attempt WS; fall back to 2-sec polling on close)
   - Criteria: works in all network conditions (firewall, proxy, etc.)

**Phase Outcomes:**
- [ ] Three.js scene renders 4000 particles at 60 FPS (60 Hz on Nvidia GPU, 30 Hz on integrated)
- [ ] Particle animation smooth: bubble rise velocity matches physical model (0.1–0.3 mm/sec, scaled for visibility)
- [ ] WebSocket performance: latency <100 ms from server broadcast to particle color update
- [ ] UI responsiveness: slider drag feels instantaneous (<50 ms perceived latency)
- [ ] Mobile testing: touch sliders work on iPhone, Android; no layout breakage below 320px width
- [ ] Accessibility: color-blind palette validated (redundant symbols for DO status)

**Owner:** Frontend Engineer + UX Designer

---

## Phase 10: System Integration & End-to-End Testing (Validation)

**Objective:** Run integrated tests across all subsystems: sensor → backend → laser/bubble → frontend.

**Deliverables:**
- End-to-end test suite: 12+ scenarios (normal operation, sensor timeout, laser disconnection, WebSocket drop, etc.)
- Load testing: maintain system stability with 50 WebSocket clients, 10,000 voxels/frame
- Latency budget validation: sensor poll → voxel render → display pixel update <200 ms
- Hardware integration test: real laser, real transducer, real sensor in saltwater tank
- Operator acceptance test: 3 human operators confirm intuitive control and visual feedback
- Regression testing: verify no existing features broken by new volumetric code

**Test Scenarios:**
1. **Normal Operation**: Load ocean profile, observe voxels render with correct colors
2. **Sensor Timeout**: YSI probe disconnected; display "STALE" indicator after 10 sec
3. **Laser Overheat**: Sustained 8 W for 1 hour; thermal shutdown at 85°C; recovery on cool-down
4. **Bubble Coalescence**: High density (1e8 /cm³) + no stabilizer; observe rapid voxel fading (bubble collapse)
5. **Network Latency**: Inject 500 ms delay on WebSocket; verify frontend stays responsive (polling fallback)
6. **Concurrent Slider Input**: Rapid wavelength + power + frequency changes; verify no state corruption
7. **Grid Resolution Change**: Switch from 100³ to 50³; confirm frame rate improves, voxel count decreases appropriately
8. **Multi-Client Sync**: 10 browsers connect; one changes laser power; all see update within 200 ms

**Phase Outcomes:**
- [ ] All 12 test scenarios passing, with pass criteria quantified
- [ ] End-to-end latency: P95 <200 ms (sensor → voxel → pixel)
- [ ] System uptime: 24-hour continuous run with <1 minute downtime (for maintenance)
- [ ] Load test: 50 WebSocket clients, zero dropped frames
- [ ] Operator feedback: ≥3/3 confirm intuitive control and visual fidelity
- [ ] Regression test: all existing features (if any) continue to work

**Owner:** QA Lead + Systems Engineer

---

## Phase 11: Seawater Deployment Adaptation (Ocean Hardening)

**Objective:** Validate system in real marine environment; optimize for salinity, temperature, pressure, biofouling.

**Deliverables:**
- Coastal tank deployment (1–5 m water depth): 1-week pilot with YSI sensor + live seawater
- Laser wavelength optimization: confirm 532 nm (or shift to 450 nm if needed) for seawater transmission
- Stabilizer persistence study: bubble lifetime vs. salinity and biofouling (algae, diatoms)
- Pressure compensation: test system at 50 m depth (5.1 atm), verify bubble nucleation still works
- Thermal stability: operate across 5–30°C ambient seawater temperature range
- Biofouling characterization: monitor YSI sensor drift due to biofilm growth
- Electrical safety: verify no corrosion, galvanic isolation, potted electronics

**Algorithm Decisions (48-Parameter Framework):**
1. **Wavelength Adaptation**: 532 nm (fixed) vs. dual-wavelength (532 + 405 nm) vs. tunable laser
   - Decision: 532 nm primary with optional 405 nm UV for germicidal (biofouling control)
   - Criteria: cost (532 DPSS lasers: $6.5k; tunable: $30k+), seawater transmission (Smith & Baker)
   - Reference: Sunlight attenuation in seawater (red absorbed by ~1 m, green by ~10 m, blue by ~50 m)

2. **Biofouling Mitigation**: Passive (optical window cleaning) vs. active (UV, copper, mechanical) vs. chemical
   - Decision: Hybrid (passive glass window + passive copper mesh on sensor + 30-min UV flush every 12 hours)
   - Criteria: low maintenance, non-toxic to marine life
   - Reference: Coastal platform maintenance literature

3. **Pressure-Depth Correlation**: Test at constant laboratory pressure vs. gradient profile
   - Decision: Conduct tests at 1 atm (lab), 2 atm (5 m sim), 5 atm (50 m sim)
   - Criteria: bubble nucleation stability (Minnaert shifts ~1% per atm)

**Phase Outcomes:**
- [ ] Coastal tank deployment completed: 1 week, zero major failures
- [ ] Laser wavelength validated for seawater: >95% transmission at 532 nm
- [ ] Bubble stabilizer persistence tested: lifetime >100 ms at 35 PSU (seawater salinity)
- [ ] Pressure test completed: system functional at 1–5 atm (0–50 m equivalent depth)
- [ ] Thermal range validated: stable operation from 5–30°C
- [ ] Sensor biofouling quantified: <2% drift per week under continuous seawater flow
- [ ] Electrical safety audit passed: no corrosion, isolation >1 MΩ

**Owner:** Marine Engineer + Field Operations Lead

---

## Phase 12: Production Hardening & Documentation (Release)

**Objective:** Finalize code quality, documentation, deployment pipeline, and operator training materials.

**Deliverables:**
- Code refactoring: eliminate technical debt, simplify interfaces
- Test coverage: target ≥90% line coverage for core modules (laser, bubble, sensor, mapper, service)
- Documentation suite:
  - **Operator Manual**: step-by-step startup, control guide, troubleshooting
  - **Technical Manual**: architecture, API reference, configuration tuning
  - **Hardware Manual**: sensor placement, laser alignment, transducer tuning
  - **Maintenance Schedule**: calibration intervals, parts replacement, firmware updates
- Docker containerization: reproducible deployment (Python backend + static frontend)
- CI/CD pipeline: automated testing on every push, linting (ruff), type checking (mypy), coverage reporting
- Training video: 5–10 min operator walk-through
- Release notes: version history, breaking changes, migration guide
- Deployment runbook: multi-environment (dev, staging, production)

**Algorithm Decisions (48-Parameter Framework):**
1. **Deployment Strategy**: Bare metal vs. Docker vs. Kubernetes
   - Decision: Docker for dev/staging; bare metal for embedded ocean deployment (reliability)
   - Criteria: operational simplicity in field; reproducible environments for debugging

2. **Configuration Management**: Environment variables vs. YAML files vs. cloud config service
   - Decision: Pydantic Settings + .env file (simple, familiar to data scientists)
   - Criteria: support 3 deployment targets without code changes

3. **Logging & Monitoring**: Syslog vs. cloud logging (Google Cloud Logging, Datadog, etc.) vs. local file
   - Decision: Local file + optional cloud forwarding (Datadog hook if available)
   - Criteria: works offline (marine deployment); cloud optional for monitoring

**Phase Outcomes:**
- [ ] Code review completed: all TODOs addressed, no code smells flagged by Ruff
- [ ] Test coverage ≥90% for core modules (modules with business logic)
- [ ] Documentation complete: all 5 manuals peer-reviewed and ready for operator use
- [ ] Docker image built and tested: reproduces lab environment on any system
- [ ] CI/CD pipeline active: linting, type checking, unit tests run on every commit
- [ ] Operator training completed: 3 staff trained and signed off
- [ ] Release tag created (v1.0.0); tagged on git with release notes in GitHub
- [ ] Deployment runbook executed: successful deployment to staging environment

**Owner:** DevOps Lead + Technical Writer + QA Lead

---

## Cross-Phase Validation Checkpoints

| Checkpoint | Phase(s) | Gate Criteria | Owner |
|------------|----------|---------------|-------|
| **Physics Validation** | 1-3 | Minnaert formula error <5% vs. experimental data | Fluid Dynamics Lead |
| **Optical Validation** | 4, 11 | Laser transmission >95% in seawater; no wavelength shift >2 nm | Optics Lead |
| **Sensor Accuracy** | 6, 10 | Sensor drift <2% over 7 days; outlier filter false positive rate <0.5% | Environmental Lead |
| **Rendering Performance** | 7, 9, 10 | Voxel frame renders in <50 ms; 60 FPS sustained on target hardware | Frontend Lead |
| **Real-Time Latency** | 8, 10 | End-to-end latency <200 ms (sensor poll → display pixel) | Systems Lead |
| **Reliability** | 10, 12 | 24-hour uptime >95%; frame drop rate <0.1% | QA Lead |
| **Deployment Readiness** | 12 | Code coverage ≥90%; operator training complete; runbook validated | DevOps Lead |

---

## Success Metrics (End of Phase 12)

- **Functional:** System renders real-time ocean data (T, S, DO, pH, turbidity) as animated 3D voxels
- **Performance:** 5 Hz render loop, <200 ms end-to-end latency, 60 FPS frontend animation
- **Reliability:** 24-hour uptime >95%, 0% data loss, graceful fault handling
- **Usability:** 3 trained operators can independently launch, run, and troubleshoot the system
- **Scalability:** Supports 50+ simultaneous WebSocket clients without frame drops
- **Seawater-Ready:** Validated in real marine environment at 0–50 m depth equivalent
- **Documented:** Complete operator, technical, and hardware manuals ready for deployment

---

## Dependencies & Critical Path

**Critical Path (longest duration):**
1. Phase 1 (requirements) → Phase 2 (literature) → Phase 3 (physics) → Phase 4 (laser) → Phase 8 (backend) → Phase 10 (integration) → Phase 12 (hardening)

**Parallelizable:** Phases 5, 6, 7 can run in parallel with Phase 4 during Phase 8 backend development.

**External Dependencies:**
- PubMed/Scholar API access for literature retrieval (Phase 2)
- YSI ProDSS probe hardware (Phase 6)
- Cambridge or Thorlabs galvo scanner (Phase 4)
- Coastal tank facility access (Phase 11)

---

## Revision History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2026-07-28 | Initial 12-phase HLD with scientific literature mapping and 48-parameter decision framework |

