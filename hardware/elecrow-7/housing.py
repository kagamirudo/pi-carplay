#!/usr/bin/env python3
"""Housing for an Elecrow RC070 7\" 1024x600 HDMI module, a Raspberry Pi 5
with Active Cooler and M.2 HAT+, and a Camera Module 3 dashcam.

Units are millimetres. Print in PETG, 0.20 mm layers, 4 perimeters, 30% infill.
  bezel.stl           face down (window on the bed)
  rear-shell.stl      back face on the bed (open side up)
  camera-bracket.stl  foot on the bed
  fit-gauge.stl       flat, print this first and drop the real module on it

Screws
  4x M3x20  display corners, into M3 nuts in the rear shell (5.5 mm AF)
  4x M2.5   Pi bosses, nuts under the board (official HAT spacer screws)
  2x M3x8   camera bracket onto the top wall
  4x M2x6   Camera Module 3 onto the bracket, M2 nuts behind the plate

The screen faces the driver. The camera sits on the top rear and looks
forward, 12 degrees down, through the windshield.
"""

from __future__ import annotations

import math
import struct
from pathlib import Path

import numpy as np
from manifold3d import Manifold

OUT = Path(__file__).resolve().parent / "export"

# --- Elecrow RC070 / Waveshare 7inch HDMI LCD (C) -------------------
# Appearance 164 x 124 x 14.4 (Elecrow). Active area 154.21 x 85.92
# (Waveshare panel drawing). Corner holes are M3, about 4.4 mm in from
# each edge on the published 164 mm width; the fit gauge is the check.
PCB_W, PCB_H, PCB_T = 164.0, 124.0, 14.4
AA_W, AA_H = 154.21, 85.92
HOLE_INSET = 4.5
OPEN_W, OPEN_H = 155.0, 86.7          # AA plus a 0.4 mm black-border reveal
AA_TOP = 8.2                          # opening inset from the top of the module
CLEAR = 0.50                          # pocket clearance per side

# --- Raspberry Pi 5, RP-008347-DS-1 ---------------------------------
PI_L, PI_W, PI_T = 85.0, 56.0, 1.6
# Hole centres from the SD-card / HDMI corner. Spacing 58 x 49.
PI_HOLES = ((3.5, 3.5), (61.5, 3.5), (3.5, 52.5), (61.5, 52.5))
USBC_X = 11.2                         # from the SD edge, along the HDMI edge
# Active cooler is 13.7 tall. M.2 HAT+ uses 16 mm spacers over it,
# then the HAT and a 2230 drive.
HAT_CLEAR = 16.0 + 1.6 + 3.8 + 3.0

# --- Camera Module 3, RP-008153-DS-1 (same holes as Camera Module 2) -
CAM_W, CAM_H = 25.0, 23.862
# Hole centres from the PCB corner: 2.0 mm from the long edges,
# 4.0 mm from the near short edge, 12.5 mm further on.
CAM_HOLES = ((2.0, 4.0), (23.0, 4.0), (2.0, 16.5), (23.0, 16.5))
CAM_TILT = 12.0

# --- Case -----------------------------------------------------------
WALL = 2.6
FRONT = 2.4
POCKET_D = PCB_T + 0.8
GAP = 8.0                             # air behind the display PCB
BEZEL_Z = FRONT + POCKET_D
REAR_INNER = GAP + PI_T + HAT_CLEAR
REAR_D = REAR_INNER + WALL
POCKET_W = PCB_W + 2 * CLEAR
POCKET_H = PCB_H + 2 * CLEAR
W = POCKET_W + 2 * WALL
H = POCKET_H + 2 * WALL

# Pi sits USB-edge toward the right wall, HDMI-edge above a cable gutter.
USB_OVERHANG = 2.0
PI_X = W - WALL - USB_OVERHANG - PI_L
PI_Y = 20.0
PI_Z = GAP                            # solder side, rear-shell local Z

CAM_FOOT_W, CAM_FOOT_D = 50.0, 24.0
CAM_X0 = (W - CAM_FOOT_W) / 2
CAM_Z0 = REAR_D - CAM_FOOT_D - 2.0
BRACKET_HOLES = ((10.0, 12.0), (40.0, 12.0))  # in the foot, 30 mm apart


def cube(sx, sy, sz, x=0.0, y=0.0, z=0.0):
    return Manifold.cube((sx, sy, sz)).translate((x, y, z))


def cyl(h, r, seg=64):
    return Manifold.cylinder(h, r, circular_segments=seg)


def hole_z(r, h, x, y, z, seg=48):
    return cyl(h, r, seg).translate((x, y, z))


def hole_y(r, length, x, y, z, seg=48):
    # +Z cylinder, +90 deg about X, lands on -Y. Shift so it occupies [y, y+length].
    return cyl(length, r, seg).rotate((90, 0, 0)).translate((x, y + length, z))


def hole_x(r, length, x, y, z, seg=48):
    # +90 deg about Y takes +Z onto +X, so the cylinder already starts at x=0.
    return cyl(length, r, seg).rotate((0, 90, 0)).translate((x, y, z))


def hex_z(across_flats, h, x, y, z):
    radius = across_flats / math.sqrt(3)
    pts = [
        (x + radius * math.cos(math.radians(60 * i + 30)),
         y + radius * math.sin(math.radians(60 * i + 30)))
        for i in range(6)
    ]
    from manifold3d import CrossSection
    return Manifold.extrude(CrossSection([pts]), h).translate((0, 0, z))


def check_helpers():
    hz = hole_z(1, 5, 0, 0, 0).bounding_box()
    assert abs(hz[5] - 5) < 1e-3 and abs(hz[2]) < 1e-3
    hy = hole_y(1, 5, 0, 2, 0).bounding_box()
    assert abs(hy[1] - 2) < 0.05 and abs(hy[4] - 7) < 0.05, hy
    hx = hole_x(1, 5, 1, 0, 0).bounding_box()
    assert abs(hx[0] - 1) < 0.05 and abs(hx[3] - 6) < 0.05, hx
    # Camera hole pattern matches the drawing chain.
    xs = sorted({p[0] for p in CAM_HOLES})
    ys = sorted({p[1] for p in CAM_HOLES})
    assert abs((xs[1] - xs[0]) - 21.0) < 1e-9
    assert abs((ys[1] - ys[0]) - 12.5) < 1e-9
    assert abs(2.0 + 21.0 + 2.0 - CAM_W) < 1e-9
    assert abs(4.0 + 12.5 + 7.362 - CAM_H) < 0.01
    # Pi holes.
    assert abs(PI_HOLES[1][0] - PI_HOLES[0][0] - 58.0) < 1e-9
    assert abs(PI_HOLES[2][1] - PI_HOLES[0][1] - 49.0) < 1e-9
    # Window reveals the black border and does not eat pixels.
    assert 0 < (OPEN_W - AA_W) < 1.2
    assert 0 < (OPEN_H - AA_H) < 1.2


def write_stl(solid: Manifold, path: Path):
    if str(solid.status()) not in ("Error.NoError", "NoError"):
        raise SystemExit(f"{path.name}: manifold status {solid.status()}")
    if solid.volume() < 100:
        raise SystemExit(f"{path.name}: volume {solid.volume()} is implausible")
    mesh = solid.to_mesh()
    vert = np.asarray(mesh.vert_properties, dtype=np.float64)[:, :3]
    tri = np.asarray(mesh.tri_verts, dtype=np.int32)
    pts = vert[tri]
    normal = np.cross(pts[:, 1] - pts[:, 0], pts[:, 2] - pts[:, 0])
    length = np.linalg.norm(normal, axis=1, keepdims=True)
    normal = normal / np.clip(length, 1e-12, None)
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("wb") as handle:
        handle.write(b"elecrow-pi5-housing".ljust(80, b"\0"))
        handle.write(struct.pack("<I", len(tri)))
        for i in range(len(tri)):
            handle.write(struct.pack("<3f", *normal[i]))
            for p in pts[i]:
                handle.write(struct.pack("<3f", *p))
            handle.write(struct.pack("<H", 0))
    bb = solid.bounding_box()
    print(f"{path.name:20}  {len(tri):7d} tris  "
          f"{bb[3]-bb[0]:7.2f} x {bb[4]-bb[1]:7.2f} x {bb[5]-bb[2]:6.2f} mm  "
          f"vol {solid.volume()/1000:7.1f} cm3")
    return bb


def display_holes(origin_x, origin_y):
    return [
        (origin_x + HOLE_INSET, origin_y + HOLE_INSET),
        (origin_x + PCB_W - HOLE_INSET, origin_y + HOLE_INSET),
        (origin_x + HOLE_INSET, origin_y + PCB_H - HOLE_INSET),
        (origin_x + PCB_W - HOLE_INSET, origin_y + PCB_H - HOLE_INSET),
    ]


def bezel():
    mod_x = WALL + CLEAR
    mod_y = WALL + CLEAR
    open_x = mod_x + (PCB_W - OPEN_W) / 2
    open_y = mod_y + PCB_H - AA_TOP - OPEN_H
    part = cube(W, H, BEZEL_Z)
    part -= cube(POCKET_W, POCKET_H, POCKET_D + 0.05, WALL, WALL, FRONT)
    part -= cube(OPEN_W, OPEN_H, FRONT + 0.1, open_x, open_y, -0.05)
    for x, y in display_holes(mod_x, mod_y):
        part -= hole_z(2.1, FRONT + 0.2, x, y, -0.1)  # Ø4.2, M3 with slack
    # Two 3 mm alignment pins on the back face.
    for x, y in ((14, 14), (W - 14, H - 14)):
        part += hole_z(1.5, 4.0, x, y, BEZEL_Z - 0.05)
    bb = part.bounding_box()
    assert abs(bb[3] - W) < 0.05 and abs(bb[4] - H) < 0.05
    # Opening stays on the glass, clear of the corner screws.
    assert open_x > mod_x + 2 and open_y > mod_y + 2
    return part


def rear_shell():
    part = cube(W, H, REAR_D)
    part -= cube(W - 2 * WALL, H - 2 * WALL, REAR_D - WALL + 0.05, WALL, WALL, -0.05)
    # Alignment sockets for the bezel pins.
    for x, y in ((14, 14), (W - 14, H - 14)):
        part -= hole_z(1.65, 4.5, x, y, -0.1)
    # Bosses reach 0.6 mm into the bezel and land on the PCB around each
    # corner hole. The M3 nut drops in from the open end of the boss.
    mod_x = WALL + CLEAR
    mod_y = WALL + CLEAR
    for x, y in display_holes(mod_x, mod_y):
        part += cyl(8.6, 4.2, seg=48).translate((x, y, -0.6))
        part -= hole_z(2.1, 9.2, x, y, -0.8)
        part -= hex_z(5.7, 2.8, x, y, 5.2)  # M3 nut, 5.5 mm across flats
    # Pi bosses. Nut opens toward the board.
    for hx, hy in PI_HOLES:
        x, y = PI_X + hx, PI_Y + hy
        part += cyl(7.0, 4.2).translate((x, y, PI_Z - 7.0))
        part -= hole_z(1.45, 8.0, x, y, PI_Z - 7.2)          # Ø2.9
        part -= hex_z(5.2, 2.0, x, y, PI_Z - 2.0)            # M2.5 nut
    # USB-A and Ethernet, right wall.
    part -= cube(WALL + 0.4, 50, 18, W - WALL - 0.2, PI_Y + 3, PI_Z - 1)
    # USB-C power, bottom wall.
    part -= cube(13, WALL + 0.4, 10, PI_X + USBC_X - 6.5, -0.2, PI_Z - 1)
    # microSD finger window in the rear wall, just left of the slot.
    part -= cube(28, 16, WALL + 0.4, PI_X - 18, PI_Y + 6, REAR_D - WALL - 0.2)
    # Fan / HAT exhaust. Slots through the rear wall over the board.
    for i in range(7):
        part -= cube(36, 2.2, WALL + 0.4, PI_X + 8, PI_Y + 8 + i * 6.2, REAR_D - WALL - 0.2)
    # Camera-bracket screws and the CSI slot, through the top wall.
    for dx, dy in BRACKET_HOLES:
        part -= hole_y(1.7, WALL + 0.4, CAM_X0 + dx, H - WALL - 0.2, CAM_Z0 + dy)
    part -= cube(18, WALL + 0.4, 2.2, CAM_X0 + 16, H - WALL - 0.2, CAM_Z0 - 1.0)
    return part


def camera_bracket():
    """Foot on the bed (Z up). Lens looks along +Y, 12 degrees down (-Z).

    On the case, +Y of this part points at the windshield and +Z is up.
    """
    foot = cube(CAM_FOOT_W, CAM_FOOT_D, 4.0)
    for dx, dy in BRACKET_HOLES:
        foot -= hole_z(1.7, 5.0, dx, dy, -0.2)

    plate_w = 30.0
    plate_h = CAM_H + 6.0
    plate_t = 3.2
    plate = cube(plate_w, plate_t, plate_h)
    pocket_x = (plate_w - (CAM_W + 0.4)) / 2
    pocket_z = 3.0
    plate -= cube(CAM_W + 0.4, 1.7, CAM_H + 0.4, pocket_x, -0.05, pocket_z)
    cx = pocket_x + 0.2 + CAM_W / 2
    cz = pocket_z + 0.2 + CAM_H / 2
    plate -= hole_y(6.0, plate_t + 0.8, cx, -0.4, cz)
    for hx, hy in CAM_HOLES:
        plate -= hole_y(1.15, plate_t + 1.0, pocket_x + 0.2 + hx, -0.5, pocket_z + 0.2 + hy)
    # Negative roll about X aims +Y down toward -Z.
    plate = plate.rotate((-CAM_TILT, 0, 0))
    bb = plate.bounding_box()
    plate = plate.translate((
        (CAM_FOOT_W - plate_w) / 2 - bb[0],
        8.0 - bb[1],
        3.6 - bb[2],
    ))
    part = foot + plate
    part -= cube(16, 2.4, 3.2, (CAM_FOOT_W - 16) / 2, 4.0, 0.4)
    stand = part.bounding_box()
    if stand[5] < 20:
        raise SystemExit(f"camera bracket does not stand up ({stand})")
    return part


def fit_gauge():
    """3 mm plate. The module should drop in and the four corner holes
    should show through the real M3 holes before the housing is printed."""
    pad = 6.0
    gw, gh = PCB_W + 2 * pad, PCB_H + 2 * pad
    part = cube(gw, gh, 3.0)
    part -= cube(PCB_W + 0.6, PCB_H + 0.6, 1.6, pad - 0.3, pad - 0.3, 1.5)
    ox = pad + (PCB_W - OPEN_W) / 2
    oy = pad + PCB_H - AA_TOP - OPEN_H
    part -= cube(OPEN_W, OPEN_H, 3.2, ox, oy, -0.1)
    for x, y in display_holes(pad, pad):
        part -= hole_z(2.1, 3.4, x, y, -0.2)
    return part


def main():
    check_helpers()
    print(f"case outline {W:.2f} x {H:.2f} mm, "
          f"bezel depth {BEZEL_Z:.2f}, rear depth {REAR_D:.2f}, "
          f"stack {BEZEL_Z + REAR_D:.2f} mm")
    print(f"Pi origin in the rear shell ({PI_X:.2f}, {PI_Y:.2f}, {PI_Z:.2f})")
    write_stl(bezel(), OUT / "bezel.stl")
    write_stl(rear_shell(), OUT / "rear-shell.stl")
    write_stl(camera_bracket(), OUT / "camera-bracket.stl")
    write_stl(fit_gauge(), OUT / "fit-gauge.stl")
    # Screws still land on the PCB, inside the pocket and outside the window.
    mod_x, mod_y = WALL + CLEAR, WALL + CLEAR
    open_x = mod_x + (PCB_W - OPEN_W) / 2
    open_y = mod_y + PCB_H - AA_TOP - OPEN_H
    for x, y in display_holes(mod_x, mod_y):
        assert mod_x < x < mod_x + PCB_W
        assert mod_y < y < mod_y + PCB_H
        assert not (open_x < x < open_x + OPEN_W and open_y < y < open_y + OPEN_H)


if __name__ == "__main__":
    main()
