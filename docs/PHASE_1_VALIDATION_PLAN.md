# Phase 1: Requirements & Architecture Review – Validation Plan
## Volumetric Laser-Bubble Ocean Display System

**Phase:** 1 of 12  
**Duration Target:** 2–3 weeks  
**Owner:** Systems Engineer + Marine Physicist  
**Date Started:** 2026-07-28  

---

## Executive Summary

Phase 1 establishes the physical and operational envelope for the volumetric display system before any algorithm implementation (Phase 3) or hardware assembly (Phases 4–5). This phase validates that the 12-phase design is achievable within real ocean constraints: pressure, temperature, salinity, light attenuation, and sensor accuracy budgets.

**Gate Criteria (All must pass before Phase 2):**
- Physics envelope document completed and reviewed by 3+ domain experts
- Sensor noise budget established: DO ±0.3 ppm, Temp ±0.1°C
- End-to-end latency budget: <200 ms (sensor → voxel render → display)
- Safety & environmental compliance checklist: zero critical gaps

---

## I. Physics Envelope Validation

### 1.1 Depth-Pressure-Bubble Dynamics

**Objective:** Verify that bubble nucleation and voxel persistence remain valid across typical oceanographic deployment depths.

**Test Plan:**
- **Baseline (1 atm / lab):** Measure bubble size distribution and Minnaert resonance predictions
- **Shallow (2 atm / ~10 m depth):** Repeat measurements under pressure chamber
- **Moderate (5 atm / ~50 m depth):** Final pressure test (typical coastal deployment)
- **Theory:** Minnaert frequency prediction error should remain <5% across pressure range

**Equipment Needed:**
- Pressure chamber with acoustic excitation (available at marine lab)
- Ultrasonic particle sizing camera (optical measurement of bubble radius)
- Function generator (40 kHz signal) with variable amplitude
- Transducer driver circuit (3–5 W input power)

**Success Criteria:**
- ✓ Minnaert predicted frequency vs. observed frequency: error <5% at each pressure level
- ✓ Bubble size distribution median: 82 µm ± 10 µm across all pressures
- ✓ No evidence of cavitation inception (sudden bubble collapse)
- ✓ Temperature control: all tests at 15–20°C, salinity 35 PSU reference

**Deliverable:** Physics_Envelope_Pressure_Report.md (2–3 pages + data tables)

---

### 1.2 Seawater Optical Attenuation

**Objective:** Confirm laser wavelength selection (532 nm green) against real seawater transmission curves.

**Test Plan:**
- Prepare 3 seawater samples: coastal (turbid), open ocean (clear), estuary (intermediate)
- Measure transmission of laser light at wavelengths: 405 nm (UV), 532 nm (green), 650 nm (red)
- Compare to Smith & Baker (1981) theoretical curves
- Estimate voxel visibility depth (when optical signal drops below sensor threshold)

**Equipment Needed:**
- Fiber-coupled laser combiner (3-wavelength source or tunable)
- Spectrophotometer or power meter
- 1-meter optical cuvette (seawater-grade acrylic)
- Seawater samples from coastal/open ocean stations

**Success Criteria:**
- ✓ 532 nm transmission >95% through 1 m clear seawater
- ✓ 532 nm transmission >70% through coastal turbid seawater (typical visibility 3–5 m)
- ✓ Measured attenuation matches Smith & Baker curves within ±10%
- ✓ No significant wavelength shift or dispersion artifacts

**Deliverable:** Physics_Envelope_Optics_Report.md (2–3 pages + spectral curves)

---

### 1.3 Bubble Rise Kinematics (Stokes Drag)

**Objective:** Predict voxel persistence time (how long a bubble stays in a single grid voxel) based on bubble rise velocity.

**Test Plan:**
- Use existing voxel grid resolution (100³ in 100 cm tank) → voxel height = 1 cm
- Measure upward rise velocity for 50 µm bubbles in quiescent seawater
- Apply Stokes drag law: v_rise = (2/9) · (ρ_bubble - ρ_water) · g · r² / η
  - ρ_bubble ≈ 0 (air); ρ_water = 1025 kg/m³ (seawater); η = 0.001 Pa·s
  - Expected: ~0.1–0.3 mm/s for 50 µm bubble
- Time in 1-cm voxel: t_persist = 10 mm / v_rise = 33–100 ms
- Verify against Frame persistence target (100–500 ms per 12_PHASE_HLD.md)

**Equipment Needed:**
- High-speed camera (240+ fps) with macro optics
- Transparent tank (5 L minimum, seawater-filled)
- Bubble generator (acoustic at 40 kHz, 1 W input)
- Image analysis software (particle tracking)

**Success Criteria:**
- ✓ Measured rise velocity within theoretical Stokes prediction ±20%
- ✓ Voxel persistence time 100–500 ms achievable with target grid resolution (100³)
- ✓ No unexpected bubble coalescence during rise (single bubbles tracked)
- ✓ Results consistent across bubble sizes (5–100 µm range)

**Deliverable:** Physics_Envelope_Kinematics_Report.md + bubble_rise_video.mp4 (10 sec clip)

---

## II. Sensor Noise & Accuracy Budget

### 2.1 YSI ProDSS Drift Characterization

**Objective:** Establish sensor calibration intervals and noise floor for DO, temperature, and pH.

**Test Plan:**
- **Baseline (Clean seawater):** Run YSI probe in stable 15°C, 35 PSU seawater for 24 hours
- **Record:** All 5 parameters (temp, salinity, DO, pH, turbidity) every 5 minutes
- **Analysis:** 
  - Noise floor: compute standard deviation over stable 1-hour windows
  - Drift rate: linear regression of mean value over 24 hours
  - Outlier frequency: count spikes >3σ per parameter
- **Repeat:** Test in coastal seawater + freshwater for comparison

**Equipment Needed:**
- YSI ProDSS probe (optical DO + electrode suite)
- Temperature-controlled seawater bath (±0.5°C stability)
- Data logger (automated serial polling at 200 ms interval)
- Reference standards (calibration buffers for pH, conductivity)

**Success Criteria:**
- ✓ DO noise floor: σ < 0.15 ppm (goal: ±0.3 ppm over 1-hour window)
- ✓ Temperature noise: σ < 0.05°C (goal: ±0.1°C over 1-hour)
- ✓ Drift rate: <0.5% per day for DO; <0.1°C per day for temp
- ✓ Outlier spike frequency: <1% of all samples
- ✓ Probe stability over 7-day continuous operation

**Deliverable:** Sensor_Drift_Report.md + telemetry_plot.png (time-series with noise bands)

---

### 2.2 Sensor Outlier Characteristics

**Objective:** Design outlier rejection filter (e.g., 3σ threshold) with validation against real sensor glitches.

**Test Plan:**
- Inject artificial outliers: disconnect probe briefly; reintroduce; observe spike recovery
- Simulate sensor faults: rapid temp swings (ice bath contact), DO sensor bubbles
- Log timestamp + magnitude of each spike
- Evaluate filter performance: false positive rate (good data rejected), false negative rate (outliers passed)

**Equipment Needed:**
- YSI probe + data logger (as above)
- Ice bath, hot water (inducing thermal stress)
- Nitrogen purge system (simulate oxygen depletion)

**Success Criteria:**
- ✓ 3σ threshold rejects >95% of artificial spikes
- ✓ False positive rate (good data rejected) <0.5%
- ✓ Recovery time after spike injection: <10 seconds (data returns to nominal)
- ✓ Filter parameters (window size, threshold) documented for Phase 6 implementation

**Deliverable:** Outlier_Filter_Validation_Report.md + spike_examples.csv

---

## III. End-to-End Latency Budget

### 3.1 Latency Path Analysis

**Objective:** Map latency from sensor poll → backend processing → laser command → voxel render → display pixel.

**Latency Breakdown (target: <200 ms end-to-end):**

```
Sensor Poll:           ~20 ms  (Modbus serial read @ 9600 baud)
Backend Processing:    ~30 ms  (Pydantic validation, voxel mapping)
Laser Command:         ~10 ms  (USB/serial to galvo driver)
Galvo Scan:            ~50 ms  (scan 100 voxels @ 2 voxels/ms)
Frontend WebSocket:    ~50 ms  (browser receive + DOM update)
Three.js Render:       ~30 ms  (particle update + GPU render)
Display Pixel:         ~10 ms  (monitor refresh, internal lag)
─────────────────────────────
TOTAL:                ~200 ms  ✓ (at budget limit)
```

**Measurement Plan:**
- Install instrumentation at each stage (logging timestamps)
- Run system under load: 50 WebSocket clients concurrent
- Measure P50 (median), P95 (95th percentile), P99 latency
- Flag any component >50 ms above budget

**Equipment Needed:**
- Complete system: laser + transducer + sensor + backend + frontend
- Network traffic analyzer (Wireshark for WebSocket timing)
- GPU profiler (for Three.js render times)

**Success Criteria:**
- ✓ P50 latency <180 ms
- ✓ P95 latency <200 ms
- ✓ No latency component >50 ms over target
- ✓ System stable under 50 concurrent WebSocket clients

**Deliverable:** Latency_Budget_Report.md + latency_profiles.json (timing samples)

---

## IV. Safety & Environmental Compliance

### 4.1 Laser Safety (IEC 60825-1)

**Objective:** Confirm system meets Class 3B laser safety standards (submarine eye hazard risk).

**Checklist:**
- ✓ Laser classification: 532 nm, 8 W → Class 4 (high hazard)
- ✓ Beam path fully enclosed (tank only)
- ✓ No uncontrolled reflected/scattered beam outside tank volume
- ✓ Eye hazard warning labels present and visible
- ✓ Interlock system: beam cut-off if tank access panel opened
- ✓ Galvo sync loss detector: beam blanked if scanner stops
- ✓ Power ramp-up control: slow ramp to prevent eye exposure during startup

**Deliverable:** Laser_Safety_Audit_Checklist.md + photos of safety features

---

### 4.2 Environmental Impact (Marine Biota)

**Objective:** Ensure acoustic and chemical impacts are acceptable for open-ocean deployment.

**Checklist:**
- ✓ Acoustic noise (40 kHz, ~100 dB re 1 µPa @ 1 m) below marine mammal hearing thresholds for project location
- ✓ SDS surfactant (0.5–2% by mass) toxicity: LD50 in marine organisms >100 ppm (check reagent SDS)
- ✓ Thermal impact on surrounding water: <0.5°C rise (no heating of marine habitat)
- ✓ Optical impact: no stray 532 nm light outside tank (prevent phytoplankton attraction)
- ✓ Electromagnetic: no stray RF interference from transducer driver (measure with spectrum analyzer)

**Deliverable:** Environmental_Impact_Assessment.md + toxicity_data_sheet.pdf

---

### 4.3 Electrical Safety (Personnel & Marine Life)

**Objective:** Verify grounding, isolation, and leakage current control.

**Tests:**
- Insulation resistance (megohm meter): >1 MΩ between AC input and water tank (submerged electronics)
- Leakage current (true-RMS clamp): <5 mA under worst-case fault (sensor connector shorted)
- Ground fault detection: verify GFCI trip <30 mA fault current
- Corrosion prevention: verify all underwater electronics potted or epoxy-sealed

**Deliverable:** Electrical_Safety_Test_Report.md + megohm_test_photos.jpg

---

## V. Deployment Environment Profiles

### 5.1 Three Representative Ocean Contexts

**Context 1: Coastal Estuary (High Variability)**
- Temperature: 5–25°C annual; 10–20°C during pilot
- Salinity: 5–35 PSU (freshwater input from river)
- Visibility: 1–3 m (moderate turbidity)
- Deployment depth: 2–5 m
- Thermal stratification: weak to moderate
- Objectives: Demonstrate system in challenging, variable conditions

**Context 2: Open Ocean (Stable, Deep)**
- Temperature: 5–15°C annual; 10–12°C at 50 m depth
- Salinity: 35–36 PSU (stable)
- Visibility: 20–50 m (clear seawater)
- Deployment depth: 10–100 m
- Thermal stratification: strong
- Objectives: Validate system in pristine, representative ocean environment

**Context 3: Bay/Harbor (Low Variability)**
- Temperature: 8–22°C annual
- Salinity: 30–35 PSU
- Visibility: 3–8 m
- Deployment depth: 5–20 m
- Thermal stratification: moderate
- Objectives: Comfortable test bed for operator training and early validation

**Phase 1 Focus:** All three contexts described; pilot deployments assigned to Phase 11.

---

## VI. Phase 1 Deliverables Checklist

- [ ] **Physics_Envelope_Report.md** (consolidated)
  - Pressure envelope validation (bubble dynamics at 1–5 atm)
  - Optical attenuation (wavelength transmission curves)
  - Bubble rise kinematics (voxel persistence prediction)
  - Safety margin analysis (design vs. experiment)

- [ ] **Sensor_Characterization_Report.md**
  - YSI ProDSS drift model (noise floor, calibration interval)
  - Outlier detection validation
  - Sensor noise budget for backend processing

- [ ] **Latency_Budget_Report.md**
  - Component-by-component latency measurements
  - P50/P95/P99 profiles under load
  - Identified bottlenecks and mitigation strategies

- [ ] **Deployment_Environment_Profiles.md**
  - Three oceanographic contexts (estuary, open ocean, bay)
  - Expected operating conditions for each context
  - Adaptation strategies for Phase 11 seawater deployment

- [ ] **Safety_Compliance_Audit.md**
  - Laser safety (IEC 60825-1) checklist: PASS
  - Environmental impact (NOAA marine biota guidelines): PASS
  - Electrical safety (personnel + marine life): PASS

- [ ] **Phase_1_Executive_Summary.md** (1-page)
  - Gate criteria status: ALL PASS / CONDITIONAL / FAIL
  - Critical risks and mitigation
  - Recommendation to proceed to Phase 2

---

## VII. Phase 1 Success Criteria

**All of the following must be TRUE:**

1. ✓ **Physics envelope validated:** Bubble dynamics stable across 1–5 atm; optical transmission >95% green; voxel persistence 100–500 ms achievable
2. ✓ **Sensor noise budget established:** DO ±0.3 ppm; temp ±0.1 °C achievable with outlier filter
3. ✓ **Latency budget confirmed:** <200 ms end-to-end achievable on target hardware
4. ✓ **Safety compliance:** Laser, environmental, and electrical safety audits: ZERO critical gaps
5. ✓ **Domain expert sign-off:** 3+ reviewers from (Physics, Marine Science, Electrical Engineering) approve all reports

**If ANY criterion is NOT met:**
- Document root cause in risk register
- Propose mitigation (design change, different component, trade-off acceptance)
- Escalate to project lead for decision (may delay Phase 2, trigger re-evaluation)

---

## VIII. Phase 1 – Phase 2 Handoff Criteria

**Condition for Phase 2 START:**
- All Phase 1 deliverables completed and reviewed
- Executive summary signed off: "PROCEED TO PHASE 2"
- Scientific literature bibliography initialized (at least 10 core references identified)
- Phase 2 task backlog groomed and assigned to Research Lead

**Phase 2 Entrance Gate:**
- Decision Log (claude.md): Phase 1 decisions (if any) documented
- Any Phase 1 findings that affect Phase 3–12 algorithms flagged for literature research

---

## IX. Risk Register (Phase 1)

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|-----------|
| Bubble coalescence under acoustic drive | Medium | High | Use reference literature (Leighton) to predict onset; measure in tank |
| Laser wavelength insufficient for coastal turbidity | Low | Medium | Have 405 nm UV backup; validate Smith & Baker curves first |
| Sensor drift faster than expected (YSI) | Low | Medium | Establish calibration interval <weekly; upgrade to lower-drift probe if needed |
| Latency exceeds budget (network jitter) | Medium | High | Measure on actual deployment network; switch to local USB connection if needed |
| Marine biota toxicity concern (SDS) | Low | High | Obtain third-party toxicity data; consider alternative surfactants if needed |

---

## X. Timeline & Ownership

| Week | Task | Owner | Dependency |
|------|------|-------|-----------|
| 1 | Pressure chamber tests (1.1) | Fluid Dynamics | Equipment access |
| 1–2 | Optical transmission measurement (1.2) | Optics Engineer | Seawater samples |
| 2 | Bubble kinematics (1.3) | Marine Physicist | High-speed camera |
| 2–3 | YSI sensor characterization (2.1, 2.2) | Environmental Lead | Sensor probe, bath |
| 3 | Latency measurement on actual system (3.1) | Backend Engineer | Complete system assembled |
| Throughout | Safety compliance audits (Section IV) | Electrical Safety Officer | Ongoing review |
| End of Week 3 | Phase 1 Executive Summary | Systems Lead | All reports complete |

---

## XI. Communication & Reviews

**Weekly Sync:** Tuesdays, 2 PM – Review progress against Phase 1 checklist

**Bi-weekly Domain Expert Review:** Thursdays, 10 AM – Interim reports; design adjustments

**Phase 1 Gate Review:** End of Week 3 – Final sign-off on Executive Summary (Project Lead + 3 domain experts)

---

## Appendix A: Reference Datasets

The following oceanographic data will be used to validate Phase 1 predictions:

- **Coastal Estuary:** Chesapeake Bay long-term monitoring (NOAA)
- **Open Ocean:** BATS (Bermuda Atlantic Time Series) buoy data
- **Lab Tank:** Historical data from marine lab (if available)

Data files to be archived in `docs/reference_datasets/` for Phase 1 validation and Phase 12 documentation.

---

**Revision:** 1.0  
**Date:** 2026-07-28  
**Status:** Ready for Phase 1 Kick-off

