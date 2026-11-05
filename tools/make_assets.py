#!/usr/bin/env python3
"""Generate the Circus Seal tileset (assets/tileset.gif) and its tile index header (src/gfx.h).

Text pixel art and a few procedural shapes (balloons, beach ball), built with the shared
tools/zealart.py. One 8-bit tileset for both layers and the sprites; palette index 0 is
transparent on layer 1 and on sprites, and is the dark tent color on layer 0.

Layout:
    0         empty
    1..31     arena tiles on layer 1 (rigging, poles, ring floor, pedestals, balloons, HUD icon)
    32..95    font at the ASCII codes (ZGDK's default ASCII map prints text as-is)
    96..127   big top background on layer 0 (stripes, valance, crowd, ring wall)
    128..     sprites; the seal and the clown are native 16x32 sprites (tile N top, N+1 bottom)
Run: python3 tools/make_assets.py   (from the game folder)
"""
import math
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
from zealart import Tileset  # noqa: E402

PAL = [
    ('0', (22, 12, 34)),     # 0 tent darkness / transparent
    ('k', (14, 12, 20)),     # outline
    ('w', (255, 255, 255)),
    ('l', (196, 196, 212)),  # string, light grey
    # seal
    ('S', (78, 92, 114)),
    ('s', (50, 60, 78)),
    ('T', (128, 144, 166)),
    # big top
    ('R', (206, 40, 54)),
    ('r', (140, 22, 40)),
    ('C', (242, 228, 198)),
    ('c', (198, 180, 150)),
    ('Y', (252, 208, 64)),
    ('y', (190, 138, 30)),
    ('q', (44, 30, 66)),     # crowd
    ('Q', (70, 52, 98)),
    ('h', (110, 80, 140)),
    ('F', (218, 172, 104)),  # sawdust
    ('f', (168, 122, 66)),
    ('B', (48, 96, 210)),    # pedestal drum
    ('b', (26, 54, 140)),
    # balloons
    ('U', (70, 150, 250)),
    ('u', (30, 84, 186)),
    ('G', (86, 212, 96)),
    ('g', (30, 140, 56)),
    ('i', (214, 220, 232)),
    ('I', (140, 148, 168)),
    ('P', (255, 126, 186)),
    ('p', (200, 60, 132)),
    ('O', (255, 152, 44)),
    ('o', (204, 92, 22)),
    ('e', (255, 232, 156)),
]
ts = Tileset(PAL)
grid, put = ts.grid, ts.put
CI = ts.ci

# ---------------------------------------------------------------- layer 1: arena
put(0, 'TILE_EMPTY', ts.blank())

# rigging bar across the top, with bulbs
put(1, 'TILE_CEIL', grid([
    "kkkkkkkkkkkkkkkk",
    "yYYYyyYYYYyyYYYy",
    "YyyyYYyyyyYYyyyY",
    "yYYYyyYYYYyyYYYy",
    "kkkkkkkkkkkkkkkk",
    "qqqqqqqqqqqqqqqq",
    "qqkqqqqqqqqkqqqq",
    "qkYkqqqqqqkYkqqq",
    "kYeYkqqqqkYeYkqq",
    "kYwYkqqqqkYwYkqq",
    "qkYkqqqqqqkYkqqq",
    "qqkqqqqqqqqkqqqq",
    "qqqqqqqqqqqqqqqq",
    "kkkkkkkkkkkkkkkk",
    "rRRRRRRRRRRRRRRr",
    "kkkkkkkkkkkkkkkk",
]), ['TF_SOLID'])


def pole(right):
    rows = []
    for y in range(16):
        r = ['0'] * 16
        for x in range(8):
            stripe = ((x + y) // 3) % 2
            r[x] = 'R' if stripe else 'C'
        r[0] = 'k'
        r[7] = 'k'
        r[1] = 'r' if r[1] == 'R' else 'c'
        rows.append(''.join(r))
    px = grid(rows)
    return [list(reversed(row)) for row in px] if right else px


# the ball bounces on the inner edge of the edge columns, so each pole sits on its inner half
put(2, 'TILE_POLE_L', pole(True), ['TF_SOLID'])
put(3, 'TILE_POLE_R', pole(False), ['TF_SOLID'])

put(4, 'TILE_FLOOR', grid([
    "kkkkkkkkkkkkkkkk",
    "RRRRRRRRRRRRRRRR",
    "YYYYYYYYYYYYYYYY",
    "rrrrrrrrrrrrrrrr",
    "kkkkkkkkkkkkkkkk",
    "FFFfFFFFFFFFfFFF",
    "FFFFFFfFFFFFFFFF",
    "FfFFFFFFFFFfFFFF",
    "FFFFFFFFfFFFFFfF",
    "FFFFfFFFFFFFFFFF",
    "FFFFFFFFFFfFFFFF",
    "FFfFFFFFFFFFFfFF",
    "FFFFFFFfFFFFFFFF",
    "fFFFFFFFFFFfFFFF",
    "FFFFfFFFFFFFFFFf",
    "FFFFFFFFFFFFFFFF",
]), ['TF_SOLID'])

# pedestal (circus drum): standable top, the drum body is scenery the seal walks in front of
put(5, 'TILE_PED_TOP', grid([
    "kkkkkkkkkkkkkkkk",
    "kYYYYYYYYYYYYYYk",
    "kyyyyyyyyyyyyyyk",
    "kkkkkkkkkkkkkkkk",
    "kBBBBBBBBBBBBBBk",
    "kBBBBBBYYBBBBBBk",
    "kBBBBBBYYBBBBBbk",
    "kBBBBYYYYYYBBBbk",
    "kBBBBBYYYYBBBBbk",
    "kBBBBBYyyYBBBBbk",
    "kBBBBYyBByYBBBbk",
    "kBBBBBBBBBBBBBbk",
    "kBBBBBBBBBBBBBbk",
    "kkkkkkkkkkkkkkkk",
    "kYYYYYYYYYYYYYYk",
    "kyyyyyyyyyyyyyyk",
]))  # Base flags are for the ball; arena.c adds TF_ONEWAY for bodies on pedestal tops.
put(6, 'TILE_PED', grid([
    "kBBBBBBBBBBBBBbk",
    "kBRBBBBBBBBBRBbk",
    "kBBRBBBBBBBRBBbk",
    "kBBBRBBBBBRBBBbk",
    "kBBBBRBBBRBBBBbk",
    "kBBBBBRBRBBBBBbk",
    "kBBBBBBRBBBBBBbk",
    "kBBBBBRBRBBBBBbk",
    "kBBBBRBBBRBBBBbk",
    "kBBBRBBBBBRBBBbk",
    "kBBRBBBBBBBRBBbk",
    "kBRBBBBBBBBBRBbk",
    "kkkkkkkkkkkkkkkk",
    "kYYYYYYYYYYYYYYk",
    "kyyyyyyyyyyyyyyk",
    "kkkkkkkkkkkkkkkk",
]))


def balloon(light, dark, mark=None):
    """procedural balloon: ellipse with a highlight, knot and string; transparent around it"""
    px = ts.blank()
    cx, cy, rx, ry = 7.5, 6.0, 6.2, 6.4
    for y in range(13):
        for x in range(16):
            d = ((x - cx) / rx) ** 2 + ((y - cy) / ry) ** 2
            if d <= 1.0:
                shade = (x - cx) * 0.6 + (y - cy) > 3.2
                px[y][x] = CI[dark if shade else light]
    for (x, y) in ((4, 3), (5, 2), (4, 4)):
        px[y][x] = CI['w']
    px[13][7] = px[13][8] = CI[dark]
    for y, x in ((14, 8), (15, 7)):
        px[y][x] = CI['l']
    if mark:
        for y, row in enumerate(mark):
            for x, ch in enumerate(row):
                if ch != '.':
                    px[3 + y][4 + x] = CI[ch]
    return ts.outline(px, 'k')


BALLOON = ['TF_SOLID', 'TF_USER1']
put(7, 'TILE_BAL_RED', balloon('R', 'r'), BALLOON)
put(8, 'TILE_BAL_BLUE', balloon('U', 'u'), BALLOON)
put(9, 'TILE_BAL_GREEN', balloon('G', 'g'), BALLOON)
put(10, 'TILE_BAL_YELLOW', balloon('Y', 'y'), BALLOON)
put(11, 'TILE_BAL_SILVER', balloon('i', 'I', [
    "...Y....",
    "..YYY...",
    "YYYYYYY.",
    ".YYYYY..",
    ".YY.YY..",
]), BALLOON)
put(12, 'TILE_BAL_FISH', balloon('O', 'o', [
    "........",
    ".kkk..k.",
    "kekeekk.",
    ".kkk..k.",
]), BALLOON)
put(13, 'TILE_BAL_HEART', balloon('P', 'p', [
    "........",
    ".ww.ww..",
    ".wwwww..",
    "..www...",
    "...w....",
]), BALLOON)

# ---------------------------------------------------------------- font 32..95
ts.font('w', 'k')

# ---------------------------------------------------------------- layer 0: the big top
put(96, 'BG_STRIPE', grid([
    "rRRRRRRrcCCCCCCc",
] * 16))
put(97, 'BG_VALANCE', grid([
    "YYYYYYYYYYYYYYYY",
    "yyyyyyyyyyyyyyyy",
    "RRRRRRRRRRRRRRRR",
    "RRRRRRRRRRRRRRRR",
    "RRRRRRRRRRRRRRRR",
    "rRRRRRRRRRRRRRRr",
    "rRRRRRRRRRRRRRRr",
    "krRRRRRRRRRRRRrk",
    "kYrRRRRRRRRRRrYk",
    "cCYrrRRRRRRrrYCc",
    "cCCYYrrrrrrYYCCc",
    "cCCCCYYYYYYCCCCc",
    "cCCCCCCyyCCCCCCc",
    "cCCCCCCCCCCCCCCc",
    "rRRRRRRrcCCCCCCc",
    "rRRRRRRrcCCCCCCc",
]))
put(98, 'BG_CROWD1', grid([
    "rRRRRRRrcCCCCCCc",
    "kkkkkkkkkkkkkkkk",
    "qqqqqqqqqqqqqqqq",
    "qqqQQqqqqqqqQQqq",
    "qqQhhQqqqqqQhhQq",
    "qqQhhQqqqqqQhhQq",
    "qqqQQqqqQQqqQQqq",
    "qqQQQQqQhhQQQQQq",
    "qQQQQQQQhhQQQQQQ",
    "QQQQQQQQQQQQQQQQ",
    "QQQQQQQQQQQQQQQQ",
    "qqqqqqqqqqqqqqqq",
    "qqqQQqqqqqqqQQqq",
    "qqQhhQqqqqqQhhQq",
    "qqQhhQqqQQqQhhQq",
    "qqqQQqqQhhQqQQqq",
]))
put(99, 'BG_CROWD2', grid([
    "qqQQQQqQhhQQQQQq",
    "qQQQQQQQQQQQQQQQ",
    "QQQQQQQQQQQQQQQQ",
    "qqqqqqqqqqqqqqqq",
    "qqqqQQqqqqqqQQqq",
    "qqqQhhQqqqqQhhQq",
    "qqqQhhQqqQqQhhQq",
    "qqqqQQqqQhQqQQqq",
    "qqqQQQQqQQQQQQQq",
    "qqQQQQQQQQQQQQQQ",
    "QQQQQQQQQQQQQQQQ",
    "kkkkkkkkkkkkkkkk",
    "YYYYYYYYYYYYYYYY",
    "yyyyyyyyyyyyyyyy",
    "kkkkkkkkkkkkkkkk",
    "RRRRRRRRRRRRRRRR",
]))
put(100, 'BG_WALL', grid([
    "RRRRRRRRRRRRRRRR",
    "RRRRRRRYRRRRRRRR",
    "RRRRRRYYYRRRRRRR",
    "RRRYYYYYYYYYRRRR",
    "RRRRYYYYYYYRRRRR",
    "RRRRRYYYYYRRRRRR",
    "RRRRYYYRYYYRRRRR",
    "RRRRYYRRRYYRRRRR",
    "RRRRRRRRRRRRRRRR",
    "rrrrrrrrrrrrrrrr",
    "RRRRRRRRRRRRRRRR",
    "RRRRRRRRRRRRRRRR",
    "RRRRRRRRRRRRRRRR",
    "rrrrrrrrrrrrrrrr",
    "RRRRRRRRRRRRRRRR",
    "rrrrrrrrrrrrrrrr",
]))
put(101, 'BG_WALL2', grid([
    "RRRRRRRRRRRRRRRR",
    "RRRRRRRRRRRRRRRR",
    "RRRRRRRRRRRRRRRR",
    "RRRRRRRRRRRRRRRR",
    "RRRRRRRRRRRRRRRR",
    "RRRRRRRRRRRRRRRR",
    "RRRRRRRRRRRRRRRR",
    "RRRRRRRRRRRRRRRR",
    "RRRRRRRRRRRRRRRR",
    "rrrrrrrrrrrrrrrr",
    "RRRRRRRRRRRRRRRR",
    "RRRRRRRRRRRRRRRR",
    "RRRRRRRRRRRRRRRR",
    "rrrrrrrrrrrrrrrr",
    "RRRRRRRRRRRRRRRR",
    "rrrrrrrrrrrrrrrr",
]))

# ---------------------------------------------------------------- sprites
ts.start_sprites(128)
spr, tall = ts.spr, ts.tall


def g32(rows):
    rows = list(rows)
    assert len(rows) == 32, len(rows)
    return grid(rows, 32)


# the seal sits up, facing right, nose to the sky; the ball rests on the nose tip (rows 0-1)
SEAL = [
    "........kk......",
    ".......kkkk.....",
    ".......kSSk.....",
    "......kSSSSk....",
    "......kSSSSk....",
    ".....kSSSSSTk...",
    ".....kSwkSSTk...",
    ".....kSkkSSTk...",
    "....kSSSSSSTTk..",
    "....kSSSSSSTTk..",
    "....kYRYRYRYRk..",
    "...ksSSSSTTTTk..",
    "...ksSSSSTTTTk..",
    "...ksSSSTTTTTTk.",
    "..ksSSSSTTTTTTk.",
    "..ksSSSSTTTTTTk.",
    "..ksSSSSTTTTTTk.",
    "..ksSSSSSTTTTTk.",
    "..ksSSSSSTTTTkSk",
    "..ksSSSSSSTTkSSk",
    "..ksSSSSSSSkSSk.",
    "..ksSSSSSSSkkk..",
    "..kssSSSSSSSk...",
    "..kssSSSSSSSk...",
    "...kssSSSSSSk...",
    "...kssSSSSSSSk..",
    "....kssSSSSSSk..",
    ".....kssSSSSSSk.",
    "......kkssSSSSk.",
    ".....kSSkkkSSSSk",
    "....kSSSk..kSSSk",
    "....kkkk....kkkk",
]


def edit(base, changes):
    out = list(base)
    for y, row in changes.items():
        out[y] = row
    return out


def shift_down(rows, n):
    return [""] * n + rows[:32 - n]


SEAL_WALK1 = edit(SEAL, {
    29: "....kSSSkkkSSSSk",
    30: "...kSSSk...kSSk.",
    31: "...kkkk....kkk..",
})
SEAL_WALK2 = edit(shift_down(SEAL, 1), {
    29: "......kkssSSSSk.",
    30: ".....kSSSkkSSSSk",
    31: ".....kkkkk.kkkkk",
})
SEAL_HOP = edit(SEAL, {
    17: "..ksSSSSSTTTTTkk",
    18: "..ksSSSSSTTTTkSk",
    19: "..ksSSSSSSTTkSSk",
    20: "..ksSSSSSSSTkSk.",
    21: "..ksSSSSSSSTkk..",
    29: "......kSkkkSSk..",
    30: ".....kSSk.kSSk..",
    31: ".....kkk..kkk...",
})
SEAL_BONK = edit(SEAL, {
    5: ".....kSSSSSTk...",
    6: ".....kkSkSSTk...",
    7: ".....kSkSSSTk...",
    8: "....kkSkSSSTTk..",
})
SEAL_SAD = edit(SEAL, {
    6: ".....kSSSSSTk...",
    7: ".....kkkkSSTk...",
})
SEAL_CHEER = edit(SEAL, {
    10: "....kYRYRYRYRkkk",
    11: "...ksSSSSTTTTkSk",
    12: "...ksSSSSTTTkSSk",
    13: "...ksSSSTTTTkSk.",
    14: "..ksSSSSTTTTTkk.",
    15: "..ksSSSSTTTTTTk.",
    18: "..ksSSSSSTTTTTk.",
    19: "..ksSSSSSSTTTTk.",
    20: "..ksSSSSSSSTTTk.",
    21: "..ksSSSSSSSTTk..",
})
tall('SPR_SEAL', g32(SEAL))
tall('SPR_SEAL_WALK1', g32(SEAL_WALK1))
tall('SPR_SEAL_WALK2', g32(SEAL_WALK2))
tall('SPR_SEAL_HOP', g32(SEAL_HOP))
tall('SPR_SEAL_BONK', g32(SEAL_BONK))
tall('SPR_SEAL_SAD', g32(SEAL_SAD))
tall('SPR_SEAL_CHEER', g32(SEAL_CHEER))


def beach_ball(frame, size=6.3):
    """procedural beach ball, 6 panels, rotated by `frame` (4 frames per panel step)"""
    px = ts.blank()
    colors = ['R', 'w', 'U', 'w', 'Y', 'w']
    cx = cy = 7.5
    for y in range(16):
        for x in range(16):
            dx, dy = x - cx, y - cy
            r = math.hypot(dx, dy)
            if r > size:
                continue
            a = (math.atan2(dy, dx) / (2 * math.pi) + frame / 24.0) % 1.0
            c = colors[int(a * 6) % 6]
            if r < 1.6:
                c = 'w'
            elif dx * 0.7 + dy > size * 0.75 and c == 'w':
                c = 'l'
            px[y][x] = CI[c]
    px[4][5] = px[5][4] = CI['w']
    return ts.outline(px, 'k')


for f in range(4):
    spr('SPR_BALL%d' % f, beach_ball(f))

CLOWN = [
    ".......kk.......",
    "......kwwk......",
    ".......kk.......",
    "......kUUk......",
    ".....kUYUUk.....",
    ".....kUUUYk.....",
    "....kUUYUUUk....",
    "..kOkkkkkkkkOk..",
    ".kOOkwwwwwwkOOk.",
    ".kOkwkwwwwkwkOk.",
    "..kkwwwRRwwwkk..",
    "...kwwRRRRwwk...",
    "...kwRwwwwRwk...",
    "....kwRRRRwk....",
    "..kYkkkkkkkkYk..",
    ".kYYYkYYYYkYYYk.",
    "..kkGGGGGGGGkk..",
    "..kGGwGGGGwGGk..",
    ".kwkGGGGGGGGkwk.",
    ".kwkGGGwGGGGkwk.",
    "..kkGwGGGGwGkk..",
    "...kGGGGGGGGk...",
    "...kGGGwGGGGk...",
    "...kGGGGGGwGk...",
    "...kGGGkkGGGk...",
    "...kGGk..kGGk...",
    "...kUUk..kUUk...",
    "...kUUk..kUUk...",
    "...kUUk..kUUk...",
    "..krrrk..krrrrk.",
    ".krrrrk..krrrrrk",
    ".kkkkk....kkkkk.",
]
CLOWN2 = edit(CLOWN, {
    25: "...kGGk.kGGk....",
    26: "...kUUk.kUUk....",
    27: "....kUUkkUUk....",
    28: "....kUUkUUk.....",
    29: "...krrrkrrrrk...",
    30: "..krrrrkrrrrrk..",
    31: "..kkkkkkkkkkk...",
})
CLOWN_HONK = edit(CLOWN, {
    10: "..kkwwwYYwwwkk..",
    11: "...kwwYYYYwwk...",
    12: "...kwwkkkkwwk...",
    13: "....kwkRRkwk....",
})
tall('SPR_CLOWN1', g32(CLOWN))
tall('SPR_CLOWN2', g32(CLOWN2))
tall('SPR_CLOWN_HONK', g32(CLOWN_HONK))

FISH1 = [
    "", "", "", "", "",
    "......kkkk......",
    "....kkOOOOkk..kk",
    "...kOwkOOOOOkkOk",
    "...kOOOOOOOOOOOk",
    "....kkeeeeekk.kk",
    "......kkkkk.....",
]
FISH2 = [
    "", "", "", "", "",
    "......kkkk....k.",
    "....kkOOOOkk.kOk",
    "...kOwkOOOOOkkOk",
    "...kOOOOOOOOOOk.",
    "....kkeeeeekk...",
    "......kkkkk.....",
]
spr('SPR_FISH1', grid(FISH1))
spr('SPR_FISH2', grid(FISH2))

spr('SPR_POP1', grid([
    "",
    "",
    "",
    ".......Y........",
    "...Y...w...Y....",
    "....w.....w.....",
    "................",
    "..Yw...k...wY...",
    "................",
    "....w.....w.....",
    "...Y...w...Y....",
    ".......Y........",
]))
spr('SPR_POP2', grid([
    "",
    ".......Y........",
    "..Y.........Y...",
    "................",
    "................",
    "................",
    "Y..............Y",
    "w..............w",
    "................",
    "................",
    "................",
    "..Y.........Y...",
    ".......Y........",
]))
spr('SPR_STAR', ts.outline(grid([
    "",
    "",
    "",
    ".......Y........",
    "......YYY.......",
    "..YYYYYwYYYY....",
    "...YYYYYYYY.....",
    "....YYYYYY......",
    "....YYYyYY......",
    "...YYy..yYY.....",
    "...Y......Y.....",
]), 'k'))

# small ball icon for the HUD (layer 1)
put(14, 'TILE_ICON_BALL', beach_ball(0, 5.0))

used = ts.save(ROOT)
print('tiles used: %d' % used)

# Shared SNES art, owned locally so the game remains standalone.
from snes_glyphs import install as install_snes_glyphs
install_snes_glyphs(ROOT, used)
