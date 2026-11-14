"""Parametric nightstand case for the Waveshare ESP32-S3-Touch-LCD-4 (V4).

Three printed parts:
  behuizing  - front frame + body, tilted 20 deg back (print front face down)
  achterklep - back cover; M2.5 x 5 into the board's standoffs, clips into
               the body (print flat, outside down)
  voet       - base plate that keeps the clock stable; clips into the body
               (print flat)

Local frame of the tilted body ("slab"): X = right (seen from the front),
Y = depth backwards from the front face, Z = up along the front face.
All module dimensions come from Waveshare's STEP model (glass centre = 0,0).
"""
import math
import cadquery as cq

# ---------------- module (Waveshare STEP, mm) ----------------
GLASS = 84.2          # cover glass, square
GLASS_T = 0.7         # cover glass thickness
STANDOFF_DEPTH = 12.0  # glass front -> end of the M2.5 standoffs on the board
TERMINAL_DEPTH = 15.0  # glass front -> back of the 10-pin terminal header
# standoff centres relative to glass centre, as (x right, z up) seen from front
STANDOFFS = [(-33.25, -33.28), (33.25, -33.28), (-33.25, 35.22), (33.25, 35.22)]
# USB-C receptacle (on the right edge seen from the front)
USB_FACE_X = 36.68    # receptacle opening, from glass centre
USB_Z = -10.71        # centre height from glass centre
USB_DEPTH = 10.77 - 1.6  # glass front -> receptacle axis (lip added below)

# ---------------- 90 deg USB-C plug (please measure your own) ----------------
PLUG_LEN = 12.0       # from receptacle face to the far side of the head
PLUG_W = 13.0         # head width (along the board edge)

# ---------------- case parameters ----------------
TILT = 20.0           # screen leans back
LIP = 1.6             # bezel in front of the glass
CLEAR = 0.2           # clearance around the glass per side
WINDOW = 76.0         # visible opening (active area is 71.9 x 70.2)
SIDE = 10.4           # frame width left/right/top of the glass
T = 24.0              # body depth (front face -> back face)
WALL = 2.5
COVER_T = 3.0
CAV_X = 50.0          # inner half-width (room for the plug head)
CAV_TOP = 50.0        # inner top, from glass centre
CAV_BOT = -44.5       # inner bottom, from glass centre
UC = 57.0             # glass centre height above the front bottom edge
R_OUT = 10.0          # outer corner radius (front view)
FILLET = 3.0          # edge rounding front/back

BASE_H = 7.0
BASE_DEPTH = 80.0
BASE_FRONT = 2.0      # base sticks out this far in front of the body
BASE_R_FRONT = 6.0
BASE_R_BACK = 22.0
RECESS = 1.2          # body sits this deep in a locating recess in the base

FW = LIP + GLASS_T + 1.3      # front wall thickness outside the glass pocket
W_OUT = 2 * (CAV_X + WALL)
TOP_OUT = UC + CAV_TOP + WALL
BOX_BOTTOM = -20.0            # extend below and cut flat afterwards
REB = 1.0                     # rebate ledge for the back cover
# snap clips cover -> body: (side, position along that wall from glass centre)
COVER_CLIPS = [("R", 22.0), ("R", -32.0), ("L", 22.0), ("L", -32.0),
               ("T", -25.0), ("T", 25.0), ("B", -25.0), ("B", 25.0)]
CLIP_W, CLIP_T, CLIP_L, CLIP_HOOK = 8.0, 1.4, 12.0, 0.8
# snap clips base -> body (world)
BASE_CLIPS_X = (-30.0, 30.0)
BCLIP_Y = 10.0                # front face of the tab, behind the front bottom edge
BCLIP_W, BCLIP_T, BCLIP_HOOK = 10.0, 1.6, 0.7
BCLIP_ROOT = 1.5              # tab grows from this height inside a pocket in the base
BCLIP_IN = 6.0                # how far the tab reaches into the body
s, c = math.sin(math.radians(TILT)), math.cos(math.radians(TILT))


def slab_prism(x0, x1, z0, z1, y0, y1, r):
    w, h = x1 - x0, z1 - z0
    sk = (cq.Sketch().rect(w, h).vertices().fillet(r)) if r > 0 else cq.Sketch().rect(w, h)
    return (cq.Workplane("XZ", origin=(0, y0, 0))
            .center((x0 + x1) / 2, (z0 + z1) / 2)
            .placeSketch(sk).extrude(-(y1 - y0)))


def to_world(wp):
    """Tilt the slab and stand it on the base."""
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


# ---------------- body ----------------
def behuizing():
    body = slab_prism(-W_OUT / 2, W_OUT / 2, BOX_BOTTOM, TOP_OUT, 0, T, R_OUT)
    body = body.faces("<Y or >Y").edges().fillet(FILLET)
    # cavity
    cav = slab_prism(-CAV_X, CAV_X, UC + CAV_BOT, UC + CAV_TOP, FW, T + 1, R_OUT - WALL)
    reb = slab_prism(-CAV_X - REB, CAV_X + REB, UC + CAV_BOT - REB, UC + CAV_TOP + REB,
                     T - COVER_T, T + 1, R_OUT - WALL + REB)
    g = GLASS + 2 * CLEAR
    pocket = slab_prism(-g / 2, g / 2, UC - g / 2, UC + g / 2, LIP, FW + 0.01, 1.0)
    # window with a 45 deg chamfer towards the front
    win = (cq.Workplane("XZ", origin=(0, -0.01, 0)).center(0, UC)
           .rect(WINDOW + 2 * LIP + 0.02, WINDOW + 2 * LIP + 0.02)
           .workplane(offset=-(LIP + 0.03)).rect(WINDOW, WINDOW).loft())
    body = body.cut(cav).cut(reb).cut(pocket).cut(win)
    for side, pos in COVER_CLIPS:
        body = body.cut(_clip_place(_clip_groove(), side, pos))
    body = to_world(body)
    body = world_cut_below(body, BASE_H - RECESS)
    # slots for the base clips; room behind the tab to flex, catch in front
    zb = BASE_H - RECESS
    z_top = zb + BCLIP_IN
    for x in BASE_CLIPS_X:
        slot = (cq.Workplane("XY", origin=(0, 0, zb - 0.01))
                .center(x, BCLIP_Y + (BCLIP_T + BCLIP_HOOK + 0.4) / 2 - 0.15)
                .rect(BCLIP_W + 0.6, BCLIP_T + BCLIP_HOOK + 0.4 + 0.3).extrude(BCLIP_IN + 0.5))
        catch = (cq.Workplane("XY", origin=(0, 0, z_top - 2.4))
                 .center(x, BCLIP_Y - (BCLIP_HOOK + 0.25) / 2 + 0.01)
                 .rect(BCLIP_W + 0.6, BCLIP_HOOK + 0.25).extrude(2.9))
        body = body.cut(slot).cut(catch)
    return body


# ---------------- back cover ----------------
def achterklep():
    y0 = T - COVER_T
    cl = 0.25
    cover = slab_prism(-CAV_X - REB + cl, CAV_X + REB - cl,
                       UC + CAV_BOT - REB + cl, UC + CAV_TOP + REB - cl,
                       y0, T, R_OUT - WALL + REB - cl)
    so_y = LIP + STANDOFF_DEPTH - 0.1   # boss presses the glass lightly against the lip
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
    # notch for the 90 deg plug head, open to the cover edge
    nx0 = USB_FACE_X - 1.5
    notch = slab_prism(nx0, CAV_X + 5, UC + USB_Z - PLUG_W / 2 - 1.5,
                       UC + USB_Z + PLUG_W / 2 + 1.5, y0 - 1, T + 1, 0)
    cover = cover.cut(notch)
    # sound slots for the buzzer
    for i in range(-3, 4):
        slot = (cq.Workplane("XZ", origin=(0, y0 - 1, 0)).center(-6, UC + 4 + i * 4.5)
                .slot2D(30, 2.2).extrude(-5))
        cover = cover.cut(slot)
    return world_cut_below(to_world(cover), BASE_H)


# ---------------- base ----------------
def body_footprint():
    """Outline of the body on top of the base (world XY), for the locating recess."""
    dy = RECESS * s / c
    return -W_OUT / 2, W_OUT / 2, -dy, T / c


def voet():
    x0, x1 = -W_OUT / 2, W_OUT / 2
    y0, y1 = -BASE_FRONT, BASE_DEPTH
    pts = cq.Sketch().rect(x1 - x0, y1 - y0)
    base = (cq.Workplane("XY").center(0, (y0 + y1) / 2).placeSketch(pts).extrude(BASE_H))
    base = base.edges("|Z and >Y").fillet(BASE_R_BACK)
    base = base.edges("|Z and <Y").fillet(BASE_R_FRONT)
    base = base.faces(">Z").edges().fillet(2.5)
    base = base.faces("<Z").edges().fillet(0.8)
    # shallow locating recess for the body
    fx0, fx1, fy0, fy1 = body_footprint()
    rec = (cq.Workplane("XY", origin=(0, 0, BASE_H - RECESS))
           .center(0, (fy0 + fy1) / 2).rect(fx1 - fx0 + 0.6, fy1 - fy0 + 0.6).extrude(2))
    base = base.cut(rec)
    # snap clips: a tab that grows from a pocket, so it is long enough to flex
    z_top = BASE_H - RECESS + BCLIP_IN
    for x in BASE_CLIPS_X:
        pk = (cq.Workplane("XY", origin=(0, 0, BCLIP_ROOT)).center(x, BCLIP_Y + BCLIP_T / 2)
              .rect(BCLIP_W + 2.0, BCLIP_T + 2 * 1.2).extrude(BASE_H))
        base = base.cut(pk)
        tab = (cq.Workplane("YZ", origin=(x - BCLIP_W / 2, 0, 0))
               .polyline([(BCLIP_Y, BCLIP_ROOT - 0.5), (BCLIP_Y + BCLIP_T, BCLIP_ROOT - 0.5),
                          (BCLIP_Y + BCLIP_T, z_top), (BCLIP_Y + 0.2, z_top),
                          (BCLIP_Y - BCLIP_HOOK, z_top - 1.4), (BCLIP_Y - BCLIP_HOOK, z_top - 1.9),
                          (BCLIP_Y, z_top - 2.5)]).close()
               .extrude(BCLIP_W))
        base = base.union(tab)
        # hole from below to push the tab back with a small screwdriver
        base = base.cut(cq.Workplane("XY", origin=(0, 0, -1)).center(x, BCLIP_Y - 0.9)
                        .rect(BCLIP_W - 2, 1.4).extrude(BCLIP_ROOT + 1.1))
    # cable groove on top, from below the plug to the back edge
    usb_x = USB_FACE_X + PLUG_LEN / 2
    g = (cq.Workplane("XY", origin=(0, 0, BASE_H - 2.5)).center(usb_x, (40 + BASE_DEPTH) / 2 + 2)
         .slot2D(BASE_DEPTH - 40 + 8, 5.0, 90).extrude(5))
    base = base.cut(g)
    # recesses for 4 rubber feet (10 mm)
    for x in (x0 + 12, x1 - 12):
        for y in (y0 + 10, y1 - 14):
            base = base.cut(cq.Workplane("XY", origin=(0, 0, -0.01)).center(x, y).circle(5.2).extrude(1.0))
    return base


def module_in_world(step_path):
    """Waveshare module placed in the case, for a fit check."""
    m = cq.importers.importStep(step_path)
    from OCP.gp import gp_Trsf
    t = gp_Trsf()
    # model (x, y, z) -> slab (-x, LIP+3.2-z, UC-0.83-y)
    t.SetValues(-1, 0, 0, 0,
                0, 0, -1, LIP + 3.2,
                0, -1, 0, UC - 0.83)
    sh = cq.Shape.cast(m.val().wrapped.Moved(cq.Location(t).wrapped))
    return to_world(cq.Workplane().add(sh))


if __name__ == "__main__":
    import sys
    out = sys.argv[1] if len(sys.argv) > 1 else "."
    parts = {"behuizing": behuizing(), "achterklep": achterklep(), "voet": voet()}
    for n, p in parts.items():
        cq.exporters.export(p, f"{out}/{n}.stl", tolerance=0.02, angularTolerance=0.1)
        cq.exporters.export(p, f"{out}/{n}.step")
        bb = p.val().BoundingBox()
        print(n, f"{bb.xlen:.1f} x {bb.ylen:.1f} x {bb.zlen:.1f} mm")
