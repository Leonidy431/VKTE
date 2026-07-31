#!/usr/bin/env python3
"""
Generate the VKTE carrier-board and rear-panel KiCad PCB files.

The carrier hosts the standard boards selected in docs/HARDWARE_BOM.md and
docs/PCB_PINOUT_DESIGN.md:

- Raspberry Pi CM4 (2x Hirose DF40C-100DS mezzanine, 4x M2.5 standoffs
  on the official 48 x 33 mm hole pattern)
- 4x Steminc SMDRV1000-1 ultrasonic driver boards (off-carrier, JST-XH looms)
- Cambridge Tech 6215H servo driver (off-carrier, IDC loom, XY2-100/SPI)
- CNI MGL-III-532 laser PSU (off-carrier, 2x13 IDC control loom)
- DLP HUD projector (off-carrier, LVDS — placeholder, official footprint
  needed in Phase 4)

Connector complement and pin counts follow docs/PCB_PINOUT_DESIGN.md.

Mezzanine and LVDS connectors are placed as outline placeholders: the
official Hirose DF40 / JAE FI-X footprints must be pulled from vendor or
KiCad libraries during Phase 4 (detailed design) — hand-rolling 0.4 mm
pitch pads here would only invite fab errors.

Usage:  python3 generate_carrier.py   (writes ../vkte_carrier/*.kicad_pcb
                                       and ../vkte_panels/*.kicad_pcb)
"""

import os
import uuid

HERE = os.path.dirname(os.path.abspath(__file__))

# ---------------------------------------------------------------------------
# s-expression helpers
# ---------------------------------------------------------------------------


def u():
    return str(uuid.uuid4())


HEADER = """(kicad_pcb (version 20221018) (generator vkte_generate_carrier)

  (general
    (thickness 1.6)
  )

  (paper "A3")
  (title_block
    (title "{title}")
    (company "VKTE — Volga volumetric display")
    (rev "0.1")
    (comment 1 "{comment}")
  )

  (layers
    (0 "F.Cu" signal)
    (31 "B.Cu" signal)
    (32 "B.Adhes" user "B.Adhesive")
    (33 "F.Adhes" user "F.Adhesive")
    (34 "B.Paste" user)
    (35 "F.Paste" user)
    (36 "B.SilkS" user "B.Silkscreen")
    (37 "F.SilkS" user "F.Silkscreen")
    (38 "B.Mask" user)
    (39 "F.Mask" user)
    (40 "Dwgs.User" user "User.Drawings")
    (41 "Cmts.User" user "User.Comments")
    (44 "Edge.Cuts" user)
    (45 "Margin" user)
    (46 "B.CrtYd" user "B.Courtyard")
    (47 "F.CrtYd" user "F.Courtyard")
    (48 "B.Fab" user)
    (49 "F.Fab" user)
  )

  (setup
    (pad_to_mask_clearance 0)
    (pcbplotparams)
  )

  (net 0 "")
"""

FOOTER = ")\n"


def edge_rect(x1, y1, x2, y2, width=0.1):
    return (
        f'  (gr_rect (start {x1} {y1}) (end {x2} {y2})'
        f' (stroke (width {width}) (type default)) (layer "Edge.Cuts") (tstamp {u()}))\n'
    )


def edge_circle(cx, cy, r, width=0.1):
    return (
        f'  (gr_circle (center {cx} {cy}) (end {cx + r} {cy})'
        f' (stroke (width {width}) (type default)) (layer "Edge.Cuts") (tstamp {u()}))\n'
    )


def silk_text(text, x, y, size=1.5, layer="F.SilkS"):
    return (
        f'  (gr_text "{text}" (at {x} {y}) (layer "{layer}") (tstamp {u()})\n'
        f'    (effects (font (size {size} {size}) (thickness {size * 0.15:.2f})))\n'
        f"  )\n"
    )


def npth_hole(ref, x, y, drill):
    """Non-plated mounting hole as a footprint."""
    pad_size = drill
    return f"""  (footprint "VKTE:MountingHole_{drill}mm" (layer "F.Cu") (tstamp {u()})
    (at {x} {y})
    (attr exclude_from_pos_files exclude_from_bom)
    (fp_text reference "{ref}" (at 0 {-(drill / 2 + 1.2):.2f}) (layer "F.SilkS") (tstamp {u()})
      (effects (font (size 1 1) (thickness 0.15)))
    )
    (fp_text value "MountingHole" (at 0 {drill / 2 + 1.2:.2f}) (layer "F.Fab") (tstamp {u()})
      (effects (font (size 1 1) (thickness 0.15)))
    )
    (fp_circle (center 0 0) (end {drill / 2 + 0.25:.2f} 0)
      (stroke (width 0.15) (type default)) (layer "F.SilkS") (tstamp {u()}))
    (pad "" np_thru_hole circle (at 0 0) (size {pad_size} {pad_size})
      (drill {drill}) (layers "*.Cu" "*.Mask") (tstamp {u()}))
  )
"""


def tht_pad(num, x, y, size, drill, shape="circle"):
    return (
        f'    (pad "{num}" thru_hole {shape} (at {x:.2f} {y:.2f}) (size {size} {size})'
        f' (drill {drill}) (layers "*.Cu" "*.Mask") (tstamp {u()}))\n'
    )


def pin_header(ref, value, x, y, rows, cols, pitch=2.54, pad=1.7, drill=1.0):
    """rows x cols THT pin header / IDC footprint, pin 1 top-left, column-major."""
    body_w = (cols - 1) * pitch + pitch
    body_h = (rows - 1) * pitch + pitch
    fp = f"""  (footprint "VKTE:PinHeader_{rows}x{cols:02d}_P{pitch}mm" (layer "F.Cu") (tstamp {u()})
    (at {x} {y})
    (attr through_hole)
    (fp_text reference "{ref}" (at {body_w / 2 - pitch / 2:.2f} {-(body_h / 2 + 1.0):.2f}) (layer "F.SilkS") (tstamp {u()})
      (effects (font (size 1 1) (thickness 0.15)))
    )
    (fp_text value "{value}" (at {body_w / 2 - pitch / 2:.2f} {body_h + 1.0:.2f}) (layer "F.Fab") (tstamp {u()})
      (effects (font (size 1 1) (thickness 0.15)))
    )
    (fp_rect (start {-pitch / 2:.2f} {-pitch / 2:.2f}) (end {body_w - pitch / 2:.2f} {body_h - pitch / 2:.2f})
      (stroke (width 0.12) (type default)) (layer "F.SilkS") (tstamp {u()}))
"""
    n = 1
    for c in range(cols):
        for r in range(rows):
            shape = "rect" if n == 1 else "circle"
            fp += tht_pad(n, c * pitch, r * pitch, pad, drill, shape)
            n += 1
    fp += "  )\n"
    return fp


def xt30(ref, x, y):
    """XT30 power connector: 2 pads, 2.5 mm pitch 5.0, 1.8 mm drill."""
    return f"""  (footprint "VKTE:XT30_Vertical" (layer "F.Cu") (tstamp {u()})
    (at {x} {y})
    (attr through_hole)
    (fp_text reference "{ref}" (at 2.5 -4.5) (layer "F.SilkS") (tstamp {u()})
      (effects (font (size 1 1) (thickness 0.15)))
    )
    (fp_text value "XT30PW-M 12V IN" (at 2.5 4.5) (layer "F.Fab") (tstamp {u()})
      (effects (font (size 1 1) (thickness 0.15)))
    )
    (fp_rect (start -2.6 -3.1) (end 7.6 3.1)
      (stroke (width 0.12) (type default)) (layer "F.SilkS") (tstamp {u()}))
{tht_pad('1', 0, 0, 2.6, 1.8, 'rect')}{tht_pad('2', 5.0, 0, 2.6, 1.8)}  )
"""


def screw_terminal(ref, value, x, y, pins, pitch=5.08):
    fp = f"""  (footprint "VKTE:TerminalBlock_1x{pins:02d}_P{pitch}mm" (layer "F.Cu") (tstamp {u()})
    (at {x} {y})
    (attr through_hole)
    (fp_text reference "{ref}" (at {(pins - 1) * pitch / 2:.2f} -5.0) (layer "F.SilkS") (tstamp {u()})
      (effects (font (size 1 1) (thickness 0.15)))
    )
    (fp_text value "{value}" (at {(pins - 1) * pitch / 2:.2f} 5.0) (layer "F.Fab") (tstamp {u()})
      (effects (font (size 1 1) (thickness 0.15)))
    )
    (fp_rect (start {-pitch / 2:.2f} -3.8) (end {(pins - 1) * pitch + pitch / 2:.2f} 3.8)
      (stroke (width 0.12) (type default)) (layer "F.SilkS") (tstamp {u()}))
"""
    for n in range(pins):
        shape = "rect" if n == 0 else "circle"
        fp += tht_pad(n + 1, n * pitch, 0, 2.4, 1.3, shape)
    fp += "  )\n"
    return fp


def placeholder(ref, value, x, y, w, h, note):
    """Outline-only placeholder for a connector whose official footprint is a Phase 4 task."""
    return f"""  (footprint "VKTE:PLACEHOLDER_{ref}" (layer "F.Cu") (tstamp {u()})
    (at {x} {y})
    (attr exclude_from_pos_files)
    (fp_text reference "{ref}" (at 0 {-(h / 2 + 1.5):.2f}) (layer "F.SilkS") (tstamp {u()})
      (effects (font (size 1.2 1.2) (thickness 0.2)))
    )
    (fp_text value "{value}" (at 0 {h / 2 + 1.5:.2f}) (layer "F.Fab") (tstamp {u()})
      (effects (font (size 1 1) (thickness 0.15)))
    )
    (fp_text user "{note}" (at 0 0) (layer "F.Fab") (tstamp {u()})
      (effects (font (size 0.8 0.8) (thickness 0.12)))
    )
    (fp_rect (start {-w / 2:.2f} {-h / 2:.2f}) (end {w / 2:.2f} {h / 2:.2f})
      (stroke (width 0.2) (type default)) (layer "F.SilkS") (tstamp {u()}))
    (fp_rect (start {-w / 2:.2f} {-h / 2:.2f}) (end {w / 2:.2f} {h / 2:.2f})
      (stroke (width 0.05) (type default)) (layer "F.CrtYd") (tstamp {u()}))
  )
"""


# ---------------------------------------------------------------------------
# Carrier board (160 x 100 mm), origin of board at page (40, 40)
# ---------------------------------------------------------------------------


def build_carrier():
    ox, oy = 40.0, 40.0  # board top-left on the page
    w, h = 160.0, 100.0

    pcb = HEADER.format(
        title="VKTE carrier board — CM4 + subsystem looms",
        comment="Connector complement per docs/PCB_PINOUT_DESIGN.md",
    )
    pcb += edge_rect(ox, oy, ox + w, oy + h)

    # Board mounting holes: M3 (3.2 mm), 5 mm inset at corners
    for i, (mx, my) in enumerate(
        [(5, 5), (w - 5, 5), (5, h - 5), (w - 5, h - 5)], start=1
    ):
        pcb += npth_hole(f"H{i}", ox + mx, oy + my, 3.2)

    # --- CM4 area (module is 55 x 40; holes 48 x 33, 3.5 mm inset) -----------
    cm4_x, cm4_y = ox + 12.0, oy + 12.0  # module top-left
    for i, (mx, my) in enumerate(
        [(3.5, 3.5), (3.5 + 48, 3.5), (3.5, 3.5 + 33), (3.5 + 48, 3.5 + 33)],
        start=5,
    ):
        pcb += npth_hole(f"H{i}", cm4_x + mx, cm4_y + my, 2.7)
    pcb += silk_text("RPi CM4", cm4_x + 27.5, cm4_y + 20, size=2.0)
    # Two DF40 100-pin mezzanine connectors (module-standard positions,
    # centered rows near the module's long edges)
    pcb += placeholder(
        "J1", "Hirose DF40C-100DS-0.4V", cm4_x + 27.5, cm4_y + 7.0, 24.0, 4.0,
        "Phase 4: official DF40 footprint",
    )
    pcb += placeholder(
        "J2", "Hirose DF40C-100DS-0.4V", cm4_x + 27.5, cm4_y + 33.0, 24.0, 4.0,
        "Phase 4: official DF40 footprint",
    )

    # --- Power input (left edge, below CM4) ---------------------------------
    pcb += xt30("J3", ox + 10.0, oy + 78.0)
    pcb += silk_text("12V 5A IN", ox + 12.5, oy + 86.0, size=1.2)

    # --- RS-485 telemetry (bottom edge) -------------------------------------
    pcb += screw_terminal("J4", "RS-485 A/B/GND/SHLD", ox + 38.0, oy + 88.0, 4)

    # --- Laser control loom: 2x13 IDC (per PCB_PINOUT_DESIGN 26-pin) --------
    pcb += pin_header("J5", "LASER CTRL IDC-26", ox + 92.0, oy + 12.0, 2, 13)
    pcb += silk_text("CNI MGL-III-532", ox + 107.0, oy + 24.0, size=1.2)

    # --- Galvo scanner loom: 2x8 IDC (SPI / XY2-100) ------------------------
    pcb += pin_header("J6", "GALVO XY2-100 IDC-16", ox + 92.0, oy + 34.0, 2, 8)
    pcb += silk_text("Cambridge 6215H", ox + 101.0, oy + 46.0, size=1.2)

    # --- Bubble driver looms: 4x JST-XH 1x6 (Steminc SMDRV1000-1) -----------
    for i in range(4):
        ref = f"J{7 + i}"
        pcb += pin_header(
            ref, f"BUBBLE DRV {i + 1} JST-XH", ox + 92.0 + i * 17.0, oy + 60.0,
            6, 1, pitch=2.5, pad=1.7, drill=1.0,
        )
    pcb += silk_text("Steminc SMDRV1000-1 x4", ox + 118.0, oy + 82.0, size=1.2)

    # --- HUD LVDS (54-pin) placeholder --------------------------------------
    pcb += placeholder(
        "J11", "HUD LVDS 54-pin (DLP TRP-4500)", ox + 45.0, oy + 65.0, 30.0, 6.0,
        "Phase 4: official LVDS footprint",
    )

    # --- Ambient light sensor I2C loom (Sprint 4 adaptive brightness) -------
    pcb += pin_header("J12", "I2C ALS JST-XH", ox + 70.0, oy + 88.0, 4, 1,
                      pitch=2.5, pad=1.7, drill=1.0)

    pcb += silk_text("VKTE CARRIER rev 0.1", ox + w / 2, oy + h - 3.0, size=1.5)
    pcb += FOOTER
    return pcb


# ---------------------------------------------------------------------------
# Rear panel (FR-4 panel trick): 180 x 60 mm with connector cutouts
# ---------------------------------------------------------------------------


def build_rear_panel():
    ox, oy = 40.0, 40.0
    w, h = 180.0, 60.0

    pcb = HEADER.format(
        title="VKTE enclosure rear panel — connector cutouts",
        comment="FR-4 2 mm panel; cutout sizes per vendor drawings, verify in Phase 5",
    )
    pcb += edge_rect(ox, oy, ox + w, oy + h)

    # Panel corner holes M3
    for i, (mx, my) in enumerate(
        [(5, 5), (w - 5, 5), (5, h - 5), (w - 5, h - 5)], start=1
    ):
        pcb += npth_hole(f"H{i}", ox + mx, oy + my, 3.2)

    # XT30 panel-mount cutout (approx 10.5 x 6.0)
    pcb += edge_rect(ox + 20.0, oy + 27.0, ox + 30.5, oy + 33.0)
    pcb += silk_text("12V IN (XT30-PB)", ox + 25.0, oy + 40.0, size=1.2)

    # DB9 cutout for RS-485 (standard: 19.2 x 10.7 + 2x 3.2 screw holes at 25.0)
    db9_cx = ox + 60.0
    pcb += edge_rect(db9_cx - 9.6, oy + 24.6, db9_cx + 9.6, oy + 35.3)
    pcb += npth_hole("H5", db9_cx - 12.5, oy + 30.0, 3.2)
    pcb += npth_hole("H6", db9_cx + 12.5, oy + 30.0, 3.2)
    pcb += silk_text("RS-485 (DB9)", db9_cx, oy + 42.0, size=1.2)

    # 2x M16 cable glands: laser + galvo looms (16.5 mm holes)
    for i, cx in enumerate([100.0, 125.0]):
        pcb += edge_circle(ox + cx, oy + 30.0, 16.5 / 2)
    pcb += silk_text("LASER / GALVO glands M16", ox + 112.0, oy + 48.0, size=1.2)

    # M20 cable gland: 4x bubble transducer loom (20.5 mm hole)
    pcb += edge_circle(ox + 155.0, oy + 30.0, 20.5 / 2)
    pcb += silk_text("BUBBLE M20", ox + 155.0, oy + 48.0, size=1.2)

    pcb += silk_text("VKTE REAR PANEL rev 0.1", ox + w / 2, oy + h - 3.0, size=1.5)
    pcb += FOOTER
    return pcb


def main():
    carrier_dir = os.path.join(HERE, "..", "vkte_carrier")
    panels_dir = os.path.join(HERE, "..", "vkte_panels")
    os.makedirs(carrier_dir, exist_ok=True)
    os.makedirs(panels_dir, exist_ok=True)

    with open(os.path.join(carrier_dir, "vkte_carrier.kicad_pcb"), "w") as f:
        f.write(build_carrier())
    with open(os.path.join(panels_dir, "vkte_rear_panel.kicad_pcb"), "w") as f:
        f.write(build_rear_panel())
    print("written: vkte_carrier.kicad_pcb, vkte_rear_panel.kicad_pcb")


if __name__ == "__main__":
    main()
