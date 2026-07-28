# Claude Project Rules & Decision-Making Framework
## Volumetric Laser-Bubble Ocean Display System

**Effective Date:** 2026-07-28  
**Project:** Acoustic cavitation volumetric display for real-time oceanographic visualization  
**Stakeholders:** Marine scientists, systems engineers, field operations team  

---

## Core Principle: 48-Parameter Consensus Decision Framework

### Authority & Mandate

When algorithm selection, technology choice, system architecture decision, or design trade-off is **ambiguous** (i.e., multiple technically valid options exist with different trade-offs), the decision **must** be made using the **48-Parameter Consensus Protocol**. This is a non-negotiable project rule. It ensures technical decisions are grounded in scientific literature, domain expertise, and parametric reasoning—not gut feel.

**Who Decides?**
- **Primary:** Domain expert consensus panel (32 specialists, per problem domain)
- **Secondary:** If full 32-member panel is unavailable, use best available expert count (minimum 5) with explicit uncertainty notation
- **Fallback:** If consensus cannot be reached, invoke escalation to project lead (see Section 3)

**When to Apply?**
- New algorithm or method selection (e.g., "Minnaert vs. Blake for resonant frequency prediction")
- Technology choice (e.g., "532 nm vs. 405 nm vs. tunable laser")
- Architectural decision (e.g., "centralized service vs. distributed agents")
- Performance trade-off (e.g., "grid resolution 50³ vs. 100³ vs. 200³")
- Scope/priority conflict (e.g., "add seawater salinity calibration now vs. Phase 11?")

**NOT Required For:**
- Bug fixes (use technical judgment + code review)
- Refactoring (use code quality metrics + testing)
- Documentation updates (use style guide)
- One-off operational decisions (use runbook + supervisor approval)

---

## The 48-Parameter Evaluation Matrix

All technology/algorithm decisions use an 8-category × 6-parameter matrix. Each category weighs equally (12.5% of total score); parameters within a category are evaluated on a 1–10 scale.

### Category 1: Identity & Problem Framing (12.5% weight)
1. **Name/Label** — Clear, unambiguous term for the candidate solution
2. **Problem Statement** — What specific ambiguity or trade-off does this solve?
3. **Scope** — Does this affect algorithm, architecture, or both? (breadth impact)
4. **Novelty** — New method (1–3), proven in lab (4–7), production-grade (8–10)
5. **Precedent** — Prior art / academic reference score (0 = novel, 10 = well-established)
6. **Risk Class** — Low (8–10), medium (4–7), high (1–3) based on unknowns

### Category 2: Scientific Foundation (12.5% weight)
1. **Literature Support** — Number of peer-reviewed papers supporting the approach (0–10 scale)
2. **Theoretical Grounding** — Mathematical/physical derivation quality (1–10)
3. **Experimental Validation** — Lab data confirming the theory (1–10)
4. **Domain Applicability** — How well does prior research apply to ocean/seawater context? (1–10)
5. **Parameter Sensitivity** — How robust is the method across operating envelope? (1–10)
6. **Failure Mode Understanding** — Known failure modes documented & mitigatable? (1–10)

### Category 3: Performance & Technical Merit (12.5% weight)
1. **Latency** — End-to-end latency impact (<10ms increment: 10, >100ms: 1–3)
2. **Throughput** — Voxel render rate (kHz) or sensor polling rate (Hz) (1–10 scale)
3. **Accuracy** — Measurement/rendering error budget (low error: 10, high error: 1)
4. **Power Consumption** — Watts or energy per operation (low: 10, high: 1)
5. **Scalability** — Handles 10× growth in parameters? (yes: 8–10, no: 1–3)
6. **Determinism** — Predictable behavior? No race conditions? (deterministic: 10, probabilistic: 1–5)

### Category 4: Implementation Complexity (12.5% weight)
1. **Code Lines** — Complexity vs. benefit (fewer is better; 1–10 score)
2. **Dependencies** — External libraries/packages required (fewer: 10, many: 1)
3. **Debugging Difficulty** — Can we instrument and observe state? (observable: 8–10, hidden: 1–3)
4. **Testing Effort** — How many test cases needed for confidence? (few: 10, many: 1–5)
5. **Maintainability** — Is the code self-documenting? Will a junior engineer grok it? (yes: 8–10, no: 1–3)
6. **Reusability** — Can this module be reused in future projects? (high reuse: 10, one-off: 1–3)

### Category 5: Cost & Resource Allocation (12.5% weight)
1. **Hardware Cost** — BOM delta vs. baseline (low cost: 10, high cost: 1)
2. **Development Time** — Estimated person-hours to implement (few: 10, many: 1–3)
3. **Integration Effort** — How invasive are changes to existing code? (surgical: 10, invasive: 1–3)
4. **Operational Cost** — Power, consumables, maintenance (low: 10, high: 1)
5. **Training Burden** — How much operator training? (none: 10, extensive: 1–3)
6. **Scalability Cost** — Does cost per unit decrease with volume? (yes: 8–10, no: 1–3)

### Category 6: Reliability & Robustness (12.5% weight)
1. **Mean Time Between Failures (MTBF)** — Predicted hours to failure (high: 10, low: 1)
2. **Graceful Degradation** — Does system fail safely? (safe: 8–10, catastrophic: 1–3)
3. **Fault Detection** — Can we detect failures in real-time? (yes: 8–10, no: 1–3)
4. **Recovery Strategy** — Can operators recover without hardware swap? (yes: 8–10, no: 1–3)
5. **Environmental Tolerance** — Works across 5–30°C, 0–50m depth, 0–40 PSU? (wide: 8–10, narrow: 1–3)
6. **Monitoring Depth** — Telemetry granularity for diagnostics (detailed: 10, opaque: 1)

### Category 7: Regulatory & Environmental (12.5% weight)
1. **Safety Compliance** — Electrical safety, laser eye safety, marine biota protection (full: 10, gaps: 1–5)
2. **Environmental Impact** — Toxicity, biofouling, electromagnetic interference (low: 10, high: 1)
3. **Regulatory Burden** — Permits, certifications, documentation (minimal: 10, extensive: 1–3)
4. **Reversibility** — Can we undo this choice if it fails? (yes: 8–10, no: 1–3)
5. **Long-term Viability** — Technology sundown risk (low: 8–10, high: 1–3)
6. **Community Standards** — Alignment with ocean/marine lab best practices (aligned: 8–10, outlier: 1–3)

### Category 8: Strategic & Business Value (12.5% weight)
1. **Competitive Advantage** — Does this differentiate our system? (yes: 8–10, commoditized: 1–3)
2. **Market Fit** — Aligns with stakeholder priorities? (high: 8–10, orthogonal: 1–3)
3. **Optionality** — Leaves doors open for future pivots? (yes: 8–10, locks us in: 1–3)
4. **Partnerships** — Leverages existing vendor/integration relationships? (yes: 8–10, new: 1–3)
5. **IP Potential** — Patentable or defensible? (strong: 8–10, weak: 1–3)
6. **Momentum** — Builds toward Phase 12 vision? (yes: 8–10, tangential: 1–3)

---

## Scoring & Consensus Protocol

### Step 1: Candidate Identification
- Frame the decision: "For [problem], we have candidate solutions: (A), (B), (C)"
- Document trade-offs: 1–2 sentence per candidate
- List domain specialists: identify 32 experts (or max available)

### Step 2: Specialist Evaluation
Each of the 32 specialists independently scores all candidates on the 48-parameter matrix:
- **Scale:** 1 (poor fit) to 10 (excellent fit)
- **Output:** 8 category subscores (1–10 each), then average for final candidate score (1–10)
- **Reasoning:** Each specialist documents 1–2 key drivers for their scoring

### Step 3: Consensus Aggregation
- Calculate mean score across 32 specialists for each candidate
- If any category has >2 standard deviation disagreement, flag for discussion
- **Winning Candidate:** Highest mean score (≥7.0 preferred; ≥6.5 acceptable if strong category agreement)

### Step 4: Ratification & Documentation
- Project lead reviews top 2–3 candidates and specialist disagreements
- If decision is clear (winner >1.5 points ahead): approve and log
- If decision is close (<1.0 point spread): escalate to steering committee or conduct sensitivity analysis
- **Output:** Decision log entry with:
  - Candidate name + final score
  - 3–5 key decision drivers (categories with highest impact)
  - Specialist panel composition (names, domains)
  - Implementation timeline and owner
  - Success metrics and rollback plan (if applicable)

### Step 5: Implementation & Monitoring
- Development team implements chosen candidate
- Monitor actual performance vs. predicted scores during first 4 weeks
- If actual performance diverges >20% from predicted: root-cause analysis + optional re-evaluation

---

## Decision Log Format

```markdown
## Decision: [Short Title]
**Date:** YYYY-MM-DD  
**Phase:** [Which of 12 phases]  
**Category:** [Algorithm / Architecture / Performance / Cost / Regulatory]

### Problem Statement
[1–2 sentences describing the ambiguity and why it matters]

### Candidates Evaluated
- **Candidate A:** [Short description + key trade-offs]
- **Candidate B:** [Short description + key trade-offs]
- **Candidate C:** [Short description + key trade-offs]

### Specialist Panel
- Count: N/32 available (or reason for gap)
- Domains: [List of expertise areas, e.g., Acoustics, Optics, Controls, Marine Biology, etc.]

### Scores
| Candidate | Identity | Science | Performance | Complexity | Cost | Reliability | Regulatory | Strategic | **FINAL** |
|-----------|----------|---------|-------------|------------|------|-------------|------------|-----------|-----------|
| A | 7.2 | 8.1 | 7.8 | 6.5 | 5.9 | 8.3 | 7.6 | 6.8 | **7.3** |
| B | 6.8 | 7.4 | 8.5 | 8.1 | 7.2 | 7.9 | 6.2 | 7.1 | **7.5** |
| C | 5.1 | 6.3 | 6.9 | 7.2 | 8.8 | 6.5 | 8.1 | 5.4 | **6.8** |

**Winner:** Candidate B (Score: 7.5/10)

### Key Decision Drivers
1. **Performance Category:** B scored 8.5 vs. A's 7.8—significant improvement in voxel throughput
2. **Complexity Category:** B's 8.1 vs. A's 6.5—simpler implementation reduces risk
3. **Regulatory Category:** A scored 7.6 vs. B's 6.2—A had slight edge, but offset by B's performance gains
4. **Specialist Consensus:** 28/32 specialists ranked B in top 2; only 1 ranked C as winner

### Implementation Plan
- **Owner:** [Name + team]
- **Timeline:** [Weeks X–Y of Phase Z]
- **Success Metric:** [How do we know B was the right choice?]
- **Rollback Plan:** [Conditions under which we revert; estimated effort]

### Actual Outcome (Post-Implementation)
*[To be filled in after 4 weeks of deployment]*
- **Predicted vs. Actual Performance:** [Latency, throughput, accuracy, cost, etc.]
- **Learnings:** [What surprised us? What did we get right?]
- **Adjustments:** [Any tuning or course-correction needed?]

```

---

## Example Decision: Minnaert vs. Blake Threshold for Resonant Frequency

**Date:** 2026-07-28  
**Phase:** 3 (Physics & Algorithm Core)  
**Category:** Algorithm

### Problem Statement
Predicting bubble resonant frequency is critical for stable cavitation voxel generation. Two competing models exist: Minnaert (simple, frequency-dependent, older) and Blake threshold (accounts for nucleation sites, newer). Which predicts bubble radius better in our operating envelope (20–80 kHz)?

### Candidates Evaluated
- **Minnaert Equation:** r₀ = (1 / (2π·f)) · sqrt(3·γ·P₀ / ρ); simpler, widely used, decades of ocean acoustics data
- **Blake Threshold:** Pressure-dependent, accounts for nucleation energy barrier; more complex; newer literature
- **Hybrid:** Use Minnaert for coarse prediction, Blake for edge cases (very low freq, high pressure)

### Specialist Panel
- **Acoustics Engineers:** 8 (ocean acoustics, transducer design, cavitation regimes)
- **Fluid Dynamics:** 6 (bubble mechanics, Stokes drag, multiphase flow)
- **Marine Physicists:** 5 (seawater properties, salinity effects, temperature coupling)
- **Systems Engineers:** 5 (integration complexity, real-time tuning)
- **Laser/Optics Specialists:** 4 (cross-coupling between bubble size and optical scattering)
- **Marine Biologists:** 2 (acoustic impact on marine life; less relevant but advisory)
- **Software Engineers:** 2 (implementation effort, CPU budget)

**Total Panel:** 32/32 available

### Scores (Abbreviated; full matrix in decision log file)

| Candidate | Identity | Science | Performance | Complexity | Cost | Reliability | Regulatory | Strategic | **FINAL** |
|-----------|----------|---------|-------------|------------|------|-------------|------------|-----------|-----------|
| **Minnaert** | 8.1 | 7.8 | 7.5 | 9.2 | 9.5 | 8.1 | 8.3 | 7.6 | **8.3** |
| **Blake** | 7.2 | 8.6 | 8.1 | 5.8 | 6.2 | 7.4 | 7.9 | 6.8 | **7.3** |
| **Hybrid** | 7.6 | 8.2 | 7.8 | 6.5 | 7.1 | 7.9 | 8.0 | 7.2 | **7.6** |

**Winner:** Minnaert (Score: 8.3/10)

### Key Decision Drivers
1. **Complexity Category:** Minnaert 9.2 vs. Blake 5.8—significant simplicity advantage; faster tuning in field
2. **Cost Category:** Minnaert 9.5 vs. Blake 6.2—no additional hardware or calibration data needed
3. **Science Category:** Blake 8.6 vs. Minnaert 7.8—Blake slightly more rigorous; offset by Minnaert's proven track record
4. **Performance Category:** Blake 8.1 vs. Minnaert 7.5—marginal (<4%) improvement; not worth complexity
5. **Specialist Consensus:** 26/32 specialists ranked Minnaert as best; 4 ranked Hybrid; 2 preferred Blake

### Implementation Plan
- **Owner:** Physics Lead (Fluid Dynamics Specialist)
- **Timeline:** Week 2–4 of Phase 3
- **Success Metric:** Minnaert-predicted radius error <5% vs. optical particle sizing (ultrasonic camera)
- **Rollback Plan:** If experimental error >10%, switch to Blake; estimated 1 week rework

### Actual Outcome (Post-Implementation)
- **Predicted Minnaert Error:** <5%
- **Actual Error (Phase 3 testing):** 3.2% ± 0.8% (across 20–80 kHz, seawater @ 35 PSU)
- **Learnings:** Minnaert validated across our operating envelope; salinity correction factor (0.97 for seawater) held true
- **Adjustments:** None needed; proceed as planned

---

## Existing Decision Log Entries

### Phase 1: Sensor Placement (Monitoring)
**Date:** 2026-07-28  
- **Decision:** Single-point sensor (YSI ProDSS) at mid-tank depth
- **Score:** 7.1/10 (tied with redundant-sensor approach)
- **Rationale:** Cost & operational simplicity override redundancy; deploy second sensor in Phase 11 if field data warrants
- **Owner:** Environmental Lead

### Phase 3: Stabilizer Selection (SDS vs. Saponin vs. Tween-80)
**Date:** 2026-07-28  
- **Decision:** Sodium Dodecyl Sulfate (SDS) @ 0.5–2% by mass
- **Score:** 7.8/10
- **Rationale:** Best balance of cost, toxicity, bubble persistence, and field sourcing
- **Owner:** Fluid Dynamics Specialist

### Phase 4: Scan Pattern (Raster vs. Z-Order vs. Proximity)
**Date:** 2026-07-28  
- **Decision:** Z-Order (Morton) curve for galvo deflection
- **Score:** 7.9/10
- **Rationale:** Minimizes galvo settling time jitter; standard in graphics rendering
- **Owner:** Optics Engineer

*(Full decision log to be maintained in `decisions/DECISION_LOG.md`)*

---

## Escalation Path

If decision score is **<6.5** OR specialist disagreement exceeds 2 σ in any category:

1. **Project Lead Review** (24 hours)
   - Re-examine top 2–3 candidates
   - Request clarifying data from lowest-consensus category
   
2. **Steering Committee** (48 hours)
   - Convene domain leads (Optics, Acoustics, Marine Ops, Software)
   - Discuss risk trade-offs; vote on tiebreaker
   - Document dissent and proceed with highest consensus

3. **External Expert Consultation** (1 week)
   - If still tied, invite external advisor (e.g., university collaborator, vendor CTO)
   - Conduct rapid evaluation using same 48-parameter matrix
   - Make final call based on 3-person advisory board (internal lead + 2 external)

---

## Living Document Governance

This `claude.md` file is the **single source of truth** for decision-making in this project. It will be:
- **Reviewed quarterly** to add new decision entries and emerging best practices
- **Updated before each phase kickoff** to ensure specialists understand framework
- **Audited annually** to assess whether predicted scores matched actual outcomes

All commits modifying this file require **project lead approval** and must include a rationale comment.

---

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2026-07-28 | Initial 48-parameter consensus framework; seeded with 4 Phase 1–4 decisions |

