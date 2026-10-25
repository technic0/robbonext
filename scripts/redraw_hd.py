#!/usr/bin/env python3
#
# HD redraw generator for RobboNext.
#
# Produces the first version of the 16x16 "HD" graphics from the original
# cart, as editable indexed PNGs:
#
#   art/hd/tiles_hd.png     board tiles, same layout as the cart sheet,
#                           animation frames below them
#   art/hd/sprites_hd.png   robbo, icons, status bar, particles, digits,
#                           logo, ending scene
#   art/hd/title.png        title screen picture, 320x256
#   art/hd/ending.png       ending picture, 320x256
#
# Those PNGs are the source the build uses (scripts/convert_assets.py);
# this script only needs to run again to start over. Edit the PNGs by hand
# in any editor that keeps indexed colour (Aseprite, GrafX2, Pro Motion).
#
# Palette: index = colour * 8 + shade. Shade 3 is the colour itself, 0-2 are
# darker, 4-7 lighter. Colours 0-15 are the cart's, 16-31 extra colours
# (gold, chrome, plasma...). Index 0 is transparent. The game builds the
# real colours from these per level, so the level colour swaps of the
# original (background, walls, crates) keep working.
#
# Usage: python3 scripts/redraw_hd.py

import os
import sys

from PIL import Image

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import convert_assets as ca   # noqa: E402
import hd_art                 # noqa: E402

ROOT = ca.ROOT
OUT = os.path.join(ROOT, "art", "hd")

BG = 1
TRANSPARENT = 0

# tiles whose bright pixels light up the floor around them
GLOW = {20, 21, 22, 23, 38, 39, 40, 41, 42, 43, 44, 45, 47, 55, 56}

# richer colours for generated tiles: cart colour -> extra colour
FIRE = {10: 20, 9: 19, 8: 27}                   # yellow, orange, red
RECOLOUR = {t: FIRE for t in list(range(20, 24)) + list(range(38, 46)) + list(range(55, 60))}
RECOLOUR[46] = {8: 27, 9: 19, 10: 20}
RECOLOUR[47] = {8: 27, 9: 19, 10: 20}
RECOLOUR[16] = RECOLOUR[17] = RECOLOUR[18] = RECOLOUR[19] = {6: 17, 13: 24, 7: 17}
RECOLOUR[28] = {6: 17, 13: 24, 7: 17}


def recolour(cells, table):
    return [[None if v is None else (table.get(v[0], v[0]), v[1]) for v in row] for row in cells]

# ---------------------------------------------------------------------------
# upscaling and shading

def scale2x(g):
    """Scale2x / EPX: doubles pixel art keeping diagonals smooth."""
    h, w = len(g), len(g[0])
    o = [[0] * (w * 2) for _ in range(h * 2)]
    for y in range(h):
        for x in range(w):
            p = g[y][x]
            a = g[y - 1][x] if y > 0 else p
            b = g[y][x + 1] if x < w - 1 else p
            c = g[y][x - 1] if x > 0 else p
            d = g[y + 1][x] if y < h - 1 else p
            e0 = a if c == a and c != d and a != b else p
            e1 = b if a == b and a != c and b != d else p
            e2 = c if d == c and d != b and c != a else p
            e3 = d if b == d and b != a and d != c else p
            o[2 * y][2 * x], o[2 * y][2 * x + 1] = e0, e1
            o[2 * y + 1][2 * x], o[2 * y + 1][2 * x + 1] = e2, e3
    return o


def floor_shade(x, y):
    """Plain floor, as in the original: cells are not divided."""
    return 3


def shade(g, bg=BG, floor=True, glow=False, transparent=False):
    """Cart colours -> (colour, shade) with light from the top left.

    bg pixels become floor (or transparent for sprites), objects get a
    bevel, a glint, a soft dark rim on the floor and a drop shadow.
    """
    h, w = len(g), len(g[0])

    def inside(x, y):
        return 0 <= x < w and 0 <= y < h

    def obj(x, y):
        return inside(x, y) and g[y][x] != bg

    def col(x, y):
        return g[y][x] if inside(x, y) else -1

    out = [[None] * w for _ in range(h)]
    for y in range(h):
        for x in range(w):
            c = g[y][x]
            if c == bg:
                if transparent:
                    out[y][x] = None
                    continue
                s = floor_shade(x, y) if floor else 3
                adj = [g[b][a] for a, b in ((x - 1, y), (x + 1, y), (x, y - 1), (x, y + 1)) if obj(a, b)]
                if glow:
                    near = adj or [g[b][a] for a, b in
                                   ((x - 1, y - 1), (x + 1, y - 1), (x - 1, y + 1), (x + 1, y + 1),
                                    (x - 2, y), (x + 2, y), (x, y - 2), (x, y + 2)) if obj(a, b)]
                    if near:
                        out[y][x] = (near[0], 1 if adj else 0)
                        continue
                if adj:
                    s = 1
                elif obj(x - 1, y - 1) or obj(x - 2, y - 2) or obj(x - 1, y - 2) or obj(x - 2, y - 1):
                    s = 2
                out[y][x] = (bg, s)
                continue
            up, lf, dn, rt = col(x, y - 1), col(x - 1, y), col(x, y + 1), col(x + 1, y)
            thin_v = up != c and dn != c
            thin_h = lf != c and rt != c
            s = 3
            if not thin_v:
                s += (up != c) - (dn != c)
            if not thin_h:
                s += (lf != c) - (rt != c)
            if up != c and lf != c and not thin_v and not thin_h:
                s = 6
            if glow:
                s = max(s, 4)
            if c == 0:
                s = 3
            out[y][x] = (c, max(0, min(7, s)))
    return out


def floorize(cells, glow=False):
    """Fill the empty (None) cells of a drawing with floor, rim and shadow."""
    h, w = len(cells), len(cells[0])

    def obj(x, y):
        return 0 <= x < w and 0 <= y < h and cells[y][x] is not None

    out = [row[:] for row in cells]
    for y in range(h):
        for x in range(w):
            if cells[y][x] is not None:
                continue
            adj = [cells[b][a] for a, b in ((x - 1, y), (x + 1, y), (x, y - 1), (x, y + 1)) if obj(a, b)]
            if glow and adj:
                out[y][x] = (adj[0][0], 1)
            elif adj:
                out[y][x] = (BG, 1)
            elif obj(x - 1, y - 1) or obj(x - 2, y - 2) or obj(x - 1, y - 2) or obj(x - 2, y - 1):
                out[y][x] = (BG, 2)
            else:
                out[y][x] = (BG, floor_shade(x, y))
    return out


def to_indices(cells):
    # (0, 0) is the transparent index: use the next darkest black instead
    return [[TRANSPARENT if v is None else max(1, v[0] * 8 + v[1]) for v in row]
            for row in cells]


def save_indexed(path, w, h, blocks, fill=TRANSPARENT):
    """blocks: list of (x, y, 2D list of indices)."""
    im = Image.new("P", (w, h), fill)
    im.putpalette(ca.preview_palette())
    for bx, by, data in blocks:
        for y, row in enumerate(data):
            for x, v in enumerate(row):
                im.putpixel((bx + x, by + y), v)
    im.save(path, transparency=TRANSPARENT)

# ---------------------------------------------------------------------------
# animation frames: variations of a finished tile (list of (colour, shade))

def bright(cell, d):
    c, s = cell
    return (c, max(0, min(7, s + d)))


def frame_of(t, base, f, n):
    g = [row[:] for row in base]
    if t == 1:
        # screw: a glint runs diagonally over the metal in frames 1-3
        if f <= 3:
            band = f * 7 - 4
            for y in range(16):
                for x in range(16):
                    c, s = g[y][x]
                    if c in (13, 12, 7, 17, 24) and band <= x + y <= band + 3:
                        g[y][x] = (17, 7)
    elif t in (14, 15):
        # capsule: its lights chase around
        for y in range(16):
            for x in range(16):
                c, s = g[y][x]
                if c in (9, 10, 11, 12, 8, 19, 20) and y < 12:
                    g[y][x] = (c, 7 if (x // 3) % n == f else max(2, s - 1))
    elif 20 <= t <= 23:
        # laser head: the core pulses
        d = (0, 1, 2, 1)[f]
        for y in range(16):
            for x in range(16):
                if g[y][x][0] in (9, 10, 7, 19, 20):
                    g[y][x] = bright(g[y][x], d)
    elif t in (46, 47):
        # teleport: sparkles turn around the centre
        import math
        for k in range(3):
            a = (f / n + k / 3) * 2 * math.pi
            x = int(round(7.5 + 5 * math.cos(a)))
            y = int(round(7.5 + 5 * math.sin(a)))
            g[y][x] = (7, 7)
    elif t == 63:
        # void: deep specks twinkle
        for i, (x, y) in enumerate(((2, 3), (11, 2), (6, 9), (13, 12), (4, 14), (9, 6))):
            if (i + f) % n == 0:
                g[y][x] = (4, 5)
    elif t in (119, 120):
        # barrier: the waves flow sideways
        sh = f * 4
        g = [[row[(x + sh) % 16] for x in range(16)] for row in base]
    return g

# ---------------------------------------------------------------------------
# sprites without an original in the cart

def panel(part):
    """Status bar panel: a bevelled steel bar."""
    g = [[None] * 16 for _ in range(16)]
    for y in range(1, 15):
        for x in range(16):
            if part == "L" and x < 2 and y in (1, 14):
                continue
            if part == "R" and x > 13 and y in (1, 14):
                continue
            s = 2
            if y == 1:
                s = 5
            elif y == 2:
                s = 4
            elif y == 14:
                s = 0
            elif y == 13:
                s = 1
            if part == "L" and x == 0 or part == "R" and x == 15:
                s = min(s, 1) if part == "R" else max(s, 4)
            g[y][x] = (24, s)
    return g


def particle(kind):
    g = [[None] * 16 for _ in range(16)]

    def dot(x, y, v):
        if 0 <= x < 16 and 0 <= y < 16:
            g[y][x] = v

    if kind == "SPARK":
        for x, y, s in ((7, 7, 7), (8, 7, 7), (7, 8, 7), (8, 8, 7), (6, 7, 5), (9, 8, 5),
                        (7, 6, 5), (8, 9, 5), (5, 7, 3), (10, 8, 3)):
            dot(x, y, (20, s))
    elif kind == "SPARK2":
        for x, y, s in ((7, 7, 6), (8, 8, 6), (8, 7, 4), (7, 8, 4)):
            dot(x, y, (19, s))
    elif kind == "DEBRIS":
        for y in range(6, 10):
            for x in range(6, 10):
                dot(x, y, (4, 5 if y == 6 or x == 6 else 1 if y == 9 or x == 9 else 3))
    elif kind == "DEBRIS2":
        for x, y, s in ((7, 7, 5), (8, 7, 3), (7, 8, 3), (8, 8, 1), (9, 8, 2)):
            dot(x, y, (4, s))
    elif kind in ("SMOKE", "SMOKE2"):
        r = 4.5 if kind == "SMOKE" else 3.0
        for y in range(16):
            for x in range(16):
                d = ((x - 7.5) ** 2 + (y - 7.5) ** 2) ** 0.5
                if d < r:
                    s = 5 if d < r * 0.4 else 4 if d < r * 0.75 else 3
                    if (x + y) % 2 and d > r * 0.75:
                        continue
                    dot(x, y, (6 if kind == "SMOKE" else 5, s))
    elif kind in ("STAR", "STAR2"):
        arm = 3 if kind == "STAR" else 2
        dot(7, 7, (20, 7))
        for i in range(1, arm + 1):
            s = 6 - i
            for x, y in ((7 - i, 7), (7 + i, 7), (7, 7 - i), (7, 7 + i)):
                dot(x, y, (16, s))
    return g


def digits(font):
    """Status bar digits, 8x16: the Spectrum font doubled in height, chrome."""
    out = []
    for d in range(10):
        glyph = font[(16 + d) * 8:(17 + d) * 8]
        g = [[None] * 8 for _ in range(16)]
        for y in range(16):
            for x in range(8):
                if glyph[y // 2] & (0x80 >> x):
                    g[y][x] = (17, 7 - y // 3 if y < 12 else 3)
                elif y > 0 and x > 0 and glyph[(y - 1) // 2] & (0x80 >> (x - 1)):
                    g[y][x] = (30, 0)          # shadow
        out.append(g)
    return out

# ---------------------------------------------------------------------------
# pictures

def text(img, font, x, y, s, colour, shade_top=6, shadow=True):
    """Draw text into a 2D list of indices (8x8 font, gradient, shadow)."""
    for i, ch in enumerate(s):
        glyph = font[(ord(ch) - 32) * 8:(ord(ch) - 31) * 8]
        for gy in range(8):
            for gx in range(8):
                if glyph[gy] & (0x80 >> gx):
                    px, py = x + i * 8 + gx, y + gy
                    if shadow:
                        img[py + 1][px + 1] = 30 * 8 + 0
                    img[py][px] = colour * 8 + max(2, shade_top - gy // 2)


def sky(w, h, seed):
    """Deep space gradient with ordered dithering and stars."""
    import random
    rnd = random.Random(seed)
    bayer = ((0, 8, 2, 10), (12, 4, 14, 6), (3, 11, 1, 9), (15, 7, 13, 5))
    img = [[0] * w for _ in range(h)]
    for y in range(h):
        t = y / (h - 1)
        level = 0.4 + t * 3.2              # deep space shades 0..3, then purple
        for x in range(w):
            v = level + bayer[y % 4][x % 4] / 16 - 0.5
            if v < 3:
                img[y][x] = 30 * 8 + max(0, int(v))
            else:
                img[y][x] = 31 * 8 + min(3, int(v - 2))
    for _ in range(int(w * h / 600)):
        x, y = rnd.randrange(w), rnd.randrange(h)
        img[y][x] = 26 * 8 + rnd.choice((2, 3, 4, 5, 6, 7))
    return img


def sphere(img, cx, cy, r, colour, rim):
    for y in range(max(0, cy - r), min(len(img), cy + r + 1)):
        for x in range(max(0, cx - r), min(len(img[0]), cx + r + 1)):
            dx, dy = (x - cx) / r, (y - cy) / r
            d2 = dx * dx + dy * dy
            if d2 > 1:
                continue
            dz = (1 - d2) ** 0.5
            light = max(0.0, -0.55 * dx - 0.6 * dy + 0.58 * dz)
            s = int(light * 7.5)
            if (x + y) % 2 and s < 7:
                s += int((light * 7.5 - s) > 0.5)
            img[y][x] = colour * 8 + max(1, min(7, s))
            if d2 > 0.9 and dx + dy < 0:
                img[y][x] = rim * 8 + 4


def band(img, y0, y1, colour, shade):
    for y in range(y0, y1):
        img[y] = [colour * 8 + shade] * len(img[0])


def title_picture(font):
    img = sky(320, 256, 1)
    sphere(img, 268, 236, 92, 12, 18)
    sphere(img, 26, 30, 13, 25, 20)
    # tinted glass window behind the scrolling instructions
    for y in range(108, 220):
        for x in range(16, 304):
            v = img[y][x]
            c, s = v >> 3, v & 7
            if c in (12, 18):
                img[y][x] = 30 * 8 + min(4, 1 + s // 2)     # planet: dark silhouette
            else:
                img[y][x] = (v & ~7) | max(1, s - 4)
    for x in range(16, 304):
        img[107][x] = 24 * 8 + 4
        img[220][x] = 24 * 8 + 1
    for y in range(107, 221):
        img[y][15] = 24 * 8 + 4
        img[y][304] = 24 * 8 + 1
    for y in (102, 224):
        img[y] = [8 * 8 + 5] * 320
        img[y + 1] = [8 * 8 + 2] * 320
    band(img, 228, 256, 30, 0)

    def centred(y, s, colour, shade_top=6):
        text(img, font, 160 - len(s) * 4, y, s, colour, shade_top)

    centred(58, "(C) 1989 BY LK Avalon", 7)
    centred(68, "JANUSZ PELC", 7)
    centred(86, "ZX Spectrum Next version", 13, 5)
    return img


def ending_picture(font, land):
    img = sky(320, 256, 7)
    sphere(img, 262, 146, 22, 25, 19)
    # land: the HD land tile repeated, then soil down to the bottom
    for x0 in range(0, 320, 16):
        for y in range(32):
            for x in range(16):
                v = land[y][x]
                if v != TRANSPARENT:
                    img[180 + y][x0 + x] = v
    for y in range(212, 256):
        for x in range(320):
            img[y][x] = 4 * 8 + max(1, 3 - (y - 212) // 12 - ((x * 7 + y * 3) % 5 == 0))
    return img

# ---------------------------------------------------------------------------

def main():
    rom = ca.load_rom(ca.CART)
    pixel = ca.make_pixel(rom)
    font = open(os.path.join(ROOT, "res", "font.bin"), "rb").read()

    def src(n, w=1, h=1):
        return [[pixel(n, x, y) for x in range(8 * w)] for y in range(8 * h)]

    os.makedirs(OUT, exist_ok=True)

    # board tiles and their animation frames
    cells_of = {}
    blocks = []
    for t in ca.BOARD_TILES:
        if t in hd_art.TILES:
            cells = floorize(hd_art.TILES[t], glow=t in GLOW)
        elif t in ca.BLANK_TILES:
            cells = shade([[BG] * 16 for _ in range(16)])
        else:
            cells = shade(scale2x(src(t)), glow=t in GLOW)
        if t in RECOLOUR:
            cells = recolour(cells, RECOLOUR[t])
        cells_of[t] = cells
        blocks.append(((t % 16) * 16, (t // 16) * 16, to_indices(cells)))
    extra = []
    for t, f in ca.anim_frames():
        extra.append(frame_of(t, cells_of[t], f, ca.ANIM[t]))
    for i, cells in enumerate(extra):
        blocks.append(((i % 16) * 16, 128 + (i // 16) * 16, to_indices(cells)))
    save_indexed(os.path.join(OUT, "tiles_hd.png"), 256, 256, blocks, fill=1)

    # sprites (layout shared with convert_assets.SPRITE_LAYOUT)
    blocks = []
    for st, name, x, y, w, h, n, tw, th, key in ca.SPRITE_LAYOUT:
        if name in hd_art.SPRITES:
            cells = hd_art.SPRITES[name]
        elif name.startswith("PANEL_"):
            cells = panel(name[-1])
        elif n is None:
            cells = particle(name)
        else:
            cells = shade(scale2x(src(n, tw, th)), bg=key, transparent=True)
        blocks.append((x, y, to_indices(cells)))
    for d, g in enumerate(digits(font)):
        blocks.append((ca.DIGITS_X + d * 8, ca.DIGITS_Y, to_indices(g)))
    save_indexed(os.path.join(OUT, "sprites_hd.png"), 256, 128, blocks)

    # pictures
    land = to_indices(shade(scale2x(src(105, 1, 2)), bg=0, transparent=True))
    save_indexed(os.path.join(OUT, "title.png"), 320, 256, [(0, 0, title_picture(font))])
    save_indexed(os.path.join(OUT, "ending.png"), 320, 256, [(0, 0, ending_picture(font, land))])
    print("wrote art/hd/tiles_hd.png, sprites_hd.png, title.png, ending.png")


if __name__ == "__main__":
    main()
