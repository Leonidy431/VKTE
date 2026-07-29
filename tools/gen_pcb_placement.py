#!/usr/bin/env python3
"""Generate component footprints + placement for STM32H745 Ballistic Corrector.

Emits placement-grade footprints (generic package outlines) into the
.kicad_pcb between marker comments, per floorplan HLD_BOARD_INTEGRATION.md §5.3.
Idempotent: re-running replaces the generated block.

Footprints are skeletons for placement/floorplan work; each package must be
verified against its datasheet before Gerber lock (see PCB work order step 8).
Net bindings are provisional and will be overwritten by netlist import.
"""

import re
import sys
import uuid
from pathlib import Path

PCB = Path(__file__).resolve().parent.parent / "hardware" / "STM32H745_Ballistic_Corrector.kicad_pcb"

BEGIN = "  ;; BEGIN GENERATED PLACEMENT (tools/gen_pcb_placement.py)"
END = "  ;; END GENERATED PLACEMENT"

# Net numbers must match (net N "NAME") entries already present in the PCB file.
NETS = {
    "GND": 1, "VCC_3V3": 2, "VCC_1V8": 3, "VCC_5V": 4, "VCC_12V": 5,
    "QSPI_CLK": 6, "QSPI_D0": 7, "QSPI_D1": 8, "QSPI_D2": 9, "QSPI_D3": 10,
    "QSPI_CS": 11, "SPI1_CLK": 12, "SPI1_MOSI": 13, "SPI1_MISO": 14,
    "SPI1_CS_IMU": 15, "I2C1_SCL": 16, "I2C1_SDA": 17, "USB_DP": 18,
    "USB_DM": 19, "UART2_TX": 20, "UART2_RX": 21, "NRST": 22, "BOOT0": 23,
}


def uid(key: str) -> str:
    return str(uuid.uuid5(uuid.NAMESPACE_DNS, "vkte-pcb-" + key))


def net_expr(net):
    if not net:
        return ""
    return f' (net {NETS[net]} "{net}")'


def smd_pad(ref, num, x, y, w, h, net=None, shape="roundrect"):
    rr = " (roundrect_rratio 0.25)" if shape == "roundrect" else ""
    return (f'    (pad "{num}" smd {shape} (at {x:g} {y:g}) (size {w:g} {h:g})'
            f' (layers "F.Cu" "F.Paste" "F.Mask"){rr}{net_expr(net)}'
            f' (tstamp "{uid(ref + "-p" + str(num) + str(x) + str(y))}"))')


def th_pad(ref, num, x, y, size, drill, net=None, shape="circle"):
    return (f'    (pad "{num}" thru_hole {shape} (at {x:g} {y:g}) (size {size:g} {size:g})'
            f' (drill {drill:g}) (layers "*.Cu" "*.Mask"){net_expr(net)}'
            f' (tstamp "{uid(ref + "-p" + str(num) + str(x) + str(y))}"))')


def footprint(lib, ref, val, x, y, pads, body_w, body_h, desc="", th=False, rot=0):
    attr = "through_hole" if th else "smd"
    cw, ch = body_w / 2 + 0.5, body_h / 2 + 0.5
    lines = [
        f'  (footprint "{lib}" (layer "F.Cu") (tstamp "{uid(ref)}") (at {x:g} {y:g} {rot:g})',
        f'    (descr "{desc}")',
        f'    (attr {attr})',
        f'    (fp_text reference "{ref}" (at 0 {-(body_h / 2 + 1.3):g}) (layer "F.SilkS")',
        f'      (effects (font (size 0.9 0.9) (thickness 0.15))) (tstamp "{uid(ref + "-ref")}"))',
        f'    (fp_text value "{val}" (at 0 {(body_h / 2 + 1.3):g}) (layer "F.Fab")',
        f'      (effects (font (size 0.9 0.9) (thickness 0.15))) (tstamp "{uid(ref + "-val")}"))',
        f'    (fp_rect (start {-body_w / 2:g} {-body_h / 2:g}) (end {body_w / 2:g} {body_h / 2:g})'
        f' (stroke (width 0.12) (type solid)) (fill none) (layer "F.Fab") (tstamp "{uid(ref + "-fab")}"))',
        f'    (fp_rect (start {-cw:g} {-ch:g}) (end {cw:g} {ch:g})'
        f' (stroke (width 0.05) (type solid)) (fill none) (layer "F.CrtYd") (tstamp "{uid(ref + "-cy")}"))',
    ]
    lines.extend(pads)
    lines.append("  )")
    return "\n".join(lines)


# ---------------------------------------------------------------------------
# Package generators (pads in footprint-local coordinates)
# ---------------------------------------------------------------------------

# STM32H745ZIT6 LQFP144 pin->net map (QSPI CS on PB6 per HLD fix).
# Pin numbers follow the standard STM32 LQFP144 arrangement; VERIFY against
# the STM32H745ZIT6 datasheet at netlist import (VCAP/analog pins left open).
U1_NETS = {
    1: "QSPI_D2",            # PE2 / QUADSPI_BK1_IO2
    16: "GND", 17: "VCC_3V3",
    25: "NRST",
    36: "UART2_TX", 37: "UART2_RX",      # PA2 / PA3
    38: "GND", 39: "VCC_3V3",
    41: "SPI1_CLK", 42: "SPI1_MISO", 43: "SPI1_MOSI",  # PA5-PA7
    46: "SPI1_CS_IMU",       # PB0
    48: "QSPI_CLK",          # PB2
    51: "GND", 52: "VCC_3V3", 61: "GND", 62: "VCC_3V3", 72: "VCC_3V3",
    80: "QSPI_D0", 81: "QSPI_D1", 82: "QSPI_D3",       # PD11-PD13
    83: "GND", 84: "VCC_3V3", 94: "GND", 95: "VCC_3V3",
    103: "USB_DM", 104: "USB_DP",        # PA11 / PA12
    107: "GND", 108: "VCC_3V3",
    120: "GND", 121: "VCC_3V3", 130: "GND", 131: "VCC_3V3",
    136: "QSPI_CS",          # PB6 / QUADSPI_BK1_NCS
    138: "BOOT0",
    139: "I2C1_SCL", 140: "I2C1_SDA",    # PB8 / PB9
    143: "GND", 144: "VCC_3V3",
}


def lqfp144(ref, nets=None):
    D, pitch, n = 10.85, 0.5, 36
    span0 = -(n - 1) / 2 * pitch  # -8.75
    nets = nets or {}
    pads = []
    for i in range(n):  # left, 1-36, top->bottom
        pads.append(smd_pad(ref, 1 + i, -D, span0 + i * pitch, 1.5, 0.3, nets.get(1 + i)))
    for i in range(n):  # bottom, 37-72, left->right
        pads.append(smd_pad(ref, 37 + i, span0 + i * pitch, D, 0.3, 1.5, nets.get(37 + i)))
    for i in range(n):  # right, 73-108, bottom->top
        pads.append(smd_pad(ref, 73 + i, D, -span0 - i * pitch, 1.5, 0.3, nets.get(73 + i)))
    for i in range(n):  # top, 109-144, right->left
        pads.append(smd_pad(ref, 109 + i, -span0 - i * pitch, -D, 0.3, 1.5, nets.get(109 + i)))
    return pads, 20, 20


def qfn(ref, n_side, pitch, D, pad_l, pad_w, ep, ep_net=None, nets=None):
    span0 = -(n_side - 1) / 2 * pitch
    nets = nets or {}
    pads = []
    num = 1
    for i in range(n_side):  # left, top->bottom
        pads.append(smd_pad(ref, num, -D, span0 + i * pitch, pad_l, pad_w, nets.get(num))); num += 1
    for i in range(n_side):  # bottom, left->right
        pads.append(smd_pad(ref, num, span0 + i * pitch, D, pad_w, pad_l, nets.get(num))); num += 1
    for i in range(n_side):  # right, bottom->top
        pads.append(smd_pad(ref, num, D, -span0 - i * pitch, pad_l, pad_w, nets.get(num))); num += 1
    for i in range(n_side):  # top, right->left
        pads.append(smd_pad(ref, num, -span0 - i * pitch, -D, pad_w, pad_l, nets.get(num))); num += 1
    pads.append(smd_pad(ref, num, 0, 0, ep, ep, ep_net, shape="rect"))
    body = (D - pad_l / 2) * 2 + 0.2
    return pads, round(body, 2), round(body, 2)


def soic8(ref, half_span, nets=None):
    nets = nets or {}
    ys = [-1.905, -0.635, 0.635, 1.905]
    pads = []
    for i, yy in enumerate(ys):
        pads.append(smd_pad(ref, 1 + i, -half_span, yy, 1.55, 0.6, nets.get(1 + i)))
    for i, yy in enumerate(reversed(ys)):
        pads.append(smd_pad(ref, 5 + i, half_span, yy, 1.55, 0.6, nets.get(5 + i)))
    return pads, (half_span - 1.0) * 2, 5.0


def msop8(ref, nets=None):
    nets = nets or {}
    ys = [-0.975, -0.325, 0.325, 0.975]
    pads = []
    for i, yy in enumerate(ys):
        pads.append(smd_pad(ref, 1 + i, -2.2, yy, 1.2, 0.4, nets.get(1 + i)))
    for i, yy in enumerate(reversed(ys)):
        pads.append(smd_pad(ref, 5 + i, 2.2, yy, 1.2, 0.4, nets.get(5 + i)))
    return pads, 3.0, 3.0


def sot23_5(ref, nets=None):
    nets = nets or {}
    pos = {1: (-0.95, 1.3), 2: (0, 1.3), 3: (0.95, 1.3), 4: (0.95, -1.3), 5: (-0.95, -1.3)}
    pads = [smd_pad(ref, n, xx, yy, 0.6, 1.2, nets.get(n)) for n, (xx, yy) in pos.items()]
    return pads, 2.9, 1.6


def sot223(ref, nets=None, tab_net=None):
    nets = nets or {}
    pads = [smd_pad(ref, n, (n - 2) * 2.3, 3.2, 1.2, 2.2, nets.get(n)) for n in (1, 2, 3)]
    pads.append(smd_pad(ref, 4, 0, -3.2, 3.6, 2.2, tab_net))
    return pads, 6.5, 3.5


def chip2(ref, half, w, h, net1=None, net2=None):
    return ([smd_pad(ref, 1, -half, 0, w, h, net1), smd_pad(ref, 2, half, 0, w, h, net2)],
            half * 2 + w, h)


def xtal_3225(ref):
    pads = [
        smd_pad(ref, 1, -1.1, 0.85, 1.3, 1.0),
        smd_pad(ref, 2, 1.1, 0.85, 1.3, 1.0, "GND"),
        smd_pad(ref, 3, 1.1, -0.85, 1.3, 1.0),
        smd_pad(ref, 4, -1.1, -0.85, 1.3, 1.0, "GND"),
    ]
    return pads, 3.2, 2.5


def lga12_vl53(ref):
    nets = {3: "GND", 4: "GND", 6: "GND", 12: "GND", 9: "I2C1_SDA",
            10: "I2C1_SCL", 11: "VCC_3V3", 1: "VCC_3V3"}
    pads = []
    for i in range(6):  # bottom row 1-6 left->right
        pads.append(smd_pad(ref, 1 + i, -2.0 + i * 0.8, 0.8, 0.45, 0.6, nets.get(1 + i)))
    for i in range(6):  # top row 7-12 right->left
        pads.append(smd_pad(ref, 7 + i, 2.0 - i * 0.8, -0.8, 0.45, 0.6, nets.get(7 + i)))
    return pads, 4.4, 2.4


def lga10_bmp390(ref):
    pads = []
    for i in range(5):  # left col 1-5 top->bottom
        pads.append(smd_pad(ref, 1 + i, -0.85, -1.0 + i * 0.5, 0.45, 0.25))
    for i in range(5):  # right col 6-10 bottom->top
        pads.append(smd_pad(ref, 6 + i, 0.85, 1.0 - i * 0.5, 0.45, 0.25))
    return pads, 2.0, 2.5


def header_1xN(ref, n, nets=None):
    nets = nets or {}
    y0 = -(n - 1) / 2 * 2.54
    pads = [th_pad(ref, i + 1, 0, y0 + i * 2.54, 1.7, 1.0, nets.get(i + 1),
                   shape="rect" if i == 0 else "circle") for i in range(n)]
    return pads, 2.54, n * 2.54


def header_2xN_h(ref, n_cols, pitch, pad, drill, nets=None):
    """2-row header, rows along X (horizontal), odd pins in top row."""
    nets = nets or {}
    x0 = -(n_cols - 1) / 2 * pitch
    pads = []
    for c in range(n_cols):
        p_top, p_bot = 1 + 2 * c, 2 + 2 * c
        pads.append(th_pad(ref, p_top, x0 + c * pitch, -pitch / 2, pad, drill, nets.get(p_top),
                           shape="rect" if p_top == 1 else "circle"))
        pads.append(th_pad(ref, p_bot, x0 + c * pitch, pitch / 2, pad, drill, nets.get(p_bot)))
    return pads, n_cols * pitch, pitch * 2


def barrel_jack(ref):
    pads = [
        th_pad(ref, 1, 0, 0, 3.0, 1.6, "VCC_12V", shape="rect"),
        th_pad(ref, 2, 6.1, 0, 3.0, 1.6, "GND"),
        th_pad(ref, 3, 3.0, 4.6, 3.0, 1.6, "GND"),
    ]
    return pads, 9.0, 11.0


def usb_mini_b(ref):
    nets = {1: "VCC_5V", 2: "USB_DM", 3: "USB_DP", 5: "GND"}
    pads = [smd_pad(ref, 1 + i, -1.6 + i * 0.8, 3.6, 0.5, 2.0, nets.get(1 + i)) for i in range(5)]
    for j, xx in enumerate((-3.8, 3.8)):
        pads.append(th_pad(ref, "S" + str(j + 1), xx, 3.0, 2.2, 1.5, "GND"))
        pads.append(th_pad(ref, "S" + str(j + 3), xx, -1.5, 2.2, 1.5, "GND"))
    return pads, 7.7, 9.3


def slide_switch(ref):
    pads = [th_pad(ref, 1, -2.54, 0, 1.6, 0.9, "VCC_3V3"),
            th_pad(ref, 2, 0, 0, 1.6, 0.9, "BOOT0"),
            th_pad(ref, 3, 2.54, 0, 1.6, 0.9, "GND")]
    return pads, 8.0, 3.5


def test_point(ref, net):
    return [smd_pad(ref, 1, 0, 0, 1.5, 1.5, net, shape="circle")], 1.5, 1.5


# ---------------------------------------------------------------------------
# Placement table: (builder result, lib name, ref, value, x, y, desc, th)
# ---------------------------------------------------------------------------

def build_all():
    out = []

    def put(gen, lib, ref, val, x, y, desc="", th=False):
        pads, bw, bh = gen
        out.append(footprint(lib, ref, val, x, y, pads, bw, bh, desc, th))

    # --- POWER zone (y 51.5-62), left->right along top edge -----------------
    put(barrel_jack("J1"), "Connector_BarrelJack:BarrelJack_5.5x2.1mm", "J1",
        "12V_IN", 63, 56.5, "12V input, verify footprint vs part", th=True)
    put(chip2("D1", 2.15, 1.5, 1.8, None, "VCC_12V"), "Diode_SMD:D_SMA", "D1",
        "1N5819 SMA", 70.5, 56.5, "Reverse polarity Schottky")
    put(sot23_5("U2", {1: "VCC_12V", 2: "GND", 3: "VCC_12V"}),
        "Package_TO_SOT_SMD:SOT-23-5", "U2", "TPS62133A", 76.5, 56.5,
        "Buck 12V->5V; VERIFY package vs ordered part variant")
    put(chip2("L1", 1.7, 1.2, 3.6, None, "VCC_5V"), "Inductor_SMD:L_4x4mm", "L1",
        "4.7uH", 82, 56.5, "Buck inductor")
    put(chip2("C2", 0.95, 1.0, 1.3, "VCC_5V", "GND"), "Capacitor_SMD:C_0805", "C2",
        "10uF", 87, 56.5, "5V bulk")
    put(sot223("U3", {1: "GND", 2: "VCC_3V3", 3: "VCC_5V"}, "VCC_3V3"),
        "Package_TO_SOT_SMD:SOT-223", "U3", "LDO_3V3_500mA", 94, 56.5, "3V3 LDO")
    put(chip2("C14", 0.95, 1.0, 1.3, "VCC_3V3", "GND"), "Capacitor_SMD:C_0805", "C14",
        "10uF", 100, 56.5, "3V3 bulk")
    put(sot23_5("U12", {1: "VCC_5V", 2: "GND", 5: "VCC_1V8"}),
        "Package_TO_SOT_SMD:SOT-23-5", "U12", "LDO_1V8_200mA", 106, 56.5, "1V8 LDO")
    put(chip2("C15", 0.95, 1.0, 1.3, "VCC_1V8", "GND"), "Capacitor_SMD:C_0805", "C15",
        "10uF", 111, 56.5, "1V8 bulk")
    for ref, net, x, y in (("TP1", "VCC_12V", 115, 54.5), ("TP2", "VCC_5V", 118, 54.5),
                           ("TP3", "VCC_3V3", 121, 54.5), ("TP4", "VCC_1V8", 115, 58.5),
                           ("TP5", "GND", 118, 58.5), ("TP6", "GND", 121, 58.5)):
        put(test_point(ref, net), "TestPoint:TestPoint_Pad_1.5mm", ref, net, x, y, "Rail test point")

    # --- DEBUG/COMM zone (x 52-74) ------------------------------------------
    put(usb_mini_b("J5"), "Connector_USB:USB_Mini-B", "J5", "USB_MINI_B", 55, 72,
        "CP2102N host link; verify footprint vs part", th=True)
    put(qfn("U8", 7, 0.5, 2.4, 0.8, 0.25, 3.35, "GND"),
        "Package_DFN_QFN:QFN-28_5x5mm_P0.5mm", "U8", "CP2102N", 64, 72,
        "USB-UART bridge; nets bind on netlist import")
    put(header_1xN("J2", 3, {1: "UART2_TX", 2: "UART2_RX", 3: "GND"}),
        "Connector_PinHeader_2.54mm:PinHeader_1x03", "J2", "UART_DBG", 55.5, 82,
        "Debug UART header", th=True)
    put(header_2xN_h("J3", 5, 1.27, 0.9, 0.65,
                     {1: "VCC_3V3", 3: "GND", 5: "GND", 9: "GND", 10: "NRST"}),
        "Connector_PinHeader_1.27mm:PinHeader_2x05", "J3", "SWD", 62, 88,
        "ARM 10-pin SWD; SWDIO/SWCLK bind on netlist import", th=True)
    put(slide_switch("SW1"), "Button_Switch_THT:SW_Slide", "SW1", "BOOT0", 70, 92,
        "Bootloader entry switch", th=True)
    put(chip2("R1", 0.51, 0.54, 0.64, "VCC_3V3", "NRST"), "Resistor_SMD:R_0402", "R1",
        "10k", 73.5, 76, "NRST pull-up")
    put(chip2("C1", 0.51, 0.54, 0.64, "NRST", "GND"), "Capacitor_SMD:C_0402", "C1",
        "100nF", 73.5, 79, "NRST debounce")

    # --- MCU zone (x 76-104) ------------------------------------------------
    put(lqfp144("U1", U1_NETS), "Package_QFP:LQFP-144_20x20mm_P0.5mm", "U1",
        "STM32H745ZIT6", 90, 78, "Dual-core MCU; nets bind on netlist import")
    put(xtal_3225("Y1"), "Crystal:Crystal_SMD_3225-4Pin", "Y1", "8MHz", 80, 92.5,
        "HSE crystal, <5mm to PH0/PH1, GND guard")
    put(chip2("C12", 0.51, 0.54, 0.64, None, "GND"), "Capacitor_SMD:C_0402", "C12",
        "12pF 1%", 76.5, 92.5, "HSE load cap")
    put(chip2("C13", 0.51, 0.54, 0.64, None, "GND"), "Capacitor_SMD:C_0402", "C13",
        "12pF 1%", 83.5, 92.5, "HSE load cap")
    decap_pos = [("C3", 82, 65), ("C4", 90, 65), ("C5", 98, 65),
                 ("C6", 103.6, 72), ("C7", 103.6, 86.6),
                 ("C8", 98, 91.5), ("C9", 76.5, 72), ("C10", 76.5, 84)]
    for ref, x, y in decap_pos:
        put(chip2(ref, 0.51, 0.54, 0.64, "VCC_3V3", "GND"), "Capacitor_SMD:C_0402",
            ref, "100nF", x, y, "VDD decoupling, <2mm to pin after final placement")
    put(chip2("C11", 0.95, 1.0, 1.3, "VCC_3V3", "GND"), "Capacitor_SMD:C_0805", "C11",
        "10uF", 103.6, 78, "MCU bulk")
    put(chip2("R2", 0.51, 0.54, 0.64, "VCC_3V3", "I2C1_SCL"), "Resistor_SMD:R_0402",
        "R2", "10k", 99, 92.5, "I2C SCL pull-up")
    put(chip2("R3", 0.51, 0.54, 0.64, "VCC_3V3", "I2C1_SDA"), "Resistor_SMD:R_0402",
        "R3", "10k", 101.5, 92.5, "I2C SDA pull-up")
    put(chip2("R4", 0.51, 0.54, 0.64, "BOOT0", "GND"), "Resistor_SMD:R_0402",
        "R4", "10k", 73.5, 82, "BOOT0 pull-down")

    # --- MEMORY zone (x 106-128) --------------------------------------------
    put(soic8("U6", 3.7, {1: "QSPI_CS", 2: "QSPI_D1", 3: "QSPI_D2", 4: "GND",
                          5: "QSPI_D0", 6: "QSPI_CLK", 7: "QSPI_D3", 8: "VCC_3V3"}),
        "Package_SO:SOIC-8_5.23x5.23mm_P1.27mm", "U6", "W25Q128JV", 114, 71,
        "QSPI flash, keep total QSPI length <=25mm, match +/-0.5mm")
    put(chip2("C16", 0.51, 0.54, 0.64, "VCC_3V3", "GND"), "Capacitor_SMD:C_0402",
        "C16", "100nF", 120.5, 68, "Flash decoupling")
    put(soic8("U7", 2.7, {1: "GND", 2: "GND", 3: "GND", 4: "GND",
                          5: "I2C1_SDA", 6: "I2C1_SCL", 7: "GND", 8: "VCC_3V3"}),
        "Package_SO:SOIC-8_3.9x4.9mm_P1.27mm", "U7", "AT24C256C", 114, 86,
        "Config EEPROM, A0-A2 + WP grounded")
    put(chip2("C17", 0.51, 0.54, 0.64, "VCC_3V3", "GND"), "Capacitor_SMD:C_0402",
        "C17", "100nF", 120.5, 83, "EEPROM decoupling")

    # --- SENSOR zone (y 96-108.5) -------------------------------------------
    put(lga12_vl53("U5"), "Sensor_Distance:ST_VL53L0X", "U5", "VL53L0X", 63, 101,
        "ToF rangefinder at board edge, clear optical aperture")
    put(chip2("C18", 0.51, 0.54, 0.64, "VCC_3V3", "GND"), "Capacitor_SMD:C_0402",
        "C18", "100nF", 84.5, 101, "IMU decoupling")
    put(qfn("U4", 6, 0.5, 1.95, 0.8, 0.25, 2.7, "GND",
            {8: "VCC_3V3", 13: "VCC_3V3", 18: "GND", 22: "SPI1_CS_IMU",
             23: "SPI1_CLK", 24: "SPI1_MOSI"}),
        "Package_DFN_QFN:QFN-24_4x4mm_P0.5mm", "U4", "ICM-20689", 90, 101,
        "IMU at mount-hole center of mass, X axis along board length; "
        "SDO/MISO pin bind on netlist import")
    put(msop8("U10", {1: "I2C1_SDA", 2: "I2C1_SCL", 4: "GND", 5: "GND",
                      6: "GND", 7: "GND", 8: "VCC_3V3"}),
        "Package_SO:MSOP-8_3x3mm_P0.65mm", "U10", "MCP9808", 96.5, 101,
        "Barrel temp sensor, keep away from buck thermal zone")
    put(lga10_bmp390("U11"), "Sensor_Pressure:Bosch_LGA-10", "U11", "BMP390",
        100.5, 101, "Barometer; pinout bind on netlist import")
    put(header_2xN_h("J4", 7, 2.54, 1.7, 1.0,
                     {1: "VCC_3V3", 2: "GND", 3: "I2C1_SCL", 4: "I2C1_SDA",
                      5: "SPI1_CLK", 6: "SPI1_MISO", 7: "SPI1_MOSI",
                      8: "SPI1_CS_IMU", 14: "GND"}),
        "Connector_PinHeader_2.54mm:PinHeader_2x07", "J4", "SENSOR_14P", 112, 100,
        "Fixed pinout, v1.x/v2.0 compatibility contract (HLD §5.4)", th=True)

    return "\n".join(out)


def main():
    text = PCB.read_text()
    block = BEGIN + "\n" + build_all() + "\n" + END
    if BEGIN in text:
        pattern = re.escape(BEGIN) + r".*?" + re.escape(END)
        text = re.sub(pattern, lambda _: block, text, flags=re.S)
    else:
        idx = text.rstrip().rfind(")")
        text = text.rstrip()[:idx] + block + "\n)\n"
    PCB.write_text(text)

    # Sanity: balanced parens
    bal = text.count("(") - text.count(")")
    n_fp = text.count("(footprint ")
    print(f"OK: wrote {PCB.name}: {n_fp} footprints, paren balance = {bal}")
    if bal != 0:
        sys.exit("ERROR: unbalanced s-expression")


if __name__ == "__main__":
    main()
