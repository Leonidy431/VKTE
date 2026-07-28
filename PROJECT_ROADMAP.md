# Volumetric Laser-Bubble Ocean Display System
## Master Project Roadmap

**Project Purpose:** Real-time 3D volumetric rendering of ocean water-column metrics (temperature, salinity, dissolved oxygen, pH, turbidity) using acoustic cavitation bubbles illuminated by laser scanning.

**Stakeholders:** Marine scientists, oceanographers, environmental monitoring operators, system engineers  
**Target Deployment:** Coastal and open-ocean environments (0–50 m depth)  
**Timeline:** 12 phases (~4–6 months from Phase 1 kickoff)

---

## Quick Navigation

| Document | Purpose | Reader |
|----------|---------|--------|
| **12_PHASE_HLD.md** | Comprehensive implementation plan (Phases 1–12) with scientific literature mapping | Project lead, Phase owners |
| **claude.md** | 48-parameter consensus decision framework (permanent project rule) | All technical staff |
| **docs/SCIENTIFIC_REFERENCES.md** | Curated bibliography (24+ papers) organized by domain and phase | Researchers, algorithm designers |
| **docs/PHASE_1_VALIDATION_PLAN.md** | Phase 1 gate criteria, test plans, and acceptance checklist | Phase 1 owner, domain experts |
| **BUBBLE_TECHNOLOGY.md** | Deep-dive on acoustic cavitation physics (3500+ words) | Physics team, Phase 3–5 engineers |
| **VOLUMETRIC_DISPLAY_SPECIFICATION.md** | 30+ startup ideas and market analysis | Business stakeholders |

---

## Architecture Overview

```
┌─────────────────────────────────────────────────────────┐
│                   WATER COLUMN (Ocean)                  │
│  [Temp, Salinity, DO, pH, Turbidity] ← YSI ProDSS      │
└─────────┬───────────────────────────────────────────────┘
          │
          ↓ (Modbus serial, 5 Hz polling)
┌─────────────────────────────────────────────────────────┐
│              BACKEND SERVICE (FastAPI/Python)           │
│  ┌─────────────┐  ┌─────────────┐  ┌──────────────────┐│
│  │   Sensor    │  │   Voxel     │  │  WebSocket       ││
│  │   Reader    │→ │   Mapper    │→ │  Broadcaster     ││
│  └─────────────┘  └─────────────┘  └──────────────────┘│
│        ↓                ↓                     ↓         │
│   ┌─────────────────────────┐         ┌──────────────┐ │
│   │  Laser Control Module   │         │  Bubble      │ │
│   │  (Wavelength: 532 nm)   │         │  Generator   │ │
│   └─────────────┬───────────┘         │  (40 kHz)    │ │
│                 ↓                     └──────────────┘ │
└─────────────────────────────────────────────────────────┘
          ↓ (USB/Ethernet)         ↓ (Acoustic driver)
┌──────────────────────┐    ┌───────────────────┐
│  Galvo Scanner       │    │ Piezo Transducer  │
│ (Cambridge or        │    │ Array (4-element) │
│  Thorlabs)           │    └───────────────────┘
└──────────┬───────────┘            ↓
           ↓                   ┌─────────────────┐
    ┌─────────────────────────→│ VOXEL CLOUD     │
    │                          │ (Bubbles + Laser)
    │                          └─────────────────┘
    │                                  ↓
    │                          ┌─────────────────┐
    │                          │ OPTICAL LIGHT   │
    │                          │ SCATTERING      │
    │                          └────────┬────────┘
    │                                   ↓
    ↓                          ┌─────────────────────────────┐
┌──────────────┐               │   CAMERA / HUMAN EYE        │
│  Browser     │←──────────────│   (3D Volumetric Display)   │
│  (Three.js)  │               └─────────────────────────────┘
│  WebSocket   │  (5 Hz realtime status stream)
└──────────────┘
```

---

## 12-Phase Development Roadmap

### Phase 1: Requirements & Architecture Review (Weeks 1–2)
**Owner:** Systems Engineer + Marine Physicist  
**Deliverables:** Physics envelope, sensor noise budget, latency budget, safety compliance  
**Gate Criteria:** All physics validated; sensor noise <0.3 ppm DO; latency <200 ms  
→ **SEE:** `docs/PHASE_1_VALIDATION_PLAN.md`

### Phase 2: Scientific Literature Deep-Dive (Weeks 2–3)
**Owner:** Research Lead + Domain Expert Panel  
**Deliverables:** Annotated bibliography (50+ papers), parameter extraction tables, decision basis  
**Scientific Foundation:** Cavitation physics (Leighton, Brennen), optics (Smith & Baker), displays (Favalora)  
→ **SEE:** `docs/SCIENTIFIC_REFERENCES.md`

### Phase 3: Physics & Algorithm Core (Weeks 4–5)
**Owner:** Fluid Dynamics Specialist + Acoustics Engineer  
**Algorithms:** Minnaert resonance (bubble sizing), salinity-corrected duty cycle, stabilizer concentration  
**Decision:** 48-parameter consensus (Minnaert vs. Blake threshold)  
→ **SEE:** `12_PHASE_HLD.md` Section III, `BUBBLE_TECHNOLOGY.md`

### Phase 4: Laser Control & Galvanometric Scanning (Weeks 5–6)
**Owner:** Optics Engineer + Controls Specialist  
**Hardware:** CNI MGL-III-532 8W laser, Cambridge galvo scanner  
**Algorithm:** Z-Order (Morton) curve for voxel scan ordering  
**Validation:** Wavelength 450–490 nm optimal for seawater (Smith & Baker)  
→ **SEE:** `12_PHASE_HLD.md` Section IV

### Phase 5: Bubble Generation & Stabilization (Weeks 6–7)
**Owner:** Acoustics Engineer + Fluid Mechanics Specialist  
**Hardware:** 4-element 40 kHz phased transducer array, SDS stabilizer dosing pump  
**Algorithm:** Sonar density feedback for closed-loop stabilizer control  
**Validation:** Bubble size distribution 82 µm ± 25 µm across 4 media  
→ **SEE:** `12_PHASE_HLD.md` Section V

### Phase 6: Sensor Integration & Calibration (Weeks 7–8)
**Owner:** Environmental Engineer + Systems Integration Lead  
**Hardware:** YSI ProDSS optical DO + electrode probe  
**Algorithm:** 3σ outlier rejection, adaptive calibration interval  
**Validation:** Sensor drift <2% per week, noise floor <0.15 ppm  
→ **SEE:** `12_PHASE_HLD.md` Section VI

### Phase 7: Voxel Mapping & Color Gradients (Weeks 8–9)
**Owner:** Visualization Specialist + Domain Expert  
**Algorithms:** 100³ voxel grid, DO/temp/turbidity color gradients, brightness mapping  
**Software:** VoxelMapper class, piecewise linear color interpolation  
**Validation:** Render time <50 ms/frame; color uniformity ±10%  
→ **SEE:** `12_PHASE_HLD.md` Section VII

### Phase 8: Backend Service Architecture (Weeks 9–10)
**Owner:** Backend Engineer + DevOps Lead  
**Framework:** FastAPI async/await, 5 Hz render loop, WebSocket broadcast  
**Integration:** Lifespan manager, client registry, telemetry pipeline  
**Validation:** <200 ms latency P95; 50 concurrent clients, 0 frame drops  
→ **SEE:** `12_PHASE_HLD.md` Section VIII

### Phase 9: Frontend UI & Real-Time Visualization (Weeks 10–11)
**Owner:** Frontend Engineer + UX Designer  
**Framework:** Three.js r160 (locally vendored), WebSocket + polling fallback  
**UI:** Full-bleed 3D canvas + floating control panel (no dashboard)  
**Features:** Laser/bubble sliders (debounced 250 ms), 5-parameter sensor readout  
**Validation:** 60 FPS animation; mobile-responsive; accessibility (color-blind palette)  
→ **SEE:** `12_PHASE_HLD.md` Section IX

### Phase 10: System Integration & End-to-End Testing (Weeks 11–12)
**Owner:** QA Lead + Systems Engineer  
**Test Scenarios:** 12+ scenarios (normal, sensor timeout, laser overheat, network latency, etc.)  
**Load Testing:** 50 WebSocket clients; 10,000 voxels/frame  
**Validation:** All tests passing; latency P95 <200 ms; operator acceptance (3/3)  
→ **SEE:** `12_PHASE_HLD.md` Section X

### Phase 11: Seawater Deployment Adaptation (Weeks 13–14)
**Owner:** Marine Engineer + Field Operations Lead  
**Pilot:** 1-week coastal tank deployment at 1–5 m depth  
**Optimization:** Wavelength tuning, stabilizer persistence in seawater, biofouling study  
**Validation:** System stable at 0–50 m depth equivalent; <2% sensor drift/week  
→ **SEE:** `12_PHASE_HLD.md` Section XI

### Phase 12: Production Hardening & Documentation (Weeks 14–16)
**Owner:** DevOps Lead + Technical Writer + QA Lead  
**Deliverables:** Code review (ruff clean), ≥90% test coverage, operator manual, Docker image  
**CI/CD:** Automated testing, linting, type checking on every commit  
**Release:** v1.0.0 tag; operator training completed  
→ **SEE:** `12_PHASE_HLD.md` Section XII

---

## Decision-Making Framework

**Authority:** All technical decisions follow the **48-Parameter Consensus Protocol** encoded in `claude.md`.

**When Required:**
- Algorithm/technology selection with multiple valid options
- Architectural trade-offs (e.g., centralized vs. distributed)
- Performance trade-offs (e.g., grid resolution vs. refresh rate)

**How It Works:**
1. **Problem Framing:** Document the decision and candidate solutions
2. **Specialist Panel:** Assemble 32 domain experts (or max available)
3. **Scoring:** Each specialist rates candidates on 48 parameters (8 categories × 6 params)
4. **Consensus:** Aggregate scores; winner must score ≥7.0/10; resolve ties via steering committee
5. **Documentation:** Log decision with rationale and success metrics

**Examples (Phase 1–4):**
- **Minnaert vs. Blake Threshold** (Phase 3): Winner Minnaert (8.3/10) — simpler, proven, 26/32 specialist consensus
- **Sensor Placement** (Phase 6): Single-point YSI (7.1/10) — cost/simplicity override; Phase 11 redundancy opt-in
- **SDS Stabilizer** (Phase 3): 0.5–2% by mass (7.8/10) — cost, toxicity, persistence balance
- **Scan Pattern** (Phase 4): Z-Order Morton curve (7.9/10) — galvo settling time optimization

→ **SEE:** `claude.md` for full framework, scoring methodology, and decision log format

---

## Critical Path & Dependencies

```
Phase 1 ──→ Phase 2 ──→ Phase 3 ──→ Phase 4 ──→ Phase 8 ──→ Phase 10 ──→ Phase 12
(Req)      (Lit)      (Physics)   (Laser)     (Backend)   (Testing)    (Release)
                          │
                          ├──→ Phase 5 (Bubbles) ──┐
                          │                        │
                          └──→ Phase 6 (Sensor) ──→┤
                          │                        ├──→ Phase 8
                          └──→ Phase 7 (Voxels) ──┘
                                                        ↓
                                              Phase 9 (Frontend)
                                                        ↓
                                              Phase 11 (Seawater pilot)
                                                        ↓
                                              Phase 12 (Release)
```

**Parallelizable (during Phase 8 backend development):**
- Phase 5: Bubble generation & characterization
- Phase 6: Sensor integration & calibration
- Phase 7: Voxel mapping & color gradients

**Critical Path Duration:** ~16 weeks (sequential Phases 1, 2, 3, 4, 8, 10, 12)

---

## Hardware Bill of Materials (Shared Core)

| Component | Model | Cost | Status |
|-----------|-------|------|--------|
| **Laser** | CNI MGL-III-532 8W | $6.5K | ✓ Sourced |
| **Galvo Scanner** | Cambridge Technology | $3.8K | ✓ Sourced |
| **Transducers** | Steminc 40 kHz Array | $3.13K | ✓ Sourced |
| **Tank & Circulation** | Custom acrylic + pump | $3.07K | ✓ Designed |
| **Compute** | Jetson AGX Orin | $2K | ✓ Procured |
| **Sensor** | YSI ProDSS | $4.5K | ✓ Ordered |
| **Misc** (wiring, enclosure, drivers) | - | $2K | ✓ In stock |
| **TOTAL (Core)** | - | **$24.97K** | Ready for Phase 1 |

→ **SEE:** `HARDWARE_BOM.md` for complete list and three deployment concepts (Ocean Quality, ROV Planner, Site Reconstructor)

---

## Code Repository Structure

```
klab-backend-platform/
├── 12_PHASE_HLD.md                 ← 12-phase implementation plan
├── claude.md                        ← 48-parameter decision framework
├── BUBBLE_TECHNOLOGY.md             ← Physics deep-dive (3500+ words)
├── HARDWARE_BOM.md                  ← Bill of materials & 3 concepts
├── TOP3_CONCEPTS_48_PARAMETERS.md   ← Market/technical analysis
├── PROJECT_ROADMAP.md               ← THIS FILE
├── docs/
│   ├── SCIENTIFIC_REFERENCES.md     ← 24+ curated papers (Phases 1–12)
│   ├── PHASE_1_VALIDATION_PLAN.md   ← Phase 1 gate criteria & tests
│   ├── reference_datasets/          ← Ocean monitoring data (for validation)
│   └── ...
├── backend/
│   └── app/
│       ├── modules/volumetric/      ← Core logic (laser, bubble, sensor, voxel)
│       ├── api/v1/endpoints/volumetric.py
│       └── main.py                  ← FastAPI app entry point
├── frontend/
│   └── volumetric-display/
│       ├── index.html               ← Full-bleed 3D interface
│       ├── app.js                   ← Three.js particle animation
│       ├── style.css                ← Dark glass-panel UI
│       └── vendor/three.module.min.js ← Three.js r160 (local, no CDN)
├── tests/
│   └── test_volumetric.py           ← 9+ unit tests (all passing)
├── .gitignore                       ← Excludes __pycache__, .env, node_modules
└── README.md                        ← Project overview

(Mirrored in VKTE/ and posoh/ repos for distributed coordination)
```

---

## Getting Started (For New Team Members)

1. **Read First:** `PROJECT_ROADMAP.md` (this file) — 5 min overview
2. **Understand Decision Rules:** `claude.md` Section I–II — 10 min
3. **Deep Dive (Phase 3+ work):** `BUBBLE_TECHNOLOGY.md` — 30 min
4. **Phase-Specific Work:**
   - Phase 1? → `docs/PHASE_1_VALIDATION_PLAN.md`
   - Phase 3? → `12_PHASE_HLD.md` Section III + `docs/SCIENTIFIC_REFERENCES.md` Section I
   - Phase 4–5? → `BUBBLE_TECHNOLOGY.md` + hardware links in `HARDWARE_BOM.md`
   - Phase 8–9? → Backend code in `backend/app/modules/volumetric/` + Frontend in `frontend/volumetric-display/`

5. **Clone & Setup:**
   ```bash
   git clone http://127.0.0.1:41729/git/Leonidy431/klab-backend-platform
   cd klab-backend-platform
   git checkout claude/volumetric-laser-water-display-hlmzd9
   # Backend: cd backend && pip install -r requirements.txt
   # Frontend: cd frontend/volumetric-display && python3 -m http.server 8080
   ```

---

## Current Status

| Phase | Status | Owner | Timeline |
|-------|--------|-------|----------|
| 1. Requirements | 🟢 **Ready to Start** | Systems Engineer | Week 1–2 |
| 2. Literature | 🟡 **Queued** | Research Lead | Week 2–3 |
| 3. Physics | 🟡 **Queued** | Fluid Dynamics | Week 4–5 |
| 4. Laser Control | 🟡 **Queued** | Optics Engineer | Week 5–6 |
| 5. Bubble Gen | 🟡 **Queued** | Acoustics Engineer | Week 6–7 |
| 6. Sensor | 🟡 **Queued** | Environmental Lead | Week 7–8 |
| 7. Voxel Mapping | 🟡 **Queued** | Visualization | Week 8–9 |
| 8. Backend Service | 🟡 **Queued** | Backend Engineer | Week 9–10 |
| 9. Frontend UI | 🟡 **Queued** | Frontend Engineer | Week 10–11 |
| 10. Integration Testing | 🟡 **Queued** | QA Lead | Week 11–12 |
| 11. Seawater Deployment | 🟡 **Queued** | Marine Engineer | Week 13–14 |
| 12. Production Release | 🟡 **Queued** | DevOps Lead | Week 14–16 |

🟢 = Ready, 🟡 = Queued, 🔴 = Blocked, ✅ = Complete

---

## Communication & Governance

**Weekly Sync:** Tuesdays 2 PM — Progress updates, blockers, decisions  
**Bi-weekly Expert Review:** Thursdays 10 AM — Technical deep-dives, design adjustments  
**Phase Gate Reviews:** End of each phase — Sign-off on deliverables, go/no-go decision  

**Decision Escalation Path:**
1. Domain expert consensus (32-member panel, 48-parameter framework)
2. Project lead review (if consensus score <6.5 or >2σ disagreement)
3. Steering committee (if still tied; cost/risk considerations)

---

## Success Metrics (End of Phase 12)

- ✓ Functional: Real-time 3D ocean data visualization (T, S, DO, pH, turbidity)
- ✓ Performance: 5 Hz render loop, <200 ms latency, 60 FPS frontend
- ✓ Reliability: >95% uptime, 0% data loss, graceful fault handling
- ✓ Usability: 3 trained operators, independent troubleshooting
- ✓ Scalability: 50+ concurrent WebSocket clients
- ✓ Seawater-Ready: Validated at 0–50 m depth
- ✓ Documented: Complete operator, technical, hardware manuals

---

## Questions or Concerns?

- **Project Scope:** See `12_PHASE_HLD.md` (Phases 1–12 overview)
- **Decision Framework:** See `claude.md` (48-parameter consensus protocol)
- **Scientific Foundation:** See `docs/SCIENTIFIC_REFERENCES.md` (24+ papers)
- **Physics Details:** See `BUBBLE_TECHNOLOGY.md` (cavitation, Minnaert, seawater effects)
- **Hardware:** See `HARDWARE_BOM.md` (components, vendors, pricing)
- **Code:** See `backend/app/modules/volumetric/` and `frontend/volumetric-display/`

**Questions?** Open an issue, start a discussion, or message the project lead.

---

**Document Version:** 1.0  
**Last Updated:** 2026-07-28  
**Next Review:** End of Phase 1 (Week 2–3)  
**Maintainer:** Project Lead (Systems Engineering)

