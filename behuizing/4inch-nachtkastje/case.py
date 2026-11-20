"""Parametric nightstand case for the Waveshare ESP32-S3-Touch-LCD-4 (V4).

Two printed parts:
  behuizing  - front frame, body and base in one piece, screen tilted 20 deg
               back (print with the front face down, no support)
  achterklep - back cover; clips into the body, the screen is screwed onto it
               with 4 x M2.5 x 5 (print flat, outside down)

The screen goes in from the front, without a bezel over the glass: the glass
sits nearly flush in an opening in the front frame, on a ledge behind it, and
the M2.5 screws through the back cover pull it onto that ledge. The screen is mounted
upside down (terminal block at the top, USB-C on the left seen from the front;
use draaiing: "180").

Local frame of the tilted body ("slab"): X = right (seen from the front),
Y = depth backwards from the front face, Z = up along the front face.
The USB-C cable is an angled plug pointing down; it leaves at the bottom of
the back cover and runs back through a groove in the base.
All module dimensions come from Waveshare's STEP model (glass centre = 0,0).
"""
import math
import cadquery as cq

# ---------------- module (Waveshare STEP, mm) ----------------
GLASS = 84.2          # cover glass, square
GLASS_T = 0.7         # cover glass thickness
STANDOFF_DEPTH = 12.0  # glass front -> end of the M2.5 standoffs on the board
TERMINAL_DEPTH = 15.0  # glass front -> back of the 10-pin terminal header
# Screen upside down (180 deg), positions relative to the glass centre,
# as (x right, z up) seen from the front:
STANDOFFS = [(33.25, 33.28), (-33.25, 33.28), (33.25, -35.22), (-33.25, -35.22)]
USB_FACE_X = -36.68   # USB-C receptacle opening (left edge)
USB_Z = 10.71         # receptacle centre height
USB_DEPTH = 9.17      # glass front -> receptacle axis

# ---------------- 90 deg USB-C plug, cable pointing down (please measure) ---
# The head stays inside; the cable runs down along the left wall, bends
# backwards at the bottom and leaves through a notch in the cover's bottom edge.
PLUG_LEN = 13.0 + (42.1 - 36.68)  # head sticks out 13 mm past the glass edge (measured)
PLUG_T = 8.0          # head thickness (front to back)
PLUG_UP = 4.0         # head above the receptacle axis
PLUG_DOWN = 20.0      # head + strain relief below the receptacle axis
CABLE_D = 4.0         # cable diameter
CABLE_X = USB_FACE_X - PLUG_LEN / 2   # where the cable runs down

# ---------------- case parameters ----------------
TILT = 20.0           # screen leans back
GLASS_IN = 0.3        # glass sits this far behind the front face
CLEAR = 0.2           # clearance around the glass per side
GLASS_R = 2.5         # corner radius of the opening (glass corners ~R2.3)
FW = 3.0              # front frame: glass pocket + 2 mm ledge behind it
# Ledge behind the glass, all round. It clears the LCD behind the glass
# (screen upside down): x -38..38, z -40.6..37.8. With the plug in, the screen
# goes in by swinging it about the left edge of the glass: USB side first, the
# plug head passes behind the ledge (it sits ~4.5 mm behind the glass).
LEDGE = (-38.0, 38.0, -40.6, 37.8)
T = 24.0              # body depth (front face -> back face)
WALL = 2.5
COVER_T = 3.0
CAV_X = 56.2          # inner half-width: 14 mm from the glass edge for the plug
CAV_TOP = 56.2        # inner top, from glass centre
CAV_BOT = -44.5       # inner bottom, from glass centre
UC = 62.0             # glass centre height above the front bottom edge
R_OUT = 10.0          # outer corner radius (front view)
FILLET = 3.0          # edge rounding front/back

BASE_H = 7.0
BASE_DEPTH = 80.0
BASE_R_FRONT = 3.0
BASE_R_BACK = 22.0

W_OUT = 2 * (CAV_X + WALL)
TOP_OUT = UC + CAV_TOP + WALL
BOX_BOTTOM = -20.0            # extend below and cut flat afterwards
REB = 1.0                     # rebate ledge for the back cover
# snap clips cover -> body: (side, position along that wall from glass centre)
COVER_CLIPS = [("R", 22.0), ("R", -30.0), ("L", 30.0), ("L", -30.0),
               ("T", -25.0), ("T", 25.0), ("B", -25.0), ("B", 25.0)]
CLIP_W, CLIP_T, CLIP_L, CLIP_HOOK = 8.0, 1.4, 12.0, 0.8
s, c = math.sin(math.radians(TILT)), math.cos(math.radians(TILT))


def slab_prism(x0, x1, z0, z1, y0, y1, r):
    w, h = x1 - x0, z1 - z0
    sk = (cq.Sketch().rect(w, h).vertices().fillet(r)) if r > 0 else cq.Sketch().rect(w, h)
    return (cq.Workplane("XZ", origin=(0, y0, 0))
            .center((x0 + x1) / 2, (z0 + z1) / 2)
            .placeSketch(sk).extrude(-(y1 - y0)))


def to_world(wp):
    """Tilt the slab and stand it on top of the base plate."""
    return wp.rotate((0, 0, 0), (1, 0, 0), -TILT).translate((0, 0, BASE_H))


def world_cut_below(wp, z):
    big = cq.Workplane("XY").box(400, 400, 200, centered=(True, True, False)).translate((0, 0, z - 200))
    return wp.cut(big)


# ---------------- snap clips (cover -> body) ----------------
# Built with the wall at x = 0 and its outward normal along +X; the clip
# hangs forward (-Y) from the inside of the cover at y = T - COVER_T.
def _clip_tab():
    y0 = T - COVER_T
    tip = y0 - CLIP_L
    xo = -0.15                       # outer face of the tab, just off the wall
    tab = (cq.Workplane("XY", origin=(0, 0, -CLIP_W / 2))
           .polyline([(xo - CLIP_T, tip), (xo, tip), (xo + CLIP_HOOK, tip + 1.5),
                      (xo + CLIP_HOOK, tip + 2.2), (xo, tip + 2.7),
                      (xo, y0 + 0.5), (xo - CLIP_T, y0 + 0.5)]).close()
           .extrude(CLIP_W))
    return tab


def _clip_groove():
    tip = T - COVER_T - CLIP_L
    return (cq.Workplane("XY", origin=(0, 0, -CLIP_W / 2 - 0.5))
            .polyline([(-0.01, tip - 0.4), (CLIP_HOOK + 0.25, tip - 0.4),
                       (CLIP_HOOK + 0.25, tip + 2.55), (-0.01, tip + 2.55)]).close()
            .extrude(CLIP_W + 1.0))


def _clip_place(shape, side, pos):
    zc = UC
    if side == "R":
        return shape.translate((CAV_X, 0, zc + pos))
    if side == "L":
        return shape.rotate((0, 0, 0), (0, 1, 0), 180).translate((-CAV_X, 0, zc + pos))
    if side == "T":
        return shape.rotate((0, 0, 0), (0, 1, 0), -90).translate((pos, 0, zc + CAV_TOP))
    return shape.rotate((0, 0, 0), (0, 1, 0), 90).translate((pos, 0, zc + CAV_BOT))


# ---------------- body + base ----------------
def _base():
    y0, y1 = -BASE_H * s / c - 3, BASE_DEPTH
    base = (cq.Workplane("XY").center(0, (y0 + y1) / 2)
            .rect(W_OUT, y1 - y0).extrude(BASE_H))
    base = base.edges("|Z and >Y").fillet(BASE_R_BACK)
    base = base.edges("|Z and <Y").fillet(BASE_R_FRONT)
    base = base.faces(">Z").edges().fillet(2.5)
    base = base.faces("<Z").edges().fillet(0.8)
    return base


def behuizing():
    body = slab_prism(-W_OUT / 2, W_OUT / 2, BOX_BOTTOM, TOP_OUT, 0, T, R_OUT)
    body = body.faces("<Y or >Y").edges().fillet(FILLET)
    cav = slab_prism(-CAV_X, CAV_X, UC + CAV_BOT, UC + CAV_TOP, FW, T + 1, R_OUT - WALL)
    reb = slab_prism(-CAV_X - REB, CAV_X + REB, UC + CAV_BOT - REB, UC + CAV_TOP + REB,
                     T - COVER_T, T + 1, R_OUT - WALL + REB)
    g = GLASS + 2 * CLEAR
    opening = slab_prism(-g / 2, g / 2, UC - g / 2, UC + g / 2, -1,
                         GLASS_IN + GLASS_T + 0.05, GLASS_R)
    x0, x1, z0, z1 = LEDGE
    behind = slab_prism(x0, x1, UC + z0, UC + z1, GLASS_IN + GLASS_T, FW + 0.01, 1.0)
    body = body.cut(cav).cut(reb).cut(opening).cut(behind)
    # small chamfer around the opening
    body = body.edges(cq.selectors.BoxSelector((-g / 2 - 0.1, -0.1, UC - g / 2 - 0.1),
                                               (g / 2 + 0.1, 0.1, UC + g / 2 + 0.1))).chamfer(0.5)
    for side, pos in COVER_CLIPS:
        body = body.cut(_clip_place(_clip_groove(), side, pos))
    body = world_cut_below(to_world(body), 0.5)
    body = body.union(_base())
    # the front of the base continues the front face
    front = to_world(slab_prism(-200, 200, -200, 400, -100, 0, 0))
    body = body.cut(front)
    # cable groove on top of the base, from the cover notch to the back edge
    gy0, gy1 = T / c + 1.5, BASE_DEPTH + 4
    body = body.cut(cq.Workplane("XY", origin=(0, 0, BASE_H - 2.5))
                    .center(CABLE_X, (gy0 + gy1) / 2)
                    .slot2D(gy1 - gy0, CABLE_D + 1.0, 90).extrude(5))
    # recesses for 4 rubber feet (10 mm)
    for x in (-W_OUT / 2 + 12, W_OUT / 2 - 12):
        for y in (8.0, BASE_DEPTH - 14):
            body = body.cut(cq.Workplane("XY", origin=(0, 0, -0.01)).center(x, y)
                            .circle(5.2).extrude(1.0))
    return body


# ---------------- back cover ----------------
def achterklep():
    y0 = T - COVER_T
    cl = 0.25
    cover = slab_prism(-CAV_X - REB + cl, CAV_X + REB - cl,
                       UC + CAV_BOT - REB + cl, UC + CAV_TOP + REB - cl,
                       y0, T, R_OUT - WALL + REB - cl)
    # 0.3 mm short of the standoffs: the screws pull the glass onto the ledge
    so_y = GLASS_IN + STANDOFF_DEPTH + 0.3
    floor = 2.0                          # material under the screw head: M2.5 x 5
    for x, z in STANDOFFS:
        tube = (cq.Workplane("XZ", origin=(0, so_y + floor, 0)).center(x, UC + z)
                .circle(4.0).extrude(-(y0 - so_y - floor + 0.5)))
        tip = (cq.Workplane("XZ", origin=(0, so_y, 0)).center(x, UC + z)
               .circle(2.9).extrude(-(floor + 0.5)))
        cover = cover.union(tube).union(tip)
        hole = (cq.Workplane("XZ", origin=(0, so_y - 1, 0)).center(x, UC + z)
                .circle(1.4).extrude(-(T + 2 - so_y)))
        cb = (cq.Workplane("XZ", origin=(0, so_y + floor, 0)).center(x, UC + z)
              .circle(2.65).extrude(-(T + 1 - so_y - floor)))
        cover = cover.cut(hole).cut(cb)
    for side, pos in COVER_CLIPS:
        cover = cover.union(_clip_place(_clip_tab(), side, pos))
    # small notch at the bottom edge to lever the cover off
    pry = slab_prism(-6, 6, UC + CAV_BOT - REB - 1, UC + CAV_BOT - REB + cl + 1.2,
                     T - 1.2, T + 1, 0)
    cover = cover.cut(pry)
    # notch in the bottom edge where the cable leaves, just above the base
    notch = slab_prism(CABLE_X - CABLE_D / 2 - 0.8, CABLE_X + CABLE_D / 2 + 0.8,
                       UC + CAV_BOT - REB - 1, UC + CAV_BOT + CABLE_D + 1.0,
                       y0 - 1, T + 1, 0)
    cover = cover.cut(notch)
    # sound slots for the buzzer
    for i in range(-3, 4):
        slot = (cq.Workplane("XZ", origin=(0, y0 - 1, 0)).center(-6, UC + 4 + i * 4.5)
                .slot2D(30, 2.2).extrude(-5))
        cover = cover.cut(slot)
    return world_cut_below(to_world(cover), BASE_H)


def module_in_world(step_path):
    """Waveshare module placed in the case, for a fit check."""
    m = cq.importers.importStep(step_path)
    from OCP.gp import gp_Trsf
    t = gp_Trsf()
    # model (x, y, z) -> slab (x, GLASS_IN+3.2-z, UC+0.83+y): upside down
    t.SetValues(1, 0, 0, 0,
                0, 0, -1, GLASS_IN + 3.2,
                0, 1, 0, UC + 0.83)
    sh = cq.Shape.cast(m.val().wrapped.Moved(cq.Location(t).wrapped))
    return to_world(cq.Workplane().add(sh))


if __name__ == "__main__":
    import sys
    out = sys.argv[1] if len(sys.argv) > 1 else "."
    parts = {"behuizing": behuizing(), "achterklep": achterklep()}
    for n, p in parts.items():
        cq.exporters.export(p, f"{out}/{n}.stl", tolerance=0.02, angularTolerance=0.1)
        cq.exporters.export(p, f"{out}/{n}.step")
        bb = p.val().BoundingBox()
        print(n, f"{bb.xlen:.1f} x {bb.ylen:.1f} x {bb.zlen:.1f} mm")
