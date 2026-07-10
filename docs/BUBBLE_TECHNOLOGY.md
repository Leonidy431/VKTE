# Bubble Technology: Acoustic Microbubble Volumetric Rendering

Detailed technical reference for the light-scattering medium used by the
volumetric display described in `VOLUMETRIC_DISPLAY_SPECIFICATION.md`.
Implementation lives in `backend/app/modules/volumetric/bubble_generator.py`.

---

## 1. Why bubbles

A volumetric display needs a physical scattering point at every addressable
(x, y, z) location, dense enough to reconstruct a shape but sparse enough
that the laser can still reach deeper voxels. Three practical media exist:

| Medium | Scattering mechanism | Works in water? |
|---|---|---|
| Fog / smoke droplets | Mie scattering off liquid aerosol | No — dissipates in liquid |
| Dust / Perlin particles | Mie scattering off solid particulates | Marginal — settles, fouls |
| **Microbubbles** | Mie + acoustic resonance scattering off gas-liquid interface | **Yes** — this document |

Gas bubbles in liquid have a huge scattering cross-section relative to their
size because the refractive index step at a gas/liquid interface (n≈1.0 vs
n≈1.33) is much larger than at a liquid/liquid or solid/liquid interface.
A 50 µm air bubble in water scatters visible light roughly two orders of
magnitude more strongly than a similarly sized solid particle of comparable
refractive index contrast, which is what makes sparse bubble fields visible
at demonstrator brightness levels (a few watts of laser power over a
resonator column, not a museum-grade multi-kilowatt rig).

---

## 2. Bubble generation methods

### 2.1 Acoustic cavitation (primary method used in this project)

A piezoelectric transducer array drives the water column at 20–80 kHz.
Above the cavitation threshold, dissolved gas nucleates into bubbles at
existing nucleation sites (microscopic gas pockets on particulates, tank
walls, or seeded nuclei). Two cavitation regimes matter here:

- **Stable cavitation**: bubbles oscillate at the driving frequency without
  collapsing violently. This is the regime the display operates in —
  bubbles persist long enough to be scanned by the laser across a frame.
- **Inertial (transient) cavitation**: bubbles grow and violently collapse,
  producing shock waves, sonoluminescence, and free radicals. This regime
  is avoided — it's noisy, erosive to the transducer face, and produces
  bubbles too short-lived and irregular for repeatable voxel rendering.

The transducer duty cycle (fraction of time actively driving) is the primary
throttle for bubble *density*; the drive *frequency* sets the resonant
bubble *size* (see §3). `BubbleGenerator._solve_duty_cycle()` implements
this density mapping, corrected for how much acoustic energy a given
salinity and stabilizer concentration requires (§4).

### 2.2 Electrolytic generation (alternative, not used here)

Applying a DC/AC voltage across submerged electrodes electrolyzes water into
H₂ and O₂ microbubbles. Produces very uniform, small (5–20 µm) bubbles but:
- requires continuous power scaling with bubble volume (energy-inefficient
  versus acoustic drive for large fields),
- produces flammable H₂ at scale (safety concern in an enclosed display),
- corrodes electrodes faster in seawater due to chloride ion attack.

Included here as a documented alternative for future dense small-voxel
variants, not the primary generation path.

### 2.3 Microfluidic / needle injection (alternative, not used here)

Forcing gas through a fine nozzle or porous frit produces bubbles whose size
is set mechanically by orifice diameter and gas flow rate. Excellent size
control but mechanically complex to scale to a full 3D field (would need a
nozzle array spanning the tank volume) and doesn't integrate with the
laser's own timing the way acoustic drive does — hence not the chosen
method, but a candidate for very-high-resolution tabletop units.

---

## 3. Bubble size: Minnaert resonance

The bubble size that responds most strongly (oscillates with largest
amplitude, and therefore scatters the most light) at a given acoustic drive
frequency is the **Minnaert resonance radius**:

```
r0 = (1 / (2π·f)) · sqrt(3·γ·P0 / ρ)
```

Where:
- `f` — acoustic drive frequency (Hz)
- `γ` — polytropic index of the gas (≈1.4 for air)
- `P0` — ambient (hydrostatic) pressure (Pa)
- `ρ` — density of the surrounding liquid (kg/m³)

Implemented in `BubbleGenerator.resonant_bubble_radius_microns()`. Because
resonant radius is inversely proportional to frequency, the drive frequency
is the *primary* dial for bubble size:

| Drive frequency | Resonant radius (seawater, 1 atm) |
|---|---|
| 20 kHz | ≈ 164 µm |
| 40 kHz | ≈ 82 µm |
| 80 kHz | ≈ 41 µm |

Smaller bubbles (higher frequency) give finer voxel granularity and settle/
rise more slowly (better frame persistence — see §5), at the cost of
weaker per-bubble scattering, so higher frequency operation needs either
higher bubble density or more laser power per voxel to stay visible.

---

## 4. Adapting bubble generation to seawater

Seawater differs from fresh water in three properties that directly affect
bubble nucleation, stability, and optical transmission:

| Property | Fresh water (20°C) | Seawater (35 PSU, 20°C) | Effect |
|---|---|---|---|
| Surface tension | 0.0728 N/m | 0.0679 N/m | Lower tension → bubbles nucleate more easily, but coalesce/collapse faster without a stabilizer |
| Density | 998 kg/m³ | 1025 kg/m³ | Shifts Minnaert resonance slightly (§3 formula) |
| Sound speed | ≈1481 m/s | ≈1500 m/s | Minor shift in transducer/tank acoustic resonance tuning |
| Optical transmission | Broad visible transmission | Blue-green window (450–490 nm) transmits best; red is absorbed within cm | Laser wavelength must be chosen for seawater, not air |

### 4.1 Nucleation and duty-cycle correction

Because seawater's lower surface tension makes cavitation nucleation
*easier* at a given acoustic amplitude, the transducer duty cycle required
to reach a target bubble density is *lower* than in fresh water at the same
frequency and target density. `BubbleGenerator._solve_duty_cycle()` applies
an inverse-salinity correction term (`REFERENCE_SALINITY_PSU / salinity`)
so operators can dial in "seawater" or "freshwater" mode and get consistent
apparent bubble density without manually re-tuning duty cycle.

### 4.2 Stabilization against coalescence

Lower surface tension also means seawater microbubbles coalesce and pop
faster than freshwater ones once formed — they need to survive from
nucleation through the laser's scan of that frame (tens of milliseconds).
A small concentration (`stabilizer_concentration_pct`, typically
0.02–0.1%) of a food-safe surfactant (e.g., sodium dodecyl sulfate, SDS, or
a saponin-based natural surfactant for exhibits open to the public) coats
the gas-liquid interface and lowers the coalescence rate by electrostatic
and steric repulsion between neighboring bubble surfaces. This is the same
stabilization principle used in industrial microbubble generators for
water treatment and food-grade foams.

### 4.3 Optical wavelength selection

Seawater strongly absorbs red and IR light within centimeters (chlorophyll
and dissolved organic matter absorb blue and red, leaving a blue-green
transmission window). `LaserController.is_wavelength_seawater_optimal()`
flags whether the configured wavelength falls in the
**450–490 nm** window; the reference hardware pick (§ Hardware BOM,
532 nm frequency-doubled Nd:YAG) sits just above this window as a practical
compromise between seawater transmission and mature, affordable laser
supply — a 473 nm DPSS laser is the tighter-optimized alternative for tanks
deeper than ~1 m.

---

## 5. Voxel persistence and frame timing

A microbubble is not a static pixel — it rises (buoyancy), drifts
(residual acoustic streaming / convection), and eventually pops or
coalesces. The render loop must treat each bubble field as a *transient*
canvas, not a persistent frame buffer:

- **Rise velocity** (Stokes' law regime, bubbles <100 µm): a 50 µm air
  bubble in seawater rises at roughly 0.1–0.3 mm/s — slow enough that a
  bubble stays within one voxel's spatial tolerance (a few mm at 100³
  grid resolution over a ~30 cm tank) for **100–500 ms**, which is the
  `VoxelFrame` lifetime budget the render loop assumes.
- **Refresh strategy**: because bubbles don't hold position indefinitely,
  the transducer continuously nucleates new bubbles at the target density
  (§2.1) while the laser re-scans the same voxel set every frame — the
  *field* is persistent even though no single bubble is. This is why
  `BubbleGenerator.start()` and the laser's per-frame `scan_voxels()` run
  as two independent, continuously-cycling processes rather than a single
  "draw once" operation.
- **Frame rate implication**: the render loop tick (`VolumetricDisplayService`,
  default 5 Hz / 200 ms) is deliberately kept within the 100–500 ms bubble
  lifetime window so that each new frame's voxels are populated with fresh
  bubbles rather than fighting stale ones from the previous frame.

---

## 6. Practical calibration checklist

When commissioning a physical rig (see `docs/HARDWARE_BOM.md` for parts):

1. Measure actual salinity/temperature of the working fluid; feed into
   `BubbleParameters.salinity_psu` so density/duty-cycle corrections are
   accurate rather than assuming the 35 PSU default.
2. Sweep drive frequency 20→80 kHz at fixed duty cycle and visually confirm
   bubble field brightness peaks near the Minnaert-predicted resonance —
   confirms transducer coupling and tank acoustic Q are as modeled.
3. Titrate stabilizer concentration upward from 0% until bubble field
   visibly stops "sparkling and vanishing" within one frame period —
   that's the minimum viable concentration; going further just wastes
   surfactant and, at public-facing exhibits, affects water clarity/foam.
4. Confirm laser wavelength against `is_wavelength_seawater_optimal()`
   before final tank fill, since re-optically-aligning after the tank is
   sealed and salted is far more expensive than checking on the bench.
