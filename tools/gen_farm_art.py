#!/usr/bin/env python3
"""Generates the sprite art for SPROUT (farm game, 320x170 landscape).

Output: src/games/farm/art.h + art.cpp
Run:    python3 tools/gen_farm_art.py

Sprites are 16x16 RGB565 pixel art. The Super Retro World pack by Gif
(store.godotengine.org/asset/gif/super-retro-world/, custom commercial
license, free) supplies the style; since the pack ships as Godot
spritesheets, every sprite here is drawn procedurally in that look and
can be replaced by a real PNG from the pack:

    assets/farm/<SpriteName>.png   (16x16, RGB/RGBA/palette/gray 8-bit)

Sprite layout (names match the SpriteId enum in farm.h):
    Soil          TilledSoil    Sprout        Crop1         Crop2
    Crop3         Withered      Cursor        Farmer        WaterCan
    Coin          Plot          House

Crops cycle Sprout -> Crop1 -> Crop2 -> Crop3 (ready) -> harvested.
A crop that dries out two days in a row becomes Withered.
"""
import os
import struct
import zlib

OUT_H = "src/games/farm/art.h"
OUT_C = "src/games/farm/art.cpp"
ASSET_DIR = "assets/farm"
TS = 16  # tile/sprite size

# ---------------------------------------------------------------- Canvas

class Canvas:
    def __init__(self, w=TS, h=TS):
        self.w, self.h = w, h
        self.px = [[(0, 0, 0)] * w for _ in range(h)]

    def set(self, x, y, c):
        if 0 <= x < self.w and 0 <= y < self.h:
            self.px[y][x] = c

    def rect(self, x0, y0, w, h, c):
        for y in range(max(0, y0), min(self.h, y0 + h)):
            for x in range(max(0, x0), min(self.w, x0 + w)):
                self.px[y][x] = c

    def outline(self, x0, y0, w, h, c):
        for x in range(x0, x0 + w):
            self.set(x, y0, c)
            self.set(x, y0 + h - 1, c)
        for y in range(y0, y0 + h):
            self.set(x0, y, c)
            self.set(x0 + w - 1, y, c)

    def disc(self, cx, cy, rad, c):
        for y in range(self.h):
            for x in range(self.w):
                if (x - cx) ** 2 + (y - cy) ** 2 <= rad * rad:
                    self.set(x, y, c)

    def tri(self, x0, y0, x1, y1, x2, y2, c):
        minx = max(0, min(x0, x1, x2))
        maxx = min(self.w - 1, max(x0, x1, x2))
        miny = max(0, min(y0, y1, y2))
        maxy = min(self.h - 1, max(y0, y1, y2))
        d = (y1 - y2) * (x0 - x2) + (x2 - x1) * (y0 - y2)
        if d == 0:
            return
        for y in range(miny, maxy + 1):
            for x in range(minx, maxx + 1):
                a = ((y1 - y2) * (x - x2) + (x2 - x1) * (y - y2)) / d
                b = ((y2 - y0) * (x - x2) + (x0 - x2) * (y - y2)) / d
                cc = 1 - a - b
                if a >= 0 and b >= 0 and cc >= 0:
                    self.set(x, y, c)

    def shift(self, dx, dy, c_from=None):
        """Shift content, wrapping transparent (c_from) pixels."""
        new = [[(0, 0, 0)] * self.w for _ in range(self.h)]
        for y in range(self.h):
            for x in range(self.w):
                sx, sy = x - dx, y - dy
                if 0 <= sx < self.w and 0 <= sy < self.h:
                    p = self.px[sy][sx]
                    if c_from is None or p != c_from:
                        new[y][x] = p
        self.px = new

# ---------------------------------------------------------------- Palette
# Super-Retro-World-ish earthy palette (RGB tuples).
C_SKY      = (126, 200, 242)
C_GRASS    = (92, 186, 92)
C_GRASS_DK = (58, 138, 66)
C_SOIL     = (134, 86, 54)
C_SOIL_DK  = (96, 58, 34)
C_SOIL_LT  = (164, 116, 74)
C_TILLED   = (110, 70, 42)
C_TILLED_LT= (140, 96, 60)
C_WATER    = (64, 140, 220)
C_WATER_DK = (36, 96, 160)
C_LEAF     = (70, 160, 70)
C_LEAF_DK  = (44, 110, 48)
C_LEAF_LT  = (130, 200, 90)
C_STEM     = (84, 128, 52)
C_CROP1    = (238, 190, 70)    # carrot tops / grain
C_CROP2    = (220, 120, 50)    # tomato-ish
C_CROP3    = (200, 60, 60)     # ripe berries
C_CROP3_LT = (240, 130, 110)
C_WITHER   = (120, 100, 60)
C_CURSOR   = (255, 255, 255)
C_CURSOR2  = (255, 220, 90)
C_SKIN     = (240, 200, 160)
C_SKIN_DK  = (210, 160, 120)
C_HAT      = (240, 220, 120)
C_SHIRT    = (70, 110, 200)
C_PANTS    = (70, 80, 140)
C_SHOE     = (90, 60, 40)
C_WOOD     = (150, 100, 60)
C_WOOD_DK  = (110, 70, 40)
C_ROOF     = (190, 80, 60)
C_ROOF_DK  = (150, 56, 40)
C_GOLD     = (250, 210, 60)
C_GOLD_DK  = (200, 160, 30)
C_WHITE    = (250, 250, 250)
C_BLACK    = (20, 16, 12)

# ---------------------------------------------------------------- Sprites

def sprite_soil():
    cv = Canvas()
    cv.rect(0, 0, TS, TS, C_SOIL)
    # clumpy texture
    rng = [(2, 3, C_SOIL_DK), (10, 2, C_SOIL_DK), (5, 8, C_SOIL_DK),
           (12, 9, C_SOIL_DK), (3, 12, C_SOIL_DK), (9, 13, C_SOIL_DK),
           (6, 5, C_SOIL_LT), (13, 5, C_SOIL_LT), (1, 9, C_SOIL_LT),
           (8, 11, C_SOIL_LT), (14, 12, C_SOIL_LT)]
    for x, y, c in rng:
        cv.rect(x, y, 2, 2, c)
    cv.outline(0, 0, TS, TS, C_SOIL_DK)
    return cv

def sprite_tilled():
    cv = Canvas()
    cv.rect(0, 0, TS, TS, C_TILLED)
    # furrow lines
    for y in (3, 7, 11):
        cv.rect(1, y, TS - 2, 1, C_TILLED_LT)
        cv.rect(1, y + 1, TS - 2, 1, C_SOIL_DK)
    # little clods
    for x, y in ((3, 1), (11, 5), (6, 9), (13, 13)):
        cv.set(x, y, C_TILLED_LT)
        cv.set(x + 1, y, C_TILLED_LT)
    cv.outline(0, 0, TS, TS, C_SOIL_DK)
    return cv

def sprite_sprout():
    cv = sprite_tilled()
    cx = TS // 2
    cv.rect(cx - 1, 8, 2, 5, C_STEM)
    cv.tri(cx - 1, 9, cx - 6, 5, cx - 1, 4, C_LEAF)
    cv.tri(cx + 1, 9, cx + 6, 5, cx + 1, 4, C_LEAF_DK)
    cv.set(cx - 4, 5, C_LEAF_LT)
    cv.set(cx + 4, 5, C_LEAF_LT)
    return cv

def sprite_crop1():
    cv = sprite_tilled()
    cx = TS // 2
    cv.rect(cx - 1, 5, 2, 8, C_STEM)
    cv.tri(cx - 1, 7, cx - 7, 2, cx - 1, 1, C_LEAF)
    cv.tri(cx + 1, 7, cx + 7, 2, cx + 1, 1, C_LEAF)
    cv.tri(cx - 1, 10, cx - 6, 5, cx - 1, 4, C_LEAF_DK)
    cv.tri(cx + 1, 10, cx + 6, 5, cx + 1, 4, C_LEAF_DK)
    cv.set(cx - 5, 3, C_LEAF_LT)
    cv.set(cx + 5, 3, C_LEAF_LT)
    return cv

def sprite_crop2():
    cv = sprite_tilled()
    cx = TS // 2
    cv.rect(cx - 1, 4, 2, 9, C_STEM)
    # bushy top
    cv.disc(cx, 4, 4, C_LEAF)
    cv.disc(cx - 4, 6, 3, C_LEAF_DK)
    cv.disc(cx + 4, 6, 3, C_LEAF_DK)
    cv.set(cx - 2, 2, C_LEAF_LT)
    # first fruits appear
    cv.disc(cx - 2, 5, 2, C_CROP2)
    cv.disc(cx + 3, 4, 2, C_CROP2)
    return cv

def sprite_crop3():
    cv = sprite_tilled()
    cx = TS // 2
    cv.rect(cx - 1, 3, 2, 10, C_STEM)
    cv.disc(cx, 3, 4, C_LEAF)
    cv.disc(cx - 5, 5, 3, C_LEAF_DK)
    cv.disc(cx + 5, 5, 3, C_LEAF_DK)
    cv.set(cx - 2, 1, C_LEAF_LT)
    # ripe crop: cluster of berries/fruit
    cv.disc(cx - 3, 4, 2, C_CROP3)
    cv.disc(cx + 2, 3, 2, C_CROP3)
    cv.disc(cx, 7, 2, C_CROP3)
    cv.disc(cx + 3, 7, 2, C_CROP3)
    cv.set(cx - 4, 3, C_CROP3_LT)
    cv.set(cx + 1, 2, C_CROP3_LT)
    return cv

def sprite_withered():
    cv = sprite_tilled()
    cx = TS // 2
    cv.rect(cx - 1, 7, 2, 6, C_WITHER)
    cv.tri(cx - 1, 9, cx - 6, 5, cx - 1, 4, C_WITHER)
    cv.tri(cx + 1, 9, cx + 5, 6, cx + 1, 4, C_WITHER)
    cv.set(cx - 4, 5, (150, 130, 80))
    cv.set(cx + 3, 5, (150, 130, 80))
    # dry soil cracks
    cv.set(3, 12, C_SOIL_DK)
    cv.set(4, 12, C_SOIL_DK)
    cv.set(11, 10, C_SOIL_DK)
    cv.set(12, 10, C_SOIL_DK)
    return cv

def sprite_cursor():
    cv = Canvas()
    cv.outline(0, 0, TS, TS, C_CURSOR)
    cv.outline(1, 1, TS - 2, TS - 2, C_CURSOR2)
    # corner brackets
    for i in range(4):
        cv.set(i + 2, 0, C_CURSOR)
        cv.set(0, i + 2, C_CURSOR)
        cv.set(TS - 1 - i - 2, TS - 1, C_CURSOR)
        cv.set(TS - 1, TS - 1 - i - 2, C_CURSOR)
    return cv

def sprite_farmer():
    """16x16 farmer, 4-direction style; facing down (front)."""
    cv = Canvas()
    # straw hat
    cv.rect(3, 1, 10, 3, C_HAT)
    cv.rect(1, 3, 14, 2, C_HAT)
    cv.rect(1, 4, 14, 1, (210, 190, 100))
    # face
    cv.rect(4, 5, 8, 4, C_SKIN)
    cv.set(5, 6, C_BLACK)
    cv.set(10, 6, C_BLACK)
    cv.set(6, 8, C_SKIN_DK)
    cv.set(9, 8, C_SKIN_DK)
    # shirt
    cv.rect(3, 9, 10, 4, C_SHIRT)
    cv.rect(1, 10, 2, 3, C_SHIRT)   # arms
    cv.rect(13, 10, 2, 3, C_SHIRT)
    cv.set(1, 13, C_SKIN)
    cv.set(14, 13, C_SKIN)
    # pants + shoes
    cv.rect(4, 13, 3, 3, C_PANTS)
    cv.rect(9, 13, 3, 3, C_PANTS)
    cv.rect(3, 15, 4, 1, C_SHOE)
    cv.rect(9, 15, 4, 1, C_SHOE)
    return cv

def sprite_watercan():
    cv = Canvas()
    # body
    cv.rect(4, 6, 8, 8, C_WATER)
    cv.rect(5, 5, 6, 2, C_WATER_DK)
    # spout
    cv.tri(12, 7, 15, 4, 12, 9, C_WATER_DK)
    # handle
    cv.rect(3, 7, 1, 4, C_WATER_DK)
    cv.rect(2, 8, 1, 2, C_WATER_DK)
    cv.rect(2, 6, 1, 1, C_WATER_DK)
    # highlight
    cv.set(5, 7, (150, 200, 240))
    cv.set(5, 8, (150, 200, 240))
    # water drops
    cv.set(14, 2, C_WATER)
    cv.set(15, 3, C_WATER)
    return cv

def sprite_coin():
    cv = Canvas()
    cv.disc(8, 8, 6, C_GOLD)
    cv.disc(8, 8, 4, C_GOLD_DK)
    cv.disc(8, 8, 3, C_GOLD)
    cv.set(6, 5, (255, 240, 140))
    cv.set(5, 6, (255, 240, 140))
    # "S" mint mark
    cv.set(8, 6, C_GOLD_DK)
    cv.set(7, 7, C_GOLD_DK)
    cv.set(9, 7, C_GOLD_DK)
    cv.set(7, 9, C_GOLD_DK)
    cv.set(9, 9, C_GOLD_DK)
    return cv

def sprite_plot():
    """Decorative empty field border (grass frame around the farm)."""
    cv = Canvas()
    cv.rect(0, 0, TS, TS, C_GRASS)
    cv.rect(2, 2, TS - 4, TS - 4, C_GRASS_DK)
    for x, y in ((1, 1), (14, 1), (1, 14), (14, 14), (7, 0), (8, 15)):
        cv.set(x, y, C_GRASS_DK)
    return cv

def sprite_house():
    """Farmhouse, 32x32 (drawn at 2x)."""
    cv = Canvas(32, 32)
    # walls
    cv.rect(4, 14, 24, 18, C_WOOD)
    for y in range(14, 32, 4):
        cv.rect(4, y, 24, 1, C_WOOD_DK)
    # roof
    cv.tri(2, 14, 16, 2, 30, 14, C_ROOF)
    cv.tri(2, 14, 16, 2, 30, 14, C_ROOF)  # solid
    cv.rect(2, 12, 28, 3, C_ROOF_DK)
    # door + window
    cv.rect(13, 22, 6, 10, C_WOOD_DK)
    cv.set(17, 27, C_GOLD)
    cv.rect(7, 18, 4, 4, C_WHITE)
    cv.rect(21, 18, 4, 4, C_WHITE)
    cv.set(9, 20, C_WHITE)
    cv.set(23, 20, C_WHITE)
    # chimney + smoke
    cv.rect(24, 4, 4, 8, C_WOOD_DK)
    cv.disc(26, 2, 2, (200, 200, 200))
    cv.disc(28, 0, 1, (180, 180, 180))
    return cv

# ---------------------------------------------------------------- RGB565

def to_rgb565(px):
    r, g, b = px
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)

def rows_of(cv):
    return [[to_rgb565(cv.px[y][x]) for x in range(cv.w)] for y in range(cv.h)]

# ---------------------------------------------------------------- PNG import

def load_png(path):
    """Minimal PNG loader (RGB, RGBA, grayscale, palette; 8 bit, not
    interlaced). Returns (w, h, [[(r,g,b), ...], ...])."""
    data = open(path, "rb").read()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise SystemExit(f"{path}: not a PNG file")
    pos = 8
    idat = b""
    w = h = bd = ct = 0
    palette = []
    while pos < len(data):
        ln, typ = struct.unpack(">I4s", data[pos:pos + 8])
        pos += 8
        chunk = data[pos:pos + ln]
        pos += ln + 4  # CRC
        if typ == b"IHDR":
            w, h, bd, ct, _comp, _filt, inter = struct.unpack(">IIBBBBB", chunk)
            if inter:
                raise SystemExit(f"{path}: interlaced PNG not supported")
            if bd != 8:
                raise SystemExit(f"{path}: bit depth 8 expected (got {bd})")
        elif typ == b"PLTE":
            palette = [tuple(chunk[i:i + 3]) for i in range(0, len(chunk), 3)]
        elif typ == b"IDAT":
            idat += chunk
        elif typ == b"IEND":
            break
    raw = zlib.decompress(idat)
    bpp = {0: 1, 2: 3, 3: 1, 4: 2, 6: 4}[ct]
    stride = w * bpp
    out = bytearray()
    prev = bytearray(stride)
    p = 0
    for _y in range(h):
        f = raw[p]; p += 1
        line = bytearray(raw[p:p + stride]); p += stride
        if f == 1:
            for i in range(bpp, stride):
                line[i] = (line[i] + line[i - bpp]) & 0xFF
        elif f == 2:
            for i in range(stride):
                line[i] = (line[i] + prev[i]) & 0xFF
        elif f == 3:
            for i in range(stride):
                a = line[i - bpp] if i >= bpp else 0
                line[i] = (line[i] + ((a + prev[i]) >> 1)) & 0xFF
        elif f == 4:
            for i in range(stride):
                a = line[i - bpp] if i >= bpp else 0
                b = prev[i]
                c = prev[i - bpp] if i >= bpp else 0
                pp = a + b - c
                pa, pb, pc = abs(pp - a), abs(pp - b), abs(pp - c)
                pr = a if (pa <= pb and pa <= pc) else (b if pb <= pc else c)
                line[i] = (line[i] + pr) & 0xFF
        out += line
        prev = line
    px = []
    idx = 0
    for _y in range(h):
        row = []
        for _x in range(w):
            if ct == 2:
                row.append((int(out[idx]), int(out[idx + 1]), int(out[idx + 2])))
                idx += 3
            elif ct == 6:
                row.append((int(out[idx]), int(out[idx + 1]), int(out[idx + 2])))
                idx += 4  # alpha ignored: sprites are opaque
            elif ct == 0:
                v = int(out[idx]); idx += 1
                row.append((v, v, v))
            elif ct == 3:
                r, g, b = palette[out[idx]]
                row.append((int(r), int(g), int(b)))
                idx += 1
            elif ct == 4:
                v = int(out[idx]); idx += 2
                row.append((v, v, v))
        px.append(row)
    return w, h, px

def sprite_from_png(name, size):
    """Load assets/farm/<name>.png, scale nearest to size x size."""
    w, h, src = load_png(os.path.join(ASSET_DIR, f"{name}.png"))
    cv = Canvas(size, size)
    for y in range(size):
        sy = y * h // size
        for x in range(size):
            sx = x * w // size
            cv.px[y][x] = src[sy][sx]
    return cv

def has_png(name):
    return os.path.exists(os.path.join(ASSET_DIR, f"{name}.png"))

# ---------------------------------------------------------------- main

SPRITES = [
    # (C symbol suffix, PNG file name, size, procedural fallback)
    ("Soil",       "Soil",       TS,  sprite_soil),
    ("TilledSoil", "TilledSoil", TS,  sprite_tilled),
    ("Sprout",     "Sprout",     TS,  sprite_sprout),
    ("Crop1",      "Crop1",      TS,  sprite_crop1),
    ("Crop2",      "Crop2",      TS,  sprite_crop2),
    ("Crop3",      "Crop3",      TS,  sprite_crop3),
    ("Withered",   "Withered",   TS,  sprite_withered),
    ("Cursor",     "Cursor",     TS,  sprite_cursor),
    ("Farmer",     "Farmer",     TS,  sprite_farmer),
    ("WaterCan",   "WaterCan",   TS,  sprite_watercan),
    ("Coin",       "Coin",       TS,  sprite_coin),
    ("Plot",       "Plot",       TS,  sprite_plot),
    ("House",      "House",      32,  sprite_house),
]

def emit(name, rows, dat):
    h = len(rows)
    w = len(rows[0])
    flat = []
    for row in rows:
        flat.extend(row)
    dat.append(f"// {name} ({w}x{h})")
    dat.append(f"static const uint16_t kSp{name}Data[{w * h}] = {{")
    for i in range(0, len(flat), 16):
        dat.append("  " + ", ".join(f"0x{c:04X}" for c in flat[i:i + 16]) + ",")
    dat.append("};")
    dat.append(f"const SpriteArt kSp{name} = {{ kSp{name}Data, {w}, {h} }};")
    dat.append("")

def main():
    hdr = [
        "// Generated by tools/gen_farm_art.py - do NOT edit manually.",
        "// 16x16 (House: 32x32) RGB565 sprites for SPROUT.",
        "// PNG overrides: assets/farm/<Name>.png (same size, 8-bit).",
        "#ifndef GAMES_FARM_ART_H",
        "#define GAMES_FARM_ART_H",
        "",
        "#include <Arduino.h>",
        "",
        "struct SpriteArt { const uint16_t* data; uint8_t w; uint8_t h; };",
        "",
    ]
    dat = [
        "// Generated by tools/gen_farm_art.py - do NOT edit manually.",
        '#include "art.h"',
        "",
    ]
    for name, pngname, size, fn in SPRITES:
        if has_png(pngname):
            cv = sprite_from_png(pngname, size)
            print(f"{name}: loaded from {ASSET_DIR}/{pngname}.png")
        else:
            cv = fn()
        rows = rows_of(cv)
        assert len(rows) == size and all(len(r) == size for r in rows)
        emit(name, rows, dat)
        hdr.append(f"extern const SpriteArt kSp{name};")
    # Sprite table indexed by SpriteId (must match farm.h order)
    hdr.append("")
    hdr.append("// Indexed by SpriteId (see farm.h).")
    hdr.append("extern const SpriteArt* const kSprites[];")
    hdr.append("")
    hdr.append("#endif  // GAMES_FARM_ART_H")
    dat.append("const SpriteArt* const kSprites[] = {")
    for name, _png, _size, _fn in SPRITES:
        dat.append(f"  &kSp{name},")
    dat.append("};")
    with open(OUT_H, "w") as f:
        f.write("\n".join(hdr))
    with open(OUT_C, "w") as f:
        f.write("\n".join(dat))
    total = sum(s * s * 2 for _n, _p, s, _f in SPRITES)
    print(f"-> {OUT_H} + {OUT_C}: {len(SPRITES)} sprites, {total} bytes flash data")

if __name__ == "__main__":
    main()
