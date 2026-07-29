#!/usr/bin/env python3
"""Fan-out + QSPI routing generator for STM32H745 Ballistic Corrector.

Emits copper (segments + vias) into the .kicad_pcb between marker comments:
  - Full QSPI routing U1 <-> U6 (CLK/CS on F.Cu, D0-D3 on B.Cu over
    continuous pours; 0.28 mm = 50R on this stackup)
  - LQFP144 power fan-out: 20 VDD/VSS stub+via pairs to In1/In2 planes
    (vias tucked inside the package body where escape corridors are busy)
  - Decoupling cap, pull-up and U6/U7 power pin via drops
  - Test point plane ties

Before writing, runs a geometric self-check ("blind zone" audit):
  1. same-layer segment/segment crossings (different nets)
  2. segment vs via clearance          (different nets)
  3. via vs via clearance              (different nets)
  4. segment vs existing footprint pads (different nets, per layer)
  5. via vs existing footprint pads     (different nets)
Aborts without modifying the board if any violation is found.

Prints the QSPI length table for the tuning pass.
"""

import math
import re
import sys
import uuid
from pathlib import Path

PCB = Path(__file__).resolve().parent.parent / "hardware" / "STM32H745_Ballistic_Corrector.kicad_pcb"

BEGIN = "  ;; BEGIN GENERATED ROUTING (tools/gen_pcb_routing.py)"
END = "  ;; END GENERATED ROUTING"

NETS = {
    "GND": 1, "VCC_3V3": 2, "VCC_1V8": 3, "VCC_5V": 4, "VCC_12V": 5,
    "QSPI_CLK": 6, "QSPI_D0": 7, "QSPI_D1": 8, "QSPI_D2": 9, "QSPI_D3": 10,
    "QSPI_CS": 11, "SPI1_CLK": 12, "SPI1_MOSI": 13, "SPI1_MISO": 14,
    "SPI1_CS_IMU": 15, "I2C1_SCL": 16, "I2C1_SDA": 17, "USB_DP": 18,
    "USB_DM": 19, "UART2_TX": 20, "UART2_RX": 21, "NRST": 22, "BOOT0": 23,
}

W_QSPI = 0.28   # 50R single-ended on L1/L4 over adjacent plane
W_STUB = 0.3    # LQFP pad-width fan-out stubs
W_AUX = 0.4     # decap / passive via drops
VIA_D, VIA_DRILL = 0.6, 0.3
CLR = 0.15      # default copper clearance
CLR_QSPI = 0.2

SEGS = []  # (net, layer, width, (x1,y1), (x2,y2))
VIAS = []  # (net, (x,y))


def poly(net, layer, width, pts):
    for a, b in zip(pts, pts[1:]):
        SEGS.append((net, layer, width, a, b))


def via(net, x, y):
    VIAS.append((net, (x, y)))


# ===========================================================================
# QSPI: U1 (90,78) <-> U6 W25Q128JV (114,71)
#   CLK, CS  : F.Cu only
#   D0..D3   : F.Cu escape stub -> B.Cu corridor -> F.Cu entry stub
# ===========================================================================

def route_qspi():
    # CS: PB6 pin136 top -> bus above MCU -> U6 pin1
    poly("QSPI_CS", "F.Cu", W_QSPI,
         [(85.25, 67.15), (85.25, 65.9), (108.6, 65.9), (108.6, 69.095), (110.3, 69.095)])

    # CLK: PB2 pin48 bottom -> south bus -> east riser -> U6 pin6
    poly("QSPI_CLK", "F.Cu", W_QSPI,
         [(86.75, 88.85), (86.75, 90.6), (118.9, 90.6), (118.9, 71.635), (117.7, 71.635)])

    # D0: PD11 pin80 right -> B.Cu south-east corridor -> U6 pin5 (from below)
    poly("QSPI_D0", "F.Cu", W_QSPI,
         [(100.85, 83.25), (101.9, 83.25), (102.45, 83.8)])
    via("QSPI_D0", 102.45, 83.8)
    poly("QSPI_D0", "B.Cu", W_QSPI,
         [(102.45, 83.8), (102.45, 85.6), (117.7, 85.6), (117.7, 74.6)])
    via("QSPI_D0", 117.7, 74.6)
    poly("QSPI_D0", "F.Cu", W_QSPI, [(117.7, 74.6), (117.7, 72.905)])

    # D1: PD12 pin81 right -> B.Cu west riser -> U6 pin2 (from west)
    poly("QSPI_D1", "F.Cu", W_QSPI, [(100.85, 82.75), (103.4, 82.75)])
    via("QSPI_D1", 103.4, 82.75)
    poly("QSPI_D1", "B.Cu", W_QSPI,
         [(103.4, 82.75), (101.5, 82.75), (101.5, 70.365), (108.5, 70.365)])
    via("QSPI_D1", 108.5, 70.365)
    poly("QSPI_D1", "F.Cu", W_QSPI, [(108.5, 70.365), (110.3, 70.365)])

    # D2: PE2 pin1 left -> B.Cu north corridor -> drop under U6 body -> pin3
    poly("QSPI_D2", "F.Cu", W_QSPI, [(79.15, 69.25), (77.6, 69.25)])
    via("QSPI_D2", 77.6, 69.25)
    poly("QSPI_D2", "B.Cu", W_QSPI,
         [(77.6, 69.25), (77.6, 66.0), (113.0, 66.0), (113.0, 71.635), (111.9, 71.635)])
    via("QSPI_D2", 111.9, 71.635)
    poly("QSPI_D2", "F.Cu", W_QSPI, [(111.9, 71.635), (110.3, 71.635)])

    # D3: PD13 pin82 right -> B.Cu mid corridor -> east riser -> U6 pin7
    poly("QSPI_D3", "F.Cu", W_QSPI,
         [(100.85, 82.25), (101.7, 82.25), (102.3, 81.65)])
    via("QSPI_D3", 102.3, 81.65)
    poly("QSPI_D3", "B.Cu", W_QSPI,
         [(102.3, 81.65), (102.3, 73.8), (119.3, 73.8), (119.3, 70.365)])
    via("QSPI_D3", 119.3, 70.365)
    poly("QSPI_D3", "F.Cu", W_QSPI, [(119.3, 70.365), (117.7, 70.365)])


# ===========================================================================
# LQFP144 power fan-out. U1 at (90,78); body x 80..100, y 68..88.
# Escape corridors (QSPI buses) block outward stubs on N/S/E sides, so
# power vias are tucked INSIDE the package body (tented via note in fab doc).
# ===========================================================================

def route_power_fanout():
    # --- top side (pads at y=67.15): VSS diag to y=68.9, VDD straight y=69.7
    for x in (81.75, 88.25, 93.25):        # VSS 143, 130, 120
        poly("GND", "F.Cu", W_STUB, [(x, 67.15), (x, 68.2), (x + 0.55, 68.9)])
        via("GND", x + 0.55, 68.9)
    for x in (81.25, 87.75, 92.75):        # VDD 144, 131, 121
        poly("VCC_3V3", "F.Cu", W_STUB, [(x, 67.15), (x, 69.7)])
        via("VCC_3V3", x, 69.7)

    # --- bottom side (pads at y=88.85): VSS diag to y=87.1, VDD straight y=86.0
    for x in (81.75, 88.25, 93.25):        # VSS 38, 51, 61
        poly("GND", "F.Cu", W_STUB, [(x, 88.85), (x, 87.8), (x - 0.55, 87.1)])
        via("GND", x - 0.55, 87.1)
    for x in (82.25, 88.75, 93.75, 98.75):  # VDD 39, 52, 62, 72
        poly("VCC_3V3", "F.Cu", W_STUB, [(x, 88.85), (x, 86.0)])
        via("VCC_3V3", x, 86.0)

    # --- right side (pads at x=100.85): tuck inward, QSPI owns the outside
    for y in (81.75, 76.25, 69.75):        # VSS 83, 94, 107
        poly("GND", "F.Cu", W_STUB, [(100.85, y), (99.0, y)])
        via("GND", 99.0, y)
    for y in (81.25, 75.75, 69.25):        # VDD 84, 95, 108
        poly("VCC_3V3", "F.Cu", W_STUB, [(100.85, y), (100.0, y), (99.0, y - 0.8)])
        via("VCC_3V3", 99.0, y - 0.8)

    # --- left side: VSS16 straight inward, VDD17 diagonal
    poly("GND", "F.Cu", W_STUB, [(79.15, 76.75), (81.0, 76.75)])
    via("GND", 81.0, 76.75)
    poly("VCC_3V3", "F.Cu", W_STUB, [(79.15, 77.25), (80.0, 77.25), (81.0, 78.05)])
    via("VCC_3V3", 81.0, 78.05)


# ===========================================================================
# Passive / memory power via drops
# ===========================================================================

def route_via_drops():
    # MCU decoupling ring: C3-C5 (north, vias above), pads at y=65
    for cx in (82, 90, 98):
        poly("VCC_3V3", "F.Cu", W_AUX, [(cx - 0.51, 65), (cx - 0.51, 63.9)])
        via("VCC_3V3", cx - 0.51, 63.9)
        poly("GND", "F.Cu", W_AUX, [(cx + 0.51, 65), (cx + 0.51, 63.9)])
        via("GND", cx + 0.51, 63.9)

    # C6 (103.6,72): 3V3 pad down-drop, GND pad east drop
    poly("VCC_3V3", "F.Cu", W_AUX, [(103.09, 72), (102.2, 72)])
    via("VCC_3V3", 102.2, 72)
    poly("GND", "F.Cu", W_AUX, [(104.11, 72), (105.21, 72)])
    via("GND", 105.21, 72)

    # C7 (103.6,86.6): 3V3 south drop, GND east drop (clears D0 B.Cu corridor)
    poly("VCC_3V3", "F.Cu", W_AUX, [(103.09, 86.6), (103.09, 87.6)])
    via("VCC_3V3", 103.09, 87.6)
    poly("GND", "F.Cu", W_AUX, [(104.11, 86.6), (105.21, 86.6)])
    via("GND", 105.21, 86.6)

    # C8 (98,91.5): 3V3 south drop, GND east drop
    poly("VCC_3V3", "F.Cu", W_AUX, [(97.49, 91.5), (97.49, 92.7)])
    via("VCC_3V3", 97.49, 92.7)
    poly("GND", "F.Cu", W_AUX, [(98.51, 91.5), (99.61, 91.5)])
    via("GND", 99.61, 91.5)

    # C9 (76.5,72): 3V3 west, GND south (clears D2 escape via)
    poly("VCC_3V3", "F.Cu", W_AUX, [(75.99, 72), (74.9, 72)])
    via("VCC_3V3", 74.9, 72)
    poly("GND", "F.Cu", W_AUX, [(77.01, 72), (77.01, 70.8)])
    via("GND", 77.01, 70.8)

    # C10 (76.5,84): 3V3 west, GND north
    poly("VCC_3V3", "F.Cu", W_AUX, [(75.99, 84), (74.9, 84)])
    via("VCC_3V3", 74.9, 84)
    poly("GND", "F.Cu", W_AUX, [(77.01, 84), (77.01, 85.2)])
    via("GND", 77.01, 85.2)

    # C11 bulk (103.6,78): 3V3 diagonal north-west, GND east
    poly("VCC_3V3", "F.Cu", W_AUX, [(102.65, 78), (103.0, 79.5)])
    via("VCC_3V3", 103.0, 79.5)
    poly("GND", "F.Cu", W_AUX, [(104.55, 78), (105.5, 78)])
    via("GND", 105.5, 78)

    # U6 flash power: pin8 VCC north drop, pin4 GND south drop
    poly("VCC_3V3", "F.Cu", W_AUX, [(117.7, 69.095), (117.7, 67.6)])
    via("VCC_3V3", 117.7, 67.6)
    poly("GND", "F.Cu", W_AUX, [(110.3, 72.905), (110.3, 74.9)])
    via("GND", 110.3, 74.9)

    # C16 flash decap (120.5,68): both pads north drops
    poly("VCC_3V3", "F.Cu", W_AUX, [(119.99, 68), (119.99, 66.8)])
    via("VCC_3V3", 119.99, 66.8)
    poly("GND", "F.Cu", W_AUX, [(121.01, 68), (121.01, 66.8)])
    via("GND", 121.01, 66.8)

    # U7 EEPROM power: pin8 VCC east drop, pin4 GND west drop
    poly("VCC_3V3", "F.Cu", W_AUX, [(116.7, 84.095), (116.7, 82.6)])
    via("VCC_3V3", 116.7, 82.6)
    poly("GND", "F.Cu", W_AUX, [(111.3, 87.905), (109.8, 87.905)])
    via("GND", 109.8, 87.905)

    # C17 EEPROM decap (120.5,83): both pads north drops
    poly("VCC_3V3", "F.Cu", W_AUX, [(119.99, 83), (119.99, 81.8)])
    via("VCC_3V3", 119.99, 81.8)
    poly("GND", "F.Cu", W_AUX, [(121.01, 83), (121.01, 81.8)])
    via("GND", 121.01, 81.8)

    # C18 IMU decap (84.5,101): west / east drops
    poly("VCC_3V3", "F.Cu", W_AUX, [(83.99, 101), (82.89, 101)])
    via("VCC_3V3", 82.89, 101)
    poly("GND", "F.Cu", W_AUX, [(85.01, 101), (86.11, 101)])
    via("GND", 86.11, 101)

    # I2C pull-up supply drops: R2/R3 3V3 pads south
    poly("VCC_3V3", "F.Cu", W_AUX, [(98.49, 92.5), (98.49, 93.7)])
    via("VCC_3V3", 98.49, 93.7)
    poly("VCC_3V3", "F.Cu", W_AUX, [(100.99, 92.5), (100.99, 93.7)])
    via("VCC_3V3", 100.99, 93.7)

    # Test point plane ties: TP5/TP6 GND, TP2 5V (In2 north pour)
    poly("GND", "F.Cu", W_AUX, [(118, 58.5), (118, 59.7)])
    via("GND", 118, 59.7)
    poly("GND", "F.Cu", W_AUX, [(121, 58.5), (121, 59.7)])
    via("GND", 121, 59.7)
    poly("VCC_5V", "F.Cu", W_AUX, [(118, 54.5), (118, 53.3)])
    via("VCC_5V", 118, 53.3)


# ===========================================================================
# Geometric self-check ("blind zone" audit)
# ===========================================================================

def seg_seg_cross(a1, a2, b1, b2):
    """True if segments properly intersect or overlap."""
    def orient(p, q, r):
        v = (q[0] - p[0]) * (r[1] - p[1]) - (q[1] - p[1]) * (r[0] - p[0])
        return 0 if abs(v) < 1e-9 else (1 if v > 0 else -1)
    o1, o2 = orient(a1, a2, b1), orient(a1, a2, b2)
    o3, o4 = orient(b1, b2, a1), orient(b1, b2, a2)
    if o1 != o2 and o3 != o4:
        return True
    return False


def pt_seg_dist(p, a, b):
    ax, ay = a
    bx, by = b
    px, py = p
    dx, dy = bx - ax, by - ay
    ll = dx * dx + dy * dy
    if ll < 1e-12:
        return math.hypot(px - ax, py - ay)
    t = max(0.0, min(1.0, ((px - ax) * dx + (py - ay) * dy) / ll))
    return math.hypot(px - (ax + t * dx), py - (ay + t * dy))


def seg_seg_dist(a1, a2, b1, b2):
    if seg_seg_cross(a1, a2, b1, b2):
        return 0.0
    return min(pt_seg_dist(a1, b1, b2), pt_seg_dist(a2, b1, b2),
               pt_seg_dist(b1, a1, a2), pt_seg_dist(b2, a1, a2))


def rect_pt_dist(p, cx, cy, hw, hh):
    dx = max(abs(p[0] - cx) - hw, 0.0)
    dy = max(abs(p[1] - cy) - hh, 0.0)
    return math.hypot(dx, dy)


def rect_seg_dist(a, b, cx, cy, hw, hh):
    # sample-based conservative distance segment<->axis-aligned rect
    n = max(2, int(math.hypot(b[0] - a[0], b[1] - a[1]) / 0.05))
    best = 1e9
    for i in range(n + 1):
        t = i / n
        p = (a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t)
        best = min(best, rect_pt_dist(p, cx, cy, hw, hh))
        if best == 0.0:
            break
    return best


def parse_pads():
    """Extract absolute pad rects: (net_name, layer_set, cx, cy, hw, hh)."""
    text = PCB.read_text()
    pads = []
    for m in re.finditer(
            r'\(footprint "[^"]+".*?\(at ([\d.]+) ([\d.]+)(?: [\d.-]+)?\)(.*?)\n  \)',
            text, re.S):
        fx, fy, body = float(m.group(1)), float(m.group(2)), m.group(3)
        for pm in re.finditer(
                r'\(pad "[^"]*" (smd|thru_hole|np_thru_hole) \w+ \(at ([-\d.]+) ([-\d.]+)\)'
                r' \(size ([\d.]+) ([\d.]+)\)[^\n]*', body):
            kind, px, py, w, h = pm.group(1), float(pm.group(2)), float(pm.group(3)), \
                float(pm.group(4)), float(pm.group(5))
            nm = re.search(r'\(net \d+ "([^"]*)"\)', pm.group(0))
            net = nm.group(1) if nm else ""
            layers = {"F.Cu", "B.Cu"} if kind != "smd" else {"F.Cu"}
            pads.append((net, layers, fx + px, fy + py, w / 2, h / 2))
    return pads


def audit():
    errors = []
    # 1. same-layer crossings
    for i in range(len(SEGS)):
        for j in range(i + 1, len(SEGS)):
            n1, l1, w1, a1, a2 = SEGS[i]
            n2, l2, w2, b1, b2 = SEGS[j]
            if n1 == n2 or l1 != l2:
                continue
            d = seg_seg_dist(a1, a2, b1, b2)
            need = w1 / 2 + w2 / 2 + CLR
            if d < need:
                errors.append(f"SEG-SEG {n1}/{n2} on {l1}: d={d:.3f} < {need:.3f} at {a1}->{a2} vs {b1}->{b2}")
    # 2. segment vs via (vias span both layers)
    for n1, l1, w1, a1, a2 in SEGS:
        for n2, (vx, vy) in VIAS:
            if n1 == n2:
                continue
            d = pt_seg_dist((vx, vy), a1, a2)
            need = w1 / 2 + VIA_D / 2 + CLR
            if d < need:
                errors.append(f"SEG-VIA {n1}/{n2}: d={d:.3f} < {need:.3f} seg {a1}->{a2} via ({vx},{vy})")
    # 3. via vs via
    for i in range(len(VIAS)):
        for j in range(i + 1, len(VIAS)):
            n1, (x1, y1) = VIAS[i]
            n2, (x2, y2) = VIAS[j]
            if n1 == n2:
                continue
            d = math.hypot(x2 - x1, y2 - y1)
            need = VIA_D + CLR
            if d < need:
                errors.append(f"VIA-VIA {n1}/{n2}: d={d:.3f} < {need:.3f} ({x1},{y1}) vs ({x2},{y2})")
    # 4/5. against existing pads
    pads = parse_pads()
    for net, layers, cx, cy, hw, hh in pads:
        for n1, l1, w1, a1, a2 in SEGS:
            if n1 == net or l1 not in layers:
                continue
            d = rect_seg_dist(a1, a2, cx, cy, hw, hh)
            need = w1 / 2 + CLR
            if d < need - 1e-6:
                errors.append(f"SEG-PAD {n1}/pad[{net or 'nc'}]@({cx:.2f},{cy:.2f}): d={d:.3f} < {need:.3f} seg {a1}->{a2}")
        for n2, (vx, vy) in VIAS:
            if n2 == net:
                continue
            d = rect_pt_dist((vx, vy), cx, cy, hw, hh)
            need = VIA_D / 2 + CLR
            if d < need - 1e-6:
                errors.append(f"VIA-PAD {n2}/pad[{net or 'nc'}]@({cx:.2f},{cy:.2f}): d={d:.3f} < {need:.3f} via ({vx},{vy})")
    return errors


def length_report():
    lens = {}
    for n, _, _, a, b in SEGS:
        if n.startswith("QSPI"):
            lens[n] = lens.get(n, 0.0) + math.hypot(b[0] - a[0], b[1] - a[1])
    return lens


# ===========================================================================
# Emit
# ===========================================================================

def uid(key):
    return str(uuid.uuid5(uuid.NAMESPACE_DNS, "vkte-route-" + key))


def emit():
    out = []
    for i, (net, layer, width, a, b) in enumerate(SEGS):
        out.append(
            f'  (segment (start {a[0]:g} {a[1]:g}) (end {b[0]:g} {b[1]:g})'
            f' (width {width:g}) (layer "{layer}") (net {NETS[net]})'
            f' (tstamp "{uid("s" + str(i))}"))')
    for i, (net, (x, y)) in enumerate(VIAS):
        out.append(
            f'  (via (at {x:g} {y:g}) (size {VIA_D:g}) (drill {VIA_DRILL:g})'
            f' (layers "F.Cu" "B.Cu") (net {NETS[net]})'
            f' (tstamp "{uid("v" + str(i))}"))')
    return "\n".join(out)


def main():
    route_qspi()
    route_power_fanout()
    route_via_drops()

    errors = audit()
    if errors:
        print(f"AUDIT FAILED: {len(errors)} violation(s)")
        for e in errors:
            print("  " + e)
        sys.exit(1)

    text = PCB.read_text()
    block = BEGIN + "\n" + emit() + "\n" + END
    if BEGIN in text:
        text = re.sub(re.escape(BEGIN) + r".*?" + re.escape(END),
                      lambda _: block, text, flags=re.S)
    else:
        idx = text.rstrip().rfind(")")
        text = text.rstrip()[:idx] + block + "\n)\n"
    PCB.write_text(text)

    bal = text.count("(") - text.count(")")
    print(f"OK: {len(SEGS)} segments, {len(VIAS)} vias, paren balance = {bal}")
    print("\nQSPI length table (mm):")
    lens = length_report()
    for n in sorted(lens):
        print(f"  {n:10s} {lens[n]:6.2f}")
    data = [v for k, v in lens.items() if k not in ("QSPI_CLK", "QSPI_CS")]
    print(f"  data skew: {max(data) - min(data):.2f} mm "
          f"(~{(max(data) - min(data)) * 6.5 / 1000:.3f} ns) -> tuning pass target ±0.5 mm")
    if bal != 0:
        sys.exit("ERROR: unbalanced s-expression")


if __name__ == "__main__":
    main()
