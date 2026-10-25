#!/usr/bin/env python3
"""Generiert die Szenengrafik fuer NEON SHARD (320x170, RGB565, RLE).

Prozedurale Cyberpunk-Comic-Kunst: Skyline-Silhouetten, Neon-Glow,
Charakter-Silhouetten mit Rim-Light, Regen. Keine externen Assets.

Ausgabe: src/games/neonshard/art.h
Aufruf:  python3 tools/gen_neonshard_art.py
"""
import math
import random
import os
import struct
import zlib

W, H = 320, 170
OUT_H = "src/games/neonshard/art.h"
OUT_C = "src/games/neonshard/art.cpp"
ASSET_DIR = "assets/neonshard"   # <Name>.png ersetzt die prozedurale Szene

# ---------------------------------------------------------------- Canvas

class Canvas:
    def __init__(self):
        self.px = [[(0.0, 0.0, 0.0) for _ in range(W)] for _ in range(H)]
        self.glow = [[(0.0, 0.0, 0.0) for _ in range(W)] for _ in range(H)]

    def set(self, x, y, c):
        if 0 <= x < W and 0 <= y < H:
            self.px[y][x] = c

    def add(self, x, y, c):
        if 0 <= x < W and 0 <= y < H:
            r, g, b = self.px[y][x]
            self.px[y][x] = (r + c[0], g + c[1], b + c[2])

    def glow_add(self, x, y, c):
        if 0 <= x < W and 0 <= y < H:
            r, g, b = self.glow[y][x]
            self.glow[y][x] = (r + c[0], g + c[1], b + c[2])

    def rect(self, x0, y0, w, h, c):
        for y in range(max(0, y0), min(H, y0 + h)):
            row = self.px[y]
            for x in range(max(0, x0), min(W, x0 + w)):
                row[x] = c

    def glow_rect(self, x0, y0, w, h, c):
        for y in range(max(0, y0), min(H, y0 + h)):
            row = self.glow[y]
            for x in range(max(0, x0), min(W, x0 + w)):
                r, g, b = row[x]
                row[x] = (r + c[0], g + c[1], b + c[2])

    def disc(self, cx, cy, rad, c):
        for y in range(H):
            for x in range(W):
                dx, dy = x - cx, y - cy
                if dx * dx + dy * dy <= rad * rad:
                    self.px[y][x] = c

    def vgrad(self, y0, y1, c0, c1):
        for y in range(max(0, y0), min(H, y1)):
            t = (y - y0) / max(1, (y1 - y0))
            c = tuple(a + (b - a) * t for a, b in zip(c0, c1))
            self.px[y] = [c for _ in range(W)]

    def hgrad_rect(self, x0, y0, w, h, c0, c1):
        for y in range(max(0, y0), min(H, y0 + h)):
            t0 = (y - y0) / max(1, h)
            for x in range(max(0, x0), min(W, x0 + w)):
                t = t0 * 0.5 + (x - x0) / max(1, w) * 0.5
                c = tuple(a + (b - a) * t for a, b in zip(c0, c1))
                self.px[y][x] = c

    def blur_glow(self, rad=5):
        # getrennte Box-Blurs (horizontal+vertikal, 2 Durchlaeufe)
        for _ in range(2):
            for y in range(H):
                row = self.glow[y]
                out = []
                for x in range(W):
                    r = g = b = 0.0
                    n = 0
                    for d in range(-rad, rad + 1):
                        xx = x + d
                        if 0 <= xx < W:
                            c = row[xx]; r += c[0]; g += c[1]; b += c[2]; n += 1
                    out.append((r / n, g / n, b / n))
                self.glow[y] = out
            for x in range(W):
                col = [self.glow[y][x] for y in range(H)]
                for y in range(H):
                    r = g = b = 0.0
                    n = 0
                    for d in range(-rad, rad + 1):
                        yy = y + d
                        if 0 <= yy < H:
                            c = col[yy]; r += c[0]; g += c[1]; b += c[2]; n += 1
                    self.glow[y][x] = (r / n, g / n, b / n)

    def bake(self, strength=0.85):
        for y in range(H):
            for x in range(W):
                gr, gg, gb = self.glow[y][x]
                self.px[y][x] = tuple(min(255.0, c + g * strength)
                                      for c, g in zip(self.px[y][x], (gr, gg, gb)))

# ---------------------------------------------------------------- RGB565

BAYER2 = [[0.25, 0.75], [1.0, 0.5]]

def to_rgb565(img):
    out = [[0] * W for _ in range(H)]
    for y in range(H):
        for x in range(W):
            r, g, b = img[y][x]
            # 2x2-Block-Dithering haelt die RLE-Laeufe lang
            d = (BAYER2[(y // 2) % 2][(x // 2) % 2] - 0.5) * 3.0
            r = max(0, min(255, int(r + d)))
            g = max(0, min(255, int(g + d)))
            b = max(0, min(255, int(b + d)))
            out[y][x] = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
    return out

def rle(rows):
    runs = []  # (color, count) - niemals ueber Zeilengrenze
    for y in range(H):
        x = 0
        row = rows[y]
        while x < W:
            c = row[x]
            n = 1
            while x + n < W and row[x + n] == c:
                n += 1
            runs.append((c, n))
            x += n
    return runs

# ---------------------------------------------------------------- Elemente

def draw_skyline(cv, base_y, max_h, color, rng, window_cols,
                 wcol_p=0.22, x_off=0):
    x = x_off - 10
    while x < W + 10:
        bw = rng.randint(14, 38)
        bh = rng.randint(int(max_h * 0.3), max_h)
        cv.rect(x, base_y - bh, bw, bh + 10, color)
        # Fenster
        for wy in range(base_y - bh + 3, base_y - 3, 5):
            for wx in range(x + 2, x + bw - 2, 4):
                if rng.random() < wcol_p:
                    cv.set(wx, wy, rng.choice(window_cols))
        # Antennen
        if rng.random() < 0.35:
            ax = x + bw // 2
            for yy in range(base_y - bh - 6, base_y - bh):
                cv.set(ax, yy, color)
            cv.set(ax, base_y - bh - 7, (200, 40, 60))  # rote Warnleuchte
        x += bw + rng.randint(1, 5)

def draw_neon_sign(cv, x, y, w, h, color, label=None):
    cv.rect(x, y, w, h, color)
    cv.glow_rect(x - 1, y - 1, w + 2, h + 2, tuple(c * 0.9 for c in color))
    if label:
        for i, ch in enumerate(label):
            if ch == ' ':
                continue
            # einfache vertikale Glyphenstreifen als Schrift-Anmutung
            cx = x + 3 + i * (w - 6) // max(1, len(label))
            for yy in range(y + 3, y + h - 3):
                if (yy + i) % 3:
                    cv.set(cx, yy, (8, 6, 20))

def draw_figure(cv, cx, top, hgt, coat, rim, wide=14):
    """Silhouette mit Rim-Light - Comic-Charakter."""
    body = coat
    hy = top                      # Kopf
    hr = max(2, hgt // 12)
    # Kopf
    for y in range(hy - hr, hy + hr):
        for x in range(cx - hr, cx + hr + 1):
            if (x - cx) ** 2 + (y - hy) ** 2 <= hr * hr:
                cv.set(x, y, body)
    # Schultern/Mantel
    sh = hy + hr
    bw = wide
    for yy in range(sh, top + hgt):
        t = (yy - sh) / max(1, (top + hgt - sh))
        half = bw * (0.55 + 0.45 * t)          # Mantel wird nach unten breiter
        for x in range(int(cx - half), int(cx + half)):
            cv.set(x, yy, body)
    # Rim-Light: Kanten oben/links cyan
    for yy in range(sh, top + hgt):
        t = (yy - sh) / max(1, (top + hgt - sh))
        half = bw * (0.55 + 0.45 * t)
        cv.set(int(cx - half) - 0, yy, rim)
        cv.set(int(cx - half) + 1, yy, rim)
    for x in range(cx - hr - 1, cx + hr + 1):
        cv.set(x, hy - hr - 1, rim)
    for y in range(hy - hr, hy + hr):
        cv.set(cx - hr - 1, y, rim)

def rain(cv, n, col=(120, 160, 200), slant=2):
    rng = random.Random(99)
    for _ in range(n):
        x = rng.randint(-10, W)
        y = rng.randint(0, H - 30)
        ln = rng.randint(6, 16)
        for i in range(ln):
            xx, yy = x + i * slant // 2, y + i
            c = tuple(min(255.0, cv.px[yy][xx][j] + col[j] * (0.5 + rng.random() * 0.5))
                      for j in range(3)) if 0 <= yy < H and 0 <= xx < W else None
            if c:
                cv.px[yy][xx] = c

def reflections(cv, y0, cols, rng, n=40):
    """Vertikale Lichtsauele auf nassem Boden."""
    for _ in range(n):
        x = rng.randint(0, W - 1)
        c = rng.choice(cols)
        ln = rng.randint(4, 14)
        for dy in range(ln):
            y = y0 + dy
            if y < H:
                cv.add(x, y, tuple(v * 0.25 for v in c))

# Pixel-Font 5x7 fuer den Schriftzug
FONT5 = {
    'A': ["01110","10001","10001","11111","10001","10001","10001"],
    'B': ["11110","10001","10001","11110","10001","10001","11110"],
    'C': ["01110","10001","10000","10000","10000","10001","01110"],
    'D': ["11110","10001","10001","10001","10001","10001","11110"],
    'E': ["11111","10000","10000","11110","10000","10000","11111"],
    'G': ["01110","10001","10000","10111","10001","10001","01110"],
    'H': ["10001","10001","10001","11111","10001","10001","10001"],
    'I': ["11111","00100","00100","00100","00100","00100","11111"],
    'K': ["10001","10010","10100","11000","10100","10010","10001"],
    'L': ["10000","10000","10000","10000","10000","10000","11111"],
    'M': ["10001","11011","10101","10101","10001","10001","10001"],
    'N': ["10001","11001","10101","10011","10001","10001","10001"],
    'O': ["01110","10001","10001","10001","10001","10001","01110"],
    'P': ["11110","10001","10001","11110","10000","10000","10000"],
    'R': ["11110","10001","10001","11110","10100","10010","10001"],
    'S': ["01111","10000","10000","01110","00001","00001","11110"],
    'T': ["11111","00100","00100","00100","00100","00100","00100"],
    'U': ["10001","10001","10001","10001","10001","10001","01110"],
    'V': ["10001","10001","10001","10001","10001","01010","00100"],
    ' ': ["00000","00000","00000","00000","00000","00000","00000"],
}

def draw_text(cv, text, x0, y0, scale, color, glow=True):
    for i, ch in enumerate(text):
        g = FONT5.get(ch)
        if not g:
            continue
        for gy, row in enumerate(g):
            for gx, bit in enumerate(row):
                if bit == '1':
                    for sy in range(scale):
                        for sx in range(scale):
                            xx = x0 + (i * 6 + gx) * scale + sx
                            yy = y0 + gy * scale + sy
                            if glow:
                                cv.glow_add(xx, yy, tuple(c * 0.8 for c in color))
                            cv.set(xx, yy, color)

# ---------------------------------------------------------------- PNG-Import

def load_png(path):
    """Minimaler PNG-Loader (RGB, RGBA, Graustufen, Palette; 8 Bit, nicht
    interlaced). Gibt (w, h, [[(r,g,b), ...], ...]) zurueck."""
    data = open(path, "rb").read()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise SystemExit(f"{path}: keine PNG-Datei")
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
                raise SystemExit(f"{path}: interlaced PNG nicht unterstuetzt")
            if bd != 8:
                raise SystemExit(f"{path}: Bit-Tiefe 8 erwartet (got {bd})")
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
                row.append((float(out[idx]), float(out[idx+1]), float(out[idx+2])))
                idx += 3
            elif ct == 6:
                row.append((float(out[idx]), float(out[idx+1]), float(out[idx+2])))
                idx += 4          # Alpha wird ignoriert (kein Blending noetig)
            elif ct == 0:
                v = float(out[idx]); idx += 1
                row.append((v, v, v))
            elif ct == 3:
                r, g, b = palette[out[idx]]
                row.append((float(r), float(g), float(b)))
                idx += 1
            elif ct == 4:
                v = float(out[idx]); idx += 2
                row.append((v, v, v))
        px.append(row)
    return w, h, px

def png_canvas(name):
    """PNG aus assets/neonshard/<name>.png -> Canvas, skaliert auf 320x170."""
    w, h, src = load_png(os.path.join(ASSET_DIR, f"{name}.png"))
    cv = Canvas()
    for y in range(H):
        sy = y * h // H
        srow = src[sy]
        for x in range(W):
            sx = x * w // W
            cv.px[y][x] = srow[sx]
    return cv

def has_png(name):
    return os.path.exists(os.path.join(ASSET_DIR, f"{name}.png"))

# ---------------------------------------------------------------- Szenen

def scene_title():
    cv = Canvas()
    cv.vgrad(0, H, (12, 8, 44), (66, 26, 78))
    # Mond/Planet mit Corona
    cv.disc(258, 40, 26, (235, 180, 190))
    for y in range(H):
        for x in range(W):
            dx, dy = x - 258, y - 40
            d = math.sqrt(dx * dx + dy * dy)
            if d < 60:
                f = max(0.0, 1.0 - d / 60) ** 2
                cv.glow[y][x] = (140 * f, 70 * f, 90 * f)
    rng = random.Random(7)
    draw_skyline(cv, 120, 52, (24, 18, 52), rng,
                 [(120, 80, 200), (60, 200, 220), (200, 100, 160)], 0.10)
    draw_skyline(cv, 132, 62, (13, 10, 30), rng,
                 [(90, 220, 240), (255, 120, 200), (255, 200, 120)], 0.16)
    # Strasse
    cv.rect(0, 132, W, H - 132, (10, 8, 22))
    reflections(cv, 134, [(90, 220, 240), (255, 120, 200), (200, 100, 160)],
                random.Random(5), 36)
    # Schriftzug mit Glow
    txt = "NEON SHARD"
    tw = len(txt) * 6 * 3
    draw_text(cv, txt, (W - tw) // 2, 34, 3, (255, 240, 210))
    cv.blur_glow(6)
    cv.bake()
    draw_text(cv, txt, (W - tw) // 2, 34, 3, (255, 245, 225), glow=False)
    return cv

def scene_alley():
    cv = Canvas()
    cv.vgrad(0, 100, (20, 10, 48), (90, 30, 70))
    # Fluchtpunkt-Glow am Ende der Gasse
    for y in range(60, 110):
        for x in range(110, 210):
            d = abs(x - 160) / 50 + abs(y - 88) / 25
            f = max(0.0, 1.0 - d) ** 2
            cv.glow[y][x] = (160 * f, 60 * f, 80 * f)
    cv.rect(0, 88, W, 4, (30, 12, 40))          # Bodenlinie Horizont
    # Seitengebaeude (Perspektive)
    rng = random.Random(11)
    for side in (0, 1):
        for i in range(4):
            t = i / 3
            top = 30 + int(t * 40)
            x0 = 0 if side == 0 else W - 1
            wid = int(24 + (1 - t) * 70)
            col = (16, 12, 34)
            if side == 0:
                cv.rect(int(6 + t * 100), top, wid, 92 - top + 40, col)
            else:
                cv.rect(W - wid - int(6 + t * 100), top, wid, 92 - top + 40, col)
    # Neon-Schilder an den Waenden
    draw_neon_sign(cv, 18, 46, 40, 12, (255, 60, 160))
    draw_neon_sign(cv, 250, 62, 46, 12, (60, 220, 255))
    draw_neon_sign(cv, 60, 20, 26, 10, (255, 190, 60))
    draw_neon_sign(cv, 284, 24, 24, 10, (200, 80, 255))
    # Boden: nass
    cv.rect(0, 92, W, H - 92, (14, 10, 26))
    reflections(cv, 94, [(255, 60, 160), (60, 220, 255), (255, 190, 60)],
                random.Random(3), 44)
    # Charakter: Riko (links, Hotspot 48..110 / 86..164)
    draw_figure(cv, 78, 92, 70, (6, 6, 14), (60, 220, 255), wide=13)
    # Drone-Wrack (rechts, Hotspot 238..314 / 108..166)
    cv.rect(250, 132, 52, 18, (30, 26, 44))      # Rumpf
    cv.rect(244, 136, 10, 8, (24, 20, 38))        # Rotorstummel
    cv.rect(296, 130, 12, 14, (24, 20, 38))
    cv.rect(262, 146, 24, 10, (20, 16, 30))       # Riss
    # Blinklicht + Glow
    for y in range(124, 146):
        for x in range(246, 306):
            d = abs(x - 276) / 30 + abs(y - 138) / 12
            f = max(0.0, 1.0 - d) ** 2
            cv.glow[y][x] = (140 * f, 40 * f, 50 * f)
    cv.rect(274, 128, 5, 5, (255, 60, 120))       # Notlicht
    # Splitter-Glimmen in der Ladebuchse
    cv.set(268, 152, (255, 230, 120))
    cv.glow_add(268, 152, (255, 200, 80))
    cv.glow_add(269, 152, (255, 200, 80))
    rain(cv, 90, slant=1)
    cv.blur_glow(5)
    cv.bake()
    return cv

def scene_bar():
    cv = Canvas()
    cv.vgrad(0, H, (28, 12, 30), (58, 20, 26))
    # Rueckwand
    cv.rect(0, 20, W, 120, (34, 16, 26))
    # Regal mit Leuchtflaschen rechts
    cv.rect(228, 34, 84, 66, (24, 12, 20))
    for sy in range(40, 96, 16):
        cv.rect(228, sy, 84, 2, (60, 30, 40))
        rng = random.Random(sy)
        for bx in range(232, 308, 8):
            c = rng.choice([(80, 240, 255), (255, 120, 200), (255, 210, 90),
                             (160, 120, 255)])
            cv.rect(bx, sy - 8, 4, 8, c)
            cv.glow_rect(bx, sy - 8, 4, 8, tuple(v * 0.7 for v in c))
    # Neon-Schild links
    draw_neon_sign(cv, 12, 30, 40, 14, (255, 70, 150))
    draw_neon_sign(cv, 18, 50, 30, 10, (70, 220, 255))
    # Theke
    cv.rect(0, 118, W, 16, (52, 26, 30))
    cv.rect(0, 134, W, 36, (30, 14, 22))
    cv.rect(40, 128, 26, 8, (255, 210, 90))       # Drink-Glow
    cv.glow_rect(38, 126, 30, 12, (180, 140, 50))
    # Charakter: Mama Byte (Mitte, Hotspot 126..202 / 78..166)
    draw_figure(cv, 162, 82, 80, (10, 6, 18), (255, 90, 160), wide=18)
    # Haengende Lampe mit Lichtkegel
    cv.rect(160, 12, 2, 12, (20, 20, 30))
    for yy in range(24, 110):
        t = (yy - 24) / 86
        half = 4 + t * 26
        for xx in range(int(161 - half), int(161 + half)):
            f = (1 - t) * 0.10
            cv.add(xx, yy, (255 * f, 200 * f, 120 * f))
    cv.blur_glow(5)
    cv.bake()
    return cv

def scene_subway():
    cv = Canvas()
    cv.vgrad(0, H, (10, 14, 24), (22, 26, 40))
    # Tunnelwand + Gleise
    cv.rect(0, 100, W, 70, (16, 18, 26))
    cv.rect(0, 118, W, 6, (40, 44, 60))           # Bahnsteigkante
    cv.rect(0, 124, W, 46, (12, 14, 20))
    for x in range(0, W, 24):                      # Schwellen
        cv.rect(x, 148, 14, 3, (24, 22, 30))
    cv.rect(0, 140, W, 3, (70, 74, 90))            # Schiene 1
    cv.rect(0, 144, W, 3, (70, 74, 90))
    # Tunnelmund rechts mit Zuglicht
    cv.rect(236, 40, 84, 84, (6, 7, 12))
    for y in range(40, 124):
        for x in range(236, 320):
            d = abs(x - 278) / 42 + abs(y - 82) / 42
            f = max(0.0, 1.0 - d) ** 2
            cv.glow[y][x] = (170 * f, 170 * f, 190 * f)
    # Signalleuchten
    cv.rect(30, 44, 6, 6, (255, 60, 60))
    cv.glow_rect(28, 42, 10, 10, (120, 30, 30))
    cv.rect(46, 44, 6, 6, (60, 255, 120))
    cv.glow_rect(44, 42, 10, 10, (20, 120, 50))
    # Bank links
    cv.rect(20, 108, 70, 4, (40, 30, 36))
    cv.rect(24, 112, 4, 16, (30, 22, 28))
    cv.rect(80, 112, 4, 16, (30, 22, 28))
    # Charakter: Agentin Chen (Mitte, Hotspot 118..188 / 84..168)
    draw_figure(cv, 150, 88, 74, (8, 8, 16), (255, 245, 235), wide=12)
    # Neonwerbung am Bahnsteig
    draw_neon_sign(cv, 100, 28, 66, 12, (90, 130, 255))
    cv.blur_glow(5)
    cv.bake()
    return cv

def scene_roof():
    cv = Canvas()
    cv.vgrad(0, 110, (14, 10, 40), (60, 24, 66))
    # Sterne
    rng = random.Random(21)
    for _ in range(40):
        x, y = rng.randint(0, W - 1), rng.randint(0, 70)
        cv.set(x, y, (180, 180, 220))
    # Stadt unten
    draw_skyline(cv, 108, 44, (20, 14, 40), rng,
                 [(90, 220, 240), (255, 120, 200)], 0.14)
    cv.rect(0, 108, W, 62, (12, 10, 24))           # Dach
    # Antennen-Wald
    for ax in range(8, W, 46):
        for yy in range(112, 126):
            cv.set(ax, yy, (26, 24, 40))
        cv.set(ax, 111, (150, 40, 60))
    # Hauptantenne (Hotspot 128..194 / 16..156)
    cv.rect(158, 16, 6, 140, (30, 28, 46))
    for yy in range(24, 150, 14):                  # Querstreben
        cv.rect(150, yy, 22, 2, (26, 24, 40))
    cv.rect(144, 130, 34, 26, (22, 20, 36))        # Basis
    # Rotfeuer oben
    cv.rect(157, 10, 8, 8, (255, 50, 60))
    for y in range(0, 40):
        for x in range(130, 190):
            d = abs(x - 161) / 30 + abs(y - 14) / 20
            f = max(0.0, 1.0 - d) ** 2
            cv.glow[y][x] = (170 * f, 30 * f, 40 * f)
    # Parabol-Schuessel
    cv.rect(196, 60, 24, 18, (34, 32, 52))
    cv.rect(199, 56, 18, 6, (40, 38, 60))
    cv.blur_glow(5)
    cv.bake()
    return cv

def scene_end_a():
    cv = Canvas()
    cv.vgrad(0, 96, (255, 150, 80), (255, 90, 130))
    cv.vgrad(96, 110, (255, 90, 130), (120, 70, 140))
    cv.vgrad(110, H, (70, 40, 70), (26, 20, 40))
    # Sonne am Horizont
    cv.disc(160, 96, 22, (255, 220, 150))
    for y in range(70, 120):
        for x in range(120, 200):
            d = math.sqrt((x - 160) ** 2 + (y - 96) ** 2)
            f = max(0.0, 1.0 - d / 40) ** 2
            cv.glow[y][x] = (220 * f, 120 * f, 60 * f)
    # Silhouetten-Stadt
    draw_skyline(cv, 112, 54, (24, 14, 30), random.Random(31),
                 [(255, 200, 120)], 0.06)
    cv.rect(0, 112, W, H - 112, (18, 12, 24))
    # Voegel
    for bx, by in [(90, 60), (104, 52), (120, 64), (210, 58), (226, 66)]:
        cv.rect(bx, by, 3, 1, (30, 16, 30))
        cv.rect(bx - 3, by + 1, 3, 1, (30, 16, 30))
    # Warme Spiegelung auf nassem Dach
    reflections(cv, 114, [(255, 180, 100), (255, 120, 140)], random.Random(9), 40)
    cv.blur_glow(6)
    cv.bake()
    return cv

def scene_end_b():
    cv = Canvas()
    cv.vgrad(0, H, (8, 10, 26), (30, 14, 40))
    rng = random.Random(13)
    draw_skyline(cv, 116, 60, (14, 10, 30), rng,
                 [(90, 220, 240), (255, 120, 200)], 0.12)
    cv.rect(0, 116, W, H - 116, (10, 8, 20))
    # Harte Neonwerbung
    draw_neon_sign(cv, 20, 30, 70, 14, (255, 60, 150))
    draw_neon_sign(cv, 220, 44, 80, 14, (60, 200, 255))
    draw_neon_sign(cv, 120, 20, 40, 10, (255, 200, 60))
    reflections(cv, 118, [(255, 60, 150), (60, 200, 255)], rng, 30)
    # Vex von hinten, allein im Regen
    draw_figure(cv, 160, 96, 72, (4, 4, 10), (120, 170, 220), wide=15)
    rain(cv, 220, slant=2)
    cv.blur_glow(5)
    cv.bake(0.6)
    return cv

# ---------------------------------------------------------------- main

def emit_data(name, runs, out):
    out.append(f"static const RleRun k{name}Data[] = {{")
    for i in range(0, len(runs), 8):
        chunk = ", ".join(f"{{0x{c:04X},{n}}}" for c, n in runs[i:i + 8])
        out.append("  " + chunk + ",")
    out.append("};")
    out.append(f"const RleArt k{name} = {{ k{name}Data, {len(runs)} }};")
    out.append("")

def main():
    scenes = [
        ("ArtTitle", "Title",  scene_title),
        ("ArtAlley", "Alley",  scene_alley),
        ("ArtBar",   "Bar",    scene_bar),
        ("ArtSubway", "Subway", scene_subway),
        ("ArtRoof",  "Roof",   scene_roof),
        ("ArtEndA",  "EndA",   scene_end_a),
        ("ArtEndB",  "EndB",   scene_end_b),
    ]
    # art.h: nur Deklarationen (wird ueber games.h in viele TUs inkludiert)
    hdr = [
        "// Generiert von tools/gen_neonshard_art.py - NICHT manuell editieren.",
        "// 320x170 RGB565, zeilenweise RLE (Laeufe enden am Zeilenende).",
        "// Daten liegen in art.cpp, damit das Header nicht aufblaht.",
        "#ifndef GAMES_NEONSHARD_ART_H",
        "#define GAMES_NEONSHARD_ART_H",
        "",
        "#include <Arduino.h>",
        "",
        "struct RleRun { uint16_t color; uint16_t count; };",
        "struct RleArt  { const RleRun* data; uint16_t runs; };",
        "",
    ]
    # art.cpp: die eigentlichen Bilddaten
    dat = [
        "// Generiert von tools/gen_neonshard_art.py - NICHT manuell editieren.",
        '#include "art.h"',
        "",
    ]
    total = 0
    for name, pngname, fn in scenes:
        if has_png(pngname):
            cv = png_canvas(pngname)
            print(f"{name}: aus {ASSET_DIR}/{pngname}.png geladen")
        else:
            cv = fn()
        rows = to_rgb565(cv.px)
        assert len(rows) == H and all(len(r) == W for r in rows)
        runs = rle(rows)
        total += len(runs)
        print(f"{name}: {len(runs)} RLE-Laeufe ({len(runs) * 4 / 1024:.0f} KB im Flash)")
        emit_data(name, runs, dat)
        hdr.append(f"extern const RleArt k{name};")
    hdr.append("")
    hdr.append("#endif  // GAMES_NEONSHARD_ART_H")
    with open(OUT_H, "w") as f:
        f.write("\n".join(hdr))
    with open(OUT_C, "w") as f:
        f.write("\n".join(dat))
    print(f"-> {OUT_H} + {OUT_C}: {total} Laeufe gesamt")

if __name__ == "__main__":
    main()
