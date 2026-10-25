#!/usr/bin/env python3
#
# Asset converter for RobboNext.
#
# Reads the HD art (art/hd/*.png, art/splash) and produces the graphics the
# Next build needs, plus the tables in src/gen_assets.h:
#
#   res/tiles.bin                   board tiles and animation frames, 16x16
#                                   at 8 bits per pixel, column by column
#                                   (the layout of the 320x256 layer 2)
#   res/spr_{set}.bin               sprite patterns of each screen
#   res/digits.bin                  status bar digits, 8x16
#   res/{title,ending}_{a,b}.bin    320x256 pictures, column by column
#   res/splash_{a,b}.bin, splash.npl the opening picture (art/splash), as is
#   res/tm_font.bin                 4-bit tilemap patterns of the font
#   res/ramps.bin                   8 shades of each colour, 9-bit RGB
#   src/gen_assets.h                tile tables, tile flags, sprite ids
#
# Source data kept as is in res/ (taken from bocianu's version, with the
# levels of the 1989 original):
#
#   res/levels.bin                  56 levels x 512 bytes
#   res/sound.bin                   music + sfx 0..25
#   res/flags.bin                   flags of the 128 tiles
#
# Pixels are palette indices: colour * 8 + shade (shade 3 is the colour
# itself). Colours 0-15 are the base ones, 16-31 extra colours of the HD
# art. Index 0 is transparent in sprites and never used as a colour.
#
# Usage: python3 scripts/convert_assets.py

import os
import sys

from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RES = os.path.join(ROOT, "res")
SRC = os.path.join(ROOT, "src")

# ---------------------------------------------------------------------------
# colours

BASE_RGB = [
    (0, 0, 0), (29, 43, 83), (126, 37, 83), (0, 135, 81),
    (171, 82, 54), (95, 87, 79), (194, 195, 199), (255, 241, 232),
    (255, 0, 77), (255, 163, 0), (255, 236, 39), (0, 228, 54),
    (41, 173, 255), (131, 118, 156), (255, 119, 168), (255, 204, 170),
    # secret palette, colours 128-143
    (41, 24, 20), (17, 29, 53), (66, 33, 54), (18, 83, 89),
    (116, 47, 41), (73, 51, 59), (162, 136, 121), (243, 239, 125),
    (190, 18, 80), (255, 108, 36), (168, 231, 46), (0, 181, 67),
    (6, 90, 181), (117, 70, 101), (255, 110, 89), (255, 157, 129),
]

# extra colours 16-31 of the HD art (not changed per level)
EXTRA_RGB = [
    (255, 196, 40),     # 16 gold
    (200, 214, 236),    # 17 chrome
    (60, 220, 255),     # 18 plasma
    (255, 110, 20),     # 19 fire
    (255, 240, 120),    # 20 hot yellow
    (240, 50, 200),     # 21 magenta energy
    (30, 210, 110),     # 22 emerald
    (140, 80, 255),     # 23 violet
    (100, 120, 160),    # 24 steel
    (205, 110, 55),     # 25 copper
    (200, 235, 255),    # 26 ice / stars
    (220, 35, 25),      # 27 lava
    (170, 255, 60),     # 28 lime
    (255, 130, 170),    # 29 rose
    (14, 22, 64),       # 30 deep space
    (52, 22, 82),       # 31 night purple
]

# lighter shades brighten along the colour's own hue towards a warm white,
# darker shades fade towards a cool near-black
LIGHT = (255, 246, 220)
DARK = (14, 8, 32)
TO_LIGHT = (0.18, 0.36, 0.56, 0.78)     # shades 4-7
TO_DARK = (0.28, 0.52, 0.74)            # shades 2, 1, 0


def light_target(rgb):
    top = max(rgb)
    if top < 8:
        return (96, 96, 128)            # black: neutral highlights
    hue = [v * 255 / top for v in rgb]
    return tuple(h + (l - h) * 0.35 for h, l in zip(hue, LIGHT))


def ramp(rgb):
    """8 shades of a colour, quantized to the Next's 3 bits per channel."""
    out = []
    for s in range(8):
        if s > 3:
            t, target = TO_LIGHT[s - 4], light_target(rgb)
        elif s < 3:
            t, target = TO_DARK[2 - s], DARK
        else:
            t, target = 0, rgb
        c = [a + (b - a) * t for a, b in zip(rgb, target)]
        out.append(tuple(round(v * 7 / 255) * 255 // 7 for v in c))
    return out


def colour_rgb(c):
    """RGB of a colour number: 0-15 base, 16-31 extra, 128-143 secret."""
    if c & 0x80:
        return BASE_RGB[16 + (c & 15)]
    if c >= 16:
        return EXTRA_RGB[c - 16]
    return BASE_RGB[c]


def rgb9(rgb):
    r, g, b = (round(v * 7 / 255) for v in rgb)
    v = (r << 6) | (g << 3) | b
    # never produce the global transparency colour (0xE3 in RGB332)
    if v >> 1 == 0xE3:
        v ^= 0x08
    return v


def build_ramps():
    """48 ramps: base 0-15, secret 128-143, extra 16-31 (in that order)."""
    data = bytearray()
    for rgb in BASE_RGB + EXTRA_RGB:
        for c in ramp(rgb):
            v = rgb9(c)
            data += bytes((v >> 1, v & 1))
    return bytes(data)


def preview_palette(level=None):
    """Palette for the indexed PNGs: index = colour * 8 + shade."""
    pal = []
    for c in range(32):
        for rgb in ramp(colour_rgb(c)):
            pal.extend(rgb)
    pal[0:3] = [255, 0, 255]            # index 0: transparent, shown magenta
    return pal

# ---------------------------------------------------------------------------
# graphics layout

TRANSPARENT = 0

# map tiles that appear on the board (robbo frames 48-54 are sprites)
BOARD_TILES = (list(range(0, 48)) + list(range(55, 64)) +
               list(range(96, 105)) + list(range(112, 121)))
BLANK_TILES = (0, 62)       # drawn as floor only

# extra animation frames of the tiles (the frame count includes the tile
# itself). They are stored in tiles_hd.png below the tiles, in this order.
ANIM = {
    1: 8,                           # screw: a glint runs over it
    14: 4, 15: 4,                   # capsule: lights chase around
    20: 4, 21: 4, 22: 4, 23: 4,     # laser heads pulse
    46: 4, 47: 4,                   # teleports swirl
    63: 4,                          # void: deep specks twinkle
    119: 4, 120: 4,                 # barrier flows
}


def anim_frames():
    """(tile, frame) of every extra frame, in storage order."""
    return [(t, f) for t in ANIM for f in range(1, ANIM[t])]


# sprite sheet: set, name, x, y, width, height
SPRITE_LAYOUT = (
    [("game", "ROBBO_%d" % n, (n - 48) * 16, 0, 16, 16) for n in range(48, 55)] +
    [("game", "ICON_SCREW", 112, 0, 16, 16),
     ("game", "ICON_LIFE", 128, 0, 16, 16),
     ("game", "ICON_KEY", 144, 0, 16, 16),
     ("game", "ICON_AMMO", 160, 0, 16, 16),
     ("game", "ICON_LEVEL", 176, 0, 16, 16),
     ("game", "PANEL_L", 192, 0, 16, 16),
     ("game", "PANEL_M", 208, 0, 16, 16),
     ("game", "PANEL_R", 224, 0, 16, 16),
     ("game", "SPARK", 0, 16, 16, 16),
     ("game", "SPARK2", 16, 16, 16, 16),
     ("game", "DEBRIS", 32, 16, 16, 16),
     ("game", "DEBRIS2", 48, 16, 16, 16),
     ("game", "SMOKE", 64, 16, 16, 16),
     ("game", "SMOKE2", 80, 16, 16, 16),
     ("game", "STAR", 96, 16, 16, 16),
     ("game", "STAR2", 112, 16, 16, 16),
     ("title", "LOGO_R", 0, 32, 32, 32),
     ("title", "LOGO_O", 32, 32, 32, 32),
     ("title", "LOGO_B", 64, 32, 32, 32),
     ("ending", "BIG_74", 0, 64, 32, 32),
     ("ending", "BIG_76", 32, 64, 32, 32),
     ("ending", "BIG_78", 64, 64, 32, 32),
     ("ending", "BIG_106", 96, 64, 32, 32),
     ("ending", "BIG_108", 128, 64, 32, 32),
     ("ending", "UFO", 0, 96, 64, 32),
     ("ending", "UFO_FEET", 64, 96, 32, 16)]
)
SPRITE_SETS = ("game", "title", "ending")

# status bar digits, 8x16 each, in sprites_hd.png
DIGITS_X, DIGITS_Y = 128, 16


def blocks16(grid, w, h):
    """Split a w x h grid of indices into 16x16 patterns, row by row."""
    out = []
    for by in range(0, h, 16):
        for bx in range(0, w, 16):
            out.append(bytes(grid[by + y][bx + x] for y in range(16) for x in range(16)))
    return out


def columns(grid, x0, y0, w, h):
    """Bytes of a region column by column (layer 2 320x256 layout)."""
    return bytes(grid[y0 + y][x0 + x] for x in range(w) for y in range(h))


def read_indexed(path, size=None):
    im = Image.open(path)
    if im.mode != "P":
        sys.exit("%s must be an indexed (palette) image" % path)
    if size and im.size != size:
        sys.exit("%s must be %dx%d" % ((path,) + size))
    w, h = im.size
    data = list(im.getdata())
    return [data[y * w:(y + 1) * w] for y in range(h)]

# ---------------------------------------------------------------------------
# HD graphics from the indexed PNGs

def hd_tiles():
    sheet = read_indexed(os.path.join(ROOT, "art", "hd", "tiles_hd.png"), (256, 256))
    data = b""
    places = [((t % 16) * 16, (t // 16) * 16) for t in BOARD_TILES]
    extra = len(anim_frames())
    places += [((i % 16) * 16, 128 + (i // 16) * 16) for i in range(extra)]
    for x0, y0 in places:
        block = columns(sheet, x0, y0, 16, 16)
        if TRANSPARENT in block:
            sys.exit("tiles_hd.png: tile at %d,%d uses index 0" % (x0, y0))
        data += block
    return data


def hd_sprites(sheet, which):
    data = b""
    for st, name, x, y, w, h in SPRITE_LAYOUT:
        if st == which:
            g = [sheet[y + py][x:x + w] for py in range(h)]
            data += b"".join(blocks16(g, w, h))
    return data


def hd_digits(sheet):
    return bytes(sheet[DIGITS_Y + y][DIGITS_X + d * 8 + x]
                 for d in range(10) for y in range(16) for x in range(8))


def picture(name):
    sheet = read_indexed(os.path.join(ROOT, "art", "hd", name), (320, 256))
    return columns(sheet, 0, 0, 320, 256)

# ---------------------------------------------------------------------------
# tilemap patterns (4 bits per pixel, 8x8, 32 bytes)

def pack4(pixels):
    return bytes((pixels[i] << 4) | pixels[i + 1] for i in range(0, len(pixels), 2))


def tm_font(font):
    """Characters 32-127; pixel 1 is the letter, 2 its shadow."""
    data = b""
    for ch in range(96):
        glyph = font[ch * 8:ch * 8 + 8]
        on = lambda x, y: 0 <= x < 8 and 0 <= y < 8 and glyph[y] & (0x80 >> x)
        px = []
        for y in range(8):
            for x in range(8):
                px.append(1 if on(x, y) else 2 if on(x - 1, y - 1) else 0)
        data += pack4(px)
    return data



# ---------------------------------------------------------------------------
# C header

def write_header(path, flags):
    slots = [0] * 128
    for i, t in enumerate(BOARD_TILES):
        slots[t] = i
    for t in BLANK_TILES:
        slots[t] = slots[0]
    frames = [1] * 128
    first = [0] * 128
    slot = len(BOARD_TILES)
    for t, n in ANIM.items():
        frames[t] = n
        first[t] = slot
        slot += n - 1
    lines = [
        "// Generated by scripts/convert_assets.py - do not edit.",
        "",
        "#ifndef _gen_assets_h_",
        "#define _gen_assets_h_",
        "",
        "#define TILE_SLOTS %d" % slot,
        "",
    ]
    for which in SPRITE_SETS:
        pattern = 0
        for st, name, x, y, w, h in SPRITE_LAYOUT:
            if st == which:
                lines.append("#define SPR_%s %d" % (name, pattern))
                pattern += (w // 16) * (h // 16)
        lines.append("#define SPRITES_%s %d" % (which.upper(), pattern))
        if pattern > 64:
            sys.exit("too many 8-bit sprite patterns in %s: %d" % (which, pattern))
    lines += ["", "#ifdef GEN_ASSETS_DATA", ""]

    def table(name, values, comment):
        out = ["// " + comment, "const unsigned char %s[128] = {" % name]
        for r in range(0, 128, 16):
            out.append("    " + ",".join("%3d" % v for v in values[r:r + 16]) + ",")
        return out + ["};", ""]

    lines += table("tile_slots", slots, "slot of each map tile in the tile graphics")
    lines += table("tile_frames", frames, "animation frames of each tile")
    lines += table("tile_frame_slot", first, "slot of the second frame")
    lines += table("tile_flags", list(flags), "tile flags")
    lines += ["#else", ""]
    for name in ("tile_slots", "tile_frames", "tile_frame_slot", "tile_flags"):
        lines.append("extern const unsigned char %s[128];" % name)
    lines += ["", "#endif", "", "#endif"]
    with open(path, "w") as f:
        f.write("\n".join(lines) + "\n")

# ---------------------------------------------------------------------------

def main():
    font = open(os.path.join(RES, "font.bin"), "rb").read()
    flags = open(os.path.join(RES, "flags.bin"), "rb").read()

    sprites = read_indexed(os.path.join(ROOT, "art", "hd", "sprites_hd.png"))
    outputs = {
        "tiles.bin": hd_tiles(),
        "digits.bin": hd_digits(sprites),
        # pictures in two halves: an assembler section is at most 64K
        "title_a.bin": picture("title.png")[:40960],
        "title_b.bin": picture("title.png")[40960:],
        "ending_a.bin": picture("ending.png")[:40960],
        "ending_b.bin": picture("ending.png")[40960:],
        "tm_font.bin": tm_font(font),
        "ramps.bin": build_ramps(),
    }
    # opening picture: already layer 2 data (320x256, column by column)
    splash = open(os.path.join(ROOT, "art", "splash", "robbo_320x256.sl2"), "rb").read()
    if len(splash) != 81920:
        sys.exit("art/splash/robbo_320x256.sl2 must be 81920 bytes")
    outputs["splash_a.bin"] = splash[:40960]
    outputs["splash_b.bin"] = splash[40960:]
    outputs["splash.npl"] = open(os.path.join(ROOT, "art", "splash", "robbo_320x256.npl"), "rb").read()
    for which in SPRITE_SETS:
        outputs["spr_%s.bin" % which] = hd_sprites(sprites, which)
    for name, data in outputs.items():
        with open(os.path.join(RES, name), "wb") as f:
            f.write(data)
        print("res/%-22s %6d bytes" % (name, len(data)))

    write_header(os.path.join(SRC, "gen_assets.h"), flags)
    print("src/gen_assets.h")


if __name__ == "__main__":
    main()
