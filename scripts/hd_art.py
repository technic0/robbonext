#
# Hand drawn HD tiles and sprites for scripts/redraw_hd.py.
#
# Each drawing is 16 strings of 16 characters. '.' is empty: floor on a
# board tile (redraw_hd.py adds the floor plate, the dark rim around the
# object and its drop shadow), transparent on a sprite. Every other
# character is looked up in the drawing's legend, which maps it to a
# base colour and a shade (0 darkest, 3 = the colour itself, 7 lightest).
#
# Light comes from the top left. Colours keep the cart's originals, so the
# per level colour swaps (walls 2/14/15, crates 4) still apply.
#
# Tiles not listed here are generated from the cart sprite.
#


def draw(rows, legend):
    assert len(rows) == 16, len(rows)
    out = []
    for r in rows:
        assert len(r) == 16, r
        out.append([None if ch == "." else legend[ch] for ch in r])
    return out


# metal: chrome (17) with a blue tint (12), steel (24) shadows
METAL = {
    "W": (17, 7),
    "H": (12, 5), "h": (12, 4),
    "A": (17, 5), "a": (17, 4), "b": (17, 3), "c": (24, 3),
    "d": (24, 2), "e": (24, 1), "f": (24, 0),
    "k": (0, 3),
}

# screw seen from the side: slotted head, threaded shank, pointed tip
SCREW = draw([
    "................",
    "....WAAAAAAa....",
    "...WAAAAAAAAab..",
    "...AAAkkkkkAbb..",
    "...aAAAAAAAAbc..",
    "....bbbbbbbbc...",
    "......HAAbc.....",
    ".....WAAAAbc....",
    "......dccdd.....",
    ".....WAAAAbc....",
    "......dccdd.....",
    ".....WAAAbc.....",
    "......dccd......",
    "......HAbc......",
    ".......Ab.......",
    "........d.......",
], METAL)

# bomb: colour 4 (crate colour, changes per level) with a grey fuse
BOMB = draw([
    "................",
    "..........WA....",
    ".........Ab.d...",
    "........dc......",
    ".....dddcdd.....",
    "...44445444444..",
    "..4677544444443.",
    "..46654444444432",
    ".44554444444433.",
    ".44444444444432.",
    ".44444444444332.",
    ".44444444443322.",
    "..444444443322..",
    "..3444443332221.",
    "....33333222....",
    "................",
], {
    "W": (20, 7), "A": (19, 5), "b": (6, 3), "c": (5, 4), "d": (5, 2),
    "7": (4, 7), "6": (4, 6), "5": (4, 5), "4": (4, 3), "3": (4, 2),
    "2": (4, 1), "1": (4, 0),
})

# key: colours 10 (yellow), 9 (orange), 4 (brown)
KEY = draw([
    "................",
    "....YYYYYYY.....",
    "...YWYooooYY4...",
    "..YYokkkkkoY44..",
    "..Yok....koo4...",
    "..Yok....ko44...",
    "..YYokkkkoY44...",
    "...YYYoooY44....",
    ".....YYo44......",
    ".....Yo44.......",
    ".....Yo44YY.....",
    ".....Yo4444.....",
    ".....Yo44.......",
    ".....Yo44YY.....",
    ".....YYo4444....",
    "......4444......",
], {
    "W": (20, 7), "Y": (16, 5), "o": (16, 3), "4": (25, 2), "k": (0, 3),
})

# ammo: a cartridge lying on its side, colours 6 and 13 case, 10 and 9 tip
AMMO = draw([
    "................",
    "................",
    "................",
    "................",
    "................",
    "...cWWWWWWWYYo..",
    "..ccAAAAAAAYYYo.",
    "..cbAAAAAAAYYYYo",
    "..cbaaaaaaaoooo.",
    "..ccbbbbbbbooo..",
    "...cddddddddo...",
    "................",
    "................",
    "................",
    "................",
    "................",
], {
    "W": (17, 7), "A": (17, 5), "a": (17, 4), "b": (17, 3),
    "c": (24, 2), "d": (24, 1),
    "Y": (16, 5), "o": (16, 3),
})

# extra life: robbo's head, colours 12, 13, 11 (antennae) and 0
LIFE = draw([
    "................",
    "....g......g....",
    "....G......G....",
    ".....g....g.....",
    "....kHHHHHHk....",
    "...HWHHHHHHhh...",
    "...HkkHHHHkkh...",
    "...HkGHHHHkGh...",
    "...hHHHHHHHhh...",
    "...aaaaaaaaab...",
    "...abbkkkkbbc...",
    "...abbbbbbbbc...",
    "....bcccccccc...",
    "....kkkkkkkkk...",
    "................",
    "................",
], {
    "W": (7, 6), "H": (12, 4), "h": (12, 3), "G": (11, 5), "g": (11, 3),
    "a": (13, 4), "b": (13, 3), "c": (13, 2), "k": (0, 3),
})

# question mark: colours 7, 10, 9 and black shadow steps
QUESTION = draw([
    "................",
    ".....WWYYYY.....",
    "....WYYYYYYo....",
    "...WYYookYYYo...",
    "...YYok..kYYo...",
    "...kkk...WYYo...",
    ".........YYok...",
    "........WYYo....",
    ".......WYYok....",
    ".......YYok.....",
    ".......YYo......",
    ".......kkk......",
    ".......WYo......",
    ".......YYo......",
    ".......kkk......",
    "................",
], {
    "W": (20, 6), "Y": (16, 5), "o": (16, 3), "k": (0, 3),
})

# ---------------------------------------------------------------------------
# full cell textures, drawn with a few helpers


def blank():
    return [[None] * 16 for _ in range(16)]


def crate(accent):
    """A wooden crate in the crate colour (4) with a braced panel."""
    g = blank()
    for y in range(1, 15):
        for x in range(1, 15):
            top, left = y - 1, x - 1
            bottom, right = 14 - y, 14 - x
            if min(top, left, bottom, right) < 2:
                # frame: lit from the top left
                if top == 0 or left == 0:
                    s = 6
                elif bottom == 0 or right == 0:
                    s = 1
                elif top == 1 or left == 1:
                    s = 4
                else:
                    s = 2
                g[y][x] = (4, s)
            else:
                # inner panel with horizontal planks
                s = 3 if (y - 3) % 4 else 2
                if x == 3 or y == 3:
                    s = 1       # inner shadow under the frame
                g[y][x] = (4, s)
    # diagonal brace in the accent colour
    for i in range(3, 13):
        x, y = i, 15 - i
        for dx, sh in ((-1, 5), (0, 3), (1, 2)):
            if 3 <= x + dx <= 12 and 3 <= y <= 12:
                g[y][x + dx] = (accent, sh)
    # nails in the corners
    for x, y in ((2, 2), (13, 2), (2, 13), (13, 13)):
        g[y][x] = (6, 5)
    return g


def door():
    """Planked door in the crate colour with the wall colour between planks."""
    g = blank()
    for y in range(16):
        for x in range(16):
            plank = x % 4
            if plank == 3:
                v = (2, 2)                      # gap between planks
            else:
                s = 4 if plank == 0 else 3
                if y == 0:
                    s = 6
                elif y == 15:
                    s = 1
                if (y * 7 + x * 3) % 11 == 0:
                    s -= 1                      # wood grain
                v = (4, s)
            g[y][x] = v
    # hinges and handle
    for y in (3, 12):
        for x in range(0, 7):
            g[y][x] = (5, 4 if x < 6 else 2)
            g[y + 1][x] = (5, 2)
    for y in range(6, 10):
        g[y][12] = (6, 5 if y == 6 else 3)
        g[y][13] = (13, 2)
    g[8][12] = (0, 3)                           # keyhole
    return g


def rubble():
    """Loose stones on the floor, in the crate colour."""
    g = blank()
    stones = [(2, 1, 4, 3), (8, 0, 5, 4), (13, 3, 3, 3), (0, 6, 4, 4), (5, 5, 5, 4),
              (11, 8, 4, 4), (2, 11, 5, 4), (8, 12, 4, 3), (13, 13, 3, 3)]
    for sx, sy, w, h in stones:
        for y in range(h):
            for x in range(w):
                # round the corners
                if (x in (0, w - 1)) and (y in (0, h - 1)):
                    continue
                px, py = sx + x, sy + y
                if px > 15 or py > 15:
                    continue
                s = 3
                if y == 0 or x == 0:
                    s = 5
                if y == h - 1 or x == w - 1:
                    s = 1
                if x == 1 and y == 1 and w > 3 and h > 3:
                    s = 6
                g[py][px] = (4, s)
    return g


def void():
    """Bottomless rock in the crate colour, with faint cracks."""
    g = blank()
    for y in range(16):
        for x in range(16):
            s = 3
            n = (x * 37 + y * 91 + x * y * 13) % 29
            if n < 3:
                s = 2
            elif n == 7:
                s = 4
            g[y][x] = (4, s)
    for x, y in ((3, 4), (4, 5), (5, 5), (6, 6), (11, 10), (12, 11), (12, 12), (13, 13), (9, 2), (10, 2)):
        g[y][x] = (4, 1)
    return g


# ---------------------------------------------------------------------------
# creatures, in the crate colour (4), which changes per level

BUG = {
    "7": (4, 6), "6": (4, 5), "5": (4, 4), "4": (4, 3), "3": (4, 2), "2": (4, 1),
    "E": (6, 6), "e": (6, 4), "k": (0, 3),
    "B": (5, 4), "b": (5, 3), "D": (13, 4), "d": (13, 3),
}

CREATURE_L1 = draw([
    "................",
    "...55.....55....",
    "..5665...5664...",
    "..56666666664...",
    "..6Ek666Ek664...",
    "..66666666643...",
    "..3445BBB5433...",
    "445.4BDDDB4..33.",
    "...54BDdDB43..33",
    "..554BDdDB4433..",
    ".55..4BDB43.....",
    "......4b43......",
    "...554.....443..",
    "..55.........33.",
    "................",
    "................",
], BUG)

CREATURE_L2 = draw([
    "................",
    "....55.....55...",
    "...5665...5664..",
    "...56666666664..",
    "...6Ek666Ek664..",
    "...66666666643..",
    "...3445BBB5433..",
    ".55..4BDDDB4.433",
    "55..54BDdDB43...",
    "..5554BDdDB443..",
    "......4BDB43..33",
    "......4b43......",
    "..554.......443.",
    ".55...........33",
    "................",
    "................",
], BUG)

CREATURE_R1 = draw([
    "................",
    ".....5.....5....",
    "....56....56....",
    "..56666666666...",
    ".566Ek6666Ek664.",
    "5666666666666643",
    ".56666666666643.",
    "..5666656666433.",
    "5...55BDDB55...3",
    "55...BDddDB...33",
    "..56666666643...",
    "..54.......43...",
    ".54.........43..",
    "................",
    "................",
    "................",
], BUG)

CREATURE_R2 = draw([
    "................",
    ".....5.....5....",
    "....56....56....",
    "..56666666666...",
    ".566Ek6666Ek664.",
    "5666666666666643",
    ".56666666666643.",
    "..5666656666433.",
    ".5..55BDDB55..3.",
    "5....BDddDB....3",
    "..56666666643...",
    "...54.....433...",
    "...54......43...",
    "................",
    "................",
    "................",
], BUG)

BAT = {
    "7": (4, 6), "6": (4, 5), "5": (4, 4), "4": (4, 3), "3": (4, 2), "2": (4, 1),
    "W": (7, 7), "w": (7, 5), "R": (8, 5), "r": (8, 3), "k": (0, 3),
}

BAT_UP = draw([
    "................",
    "................",
    "................",
    "................",
    "....5......5....",
    "...565....565...",
    "..5664Ww5664....",
    ".566643kk4664...",
    "566645655466643.",
    "5664.566664.4643",
    "564...5664...463",
    "54.....54.....43",
    "................",
    "................",
    "................",
    "................",
], BAT)

BAT_DOWN = draw([
    "................",
    "................",
    "................",
    "5..............3",
    "56............43",
    "566..5....5..463",
    ".5664Ww..wW4663.",
    "..566643366643..",
    "...5666556663...",
    ".....566663.....",
    "......5663......",
    ".......54.......",
    "................",
    "................",
    "................",
    "................",
], BAT)

BAT_PIF_UP = [[(8, 5) if v == (7, 7) else (8, 3) if v == (7, 5) else v for v in row] for row in BAT_UP]
BAT_PIF_DOWN = [[(8, 5) if v == (7, 7) else (8, 3) if v == (7, 5) else v for v in row] for row in BAT_DOWN]

TILES = {
    1: SCREW,
    2: BOMB,
    3: KEY,
    8: AMMO,
    11: LIFE,
    12: QUESTION,
    4: door(),
    5: rubble(),
    6: crate(2),
    7: crate(14),
    30: CREATURE_L1,
    31: CREATURE_L2,
    34: BAT_UP,
    35: BAT_DOWN,
    36: CREATURE_R1,
    37: CREATURE_R2,
    60: BAT_PIF_UP,
    61: BAT_PIF_DOWN,
    63: void(),
}

# ---------------------------------------------------------------------------
# robbo: colours 12 (blue head and feet), 13 (body), 5 (dark parts),
# 11 (antennae) and 3 (eyes). Built from a head and a body per frame.

ROBBO = {
    "G": (11, 6), "g": (11, 4), "n": (5, 4),
    "W": (7, 6), "H": (12, 5), "h": (12, 4), "i": (12, 3), "j": (12, 2),
    "E": (3, 6), "e": (3, 4), "w": (7, 7), "k": (0, 3),
    "A": (13, 4), "a": (13, 3), "b": (13, 2), "c": (13, 1),
    "D": (5, 3), "d": (5, 2), "f": (5, 1),
    "Y": (10, 5), "R": (8, 4),
    "F": (12, 4), "u": (12, 3), "v": (12, 2),
}

HEAD_FRONT = [
    "..GG........GG..",
    "..Gg........Gg..",
    "...n........n...",
    "...HWWHHHHHHh...",
    "..HWHHHHHHHHHi..",
    "..HhwEehhwEehi..",
    "..HhEEehhEEehi..",
    "..hhhhhhhhhhij..",
    "...iiiiiiiijj...",
]

HEAD_BACK = [
    "..GG........GG..",
    "..Gg........Gg..",
    "...n........n...",
    "...HWWHHHHHHh...",
    "..HWHHHHHHHHHi..",
    "..HhhhhhhhhhHi..",
    "..Hhiiiiiiiihi..",
    "..hhhhhhhhhhij..",
    "...iiiiiiiijj...",
]

HEAD_SIDE = [
    "..GG.......GG...",
    "..Gg.......Gg...",
    "...n.......n....",
    "...HWWHHHHHh....",
    "..HWHHHHHHHHi...",
    "..wEehhhhhhhi...",
    "..EEehhhhhhhi...",
    "..hhhhhhhhhij...",
    "...iiiiiiijj....",
]

BODY_FRONT_STAND = [
    ".aAAAAAAAAAAAAb.",
    "aAAADDDDDDDDAbbc",
    "aA.ADdYddRdDAb.c",
    "aA.ADddddddDAb.c",
    "...aAAAbbAAAb...",
    "...FFu....FFu...",
    "..FFuv...FFuv...",
]

BODY_FRONT_WALK = [
    ".aAAAAAAAAAAAAb.",
    "aAAADDDDDDDDAbbc",
    "aA.ADdYddRdDAb.c",
    "...ADddddddDAb.c",
    "...aAAAbbAAAb...",
    "...FFu.....FFu..",
    "...........FFuv.",
]

BODY_BACK_STAND = [
    ".aAAAAAAAAAAAAb.",
    "aAAAAAAAAAAAAbbc",
    "aA.AbddddddbAb.c",
    "aA.AbddddddbAb.c",
    "...aAAAbbAAAb...",
    "...FFu....FFu...",
    "..FFuv...FFuv...",
]

BODY_BACK_WALK = [
    ".aAAAAAAAAAAAAb.",
    "aAAAAAAAAAAAAbbc",
    "aA.AbddddddbAb.c",
    "aA.Abddddddb....",
    "...aAAAbbAAAb...",
    "..FFu.....FFu...",
    "..FFuv..........",
]

BODY_SIDE_STAND = [
    "...aAAAAAAAb....",
    "...aAADDDAAbb...",
    "...aAADdDAAbb...",
    "...aAADDDAAb....",
    "...aAAAbbAAb....",
    "....FFu..FFu....",
    "...FFuv.FFuv....",
]

BODY_SIDE_WALK1 = [
    "...aAAAAAAAb....",
    "..aaAADDDAAbb...",
    ".aA.AADdDAAbb...",
    "....aADDDAAb....",
    "...aAAAbbAAb....",
    "..FFu.....FFu...",
    ".FFuv......FFuv.",
]

BODY_SIDE_WALK2 = [
    "...aAAAAAAAb....",
    "...aAADDDAAbbc..",
    "...aAADdDAAb.bc.",
    "...aAADDDAAb....",
    "...aAAAbbAAb....",
    ".....FFuFFu.....",
    "....FFuvFuv.....",
]


def robbo(head, body):
    return draw(head + body, ROBBO)


SPRITES = {
    "ROBBO_48": robbo(HEAD_FRONT, BODY_FRONT_STAND),
    "ROBBO_49": robbo(HEAD_FRONT, BODY_FRONT_WALK),
    "ROBBO_50": robbo(HEAD_BACK, BODY_BACK_STAND),
    "ROBBO_51": robbo(HEAD_BACK, BODY_BACK_WALK),
    "ROBBO_52": robbo(HEAD_SIDE, BODY_SIDE_STAND),
    "ROBBO_53": robbo(HEAD_SIDE, BODY_SIDE_WALK1),
    "ROBBO_54": robbo(HEAD_SIDE, BODY_SIDE_WALK2),
    # status bar icons reuse the board drawings
    "ICON_SCREW": SCREW,
    "ICON_LIFE": LIFE,
    "ICON_KEY": KEY,
}
