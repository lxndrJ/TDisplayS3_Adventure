#!/usr/bin/env python3
"""Slices a Godot TileSet (.tres + spritesheet PNG) into 16x16 tiles.

The Super Retro World pack ships its tiles as Godot AtlasTextures: one
big spritesheet PNG plus a .tres that lists every tile as a region
(x, y, w, h) inside that sheet. This tool reads the .tres, cuts the
regions out of the PNG and writes one PNG per tile.

Usage:
    python3 tools/slice_godot_tiles.py <pack_dir> <out_dir> [name_filter ...]

    <pack_dir>    directory containing the .tres and the spritesheet(s)
    <out_dir>     where the sliced PNGs go (created if missing)
    name_filter   optional substrings; only tiles whose .tres key or
                  region name contains one of them are written
                  (case-insensitive). No filter = all tiles.

Example (Super Retro World):
    python3 tools/slice_godot_tiles.py ~/Downloads/super_retro_world out tiles
    # then copy the interesting ones into assets/farm/:
    #   cp out/*crop* assets/farm/Sprout.png   (etc.)

No external dependencies: reuses the PNG loader from gen_farm_art.py
and writes PNGs with a tiny built-in encoder (RGB, filter 0).
"""
import os
import re
import struct
import sys
import zlib

import importlib.util

# Reuse the battle-tested PNG loader from the farm art generator.
_spec = importlib.util.spec_from_file_location(
    "gen_farm_art", os.path.join(os.path.dirname(__file__), "gen_farm_art.py"))
gen = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(gen)

TILE = 16

# ---------------------------------------------------------------- .tres parsing

# Godot 4 .tres section header: [sub_resource type="AtlasTexture" id="AtlasTexture_x"]
SUBRES_HDR_RE = re.compile(
    r'\[sub_resource\s+type="AtlasTexture"\s+id="([^"]+)"\]')
# Legacy inline form (Godot 3 style): SubResource("AtlasTexture_x"): { ... }
SUBRES_INLINE_RE = re.compile(
    r'SubResource\("(AtlasTexture_[^"]+)"\)\s*:\s*\{(.*?)\n\s*\}', re.S)
# Godot 4 ext_resource header: [ext_resource type="Texture2D" path="res://x.png" id="1_y"]
EXT_HDR_RE = re.compile(
    r'\[ext_resource\s+[^\]]*?path="([^"]+)"[^\]]*?id="([^"]+)"\s*\]')
# Legacy inline form: ExtResource("id") = path = "res://..."
EXT_INLINE_RE = re.compile(
    r'ExtResource\("([^"]+)"\)\s*=\s*path\s*=\s*"([^"]+)"', re.S)
RECT_RE = re.compile(r'Rect2i\(\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*\)')


def _parse_atlas_body(key, body, tiles):
    fields = dict(re.findall(r'(atlas|region)\s*=\s*([^\n]+)', body))
    if "atlas" not in fields or "region" not in fields:
        return
    am = re.search(r'ExtResource\("([^"]+)"\)', fields["atlas"])
    rm = RECT_RE.search(fields["region"])
    if not am or not rm:
        return
    x, y, w, h = (int(v) for v in rm.groups())
    tiles.append({"key": key, "atlas": am.group(1),
                  "x": x, "y": y, "w": w, "h": h})


def parse_tres(path):
    """Returns (tiles, images):
    tiles  = list of dicts {key, atlas_id, x, y, w, h}
    images = dict atlas_id -> absolute path of the PNG
    Handles both Godot 4 ([sub_resource ...] blocks) and the legacy
    inline SubResource("..."): { ... } format."""
    text = open(path, encoding="utf-8", errors="replace").read()
    base = os.path.dirname(path)
    images = {}
    # Godot 4 headers first (id comes after path in the attribute list)
    for m in EXT_HDR_RE.finditer(text):
        res_path, rid = m.group(1), m.group(2)
        if res_path.lower().endswith((".png", ".jpg", ".webp")):
            images[rid] = os.path.join(base, res_path.replace("res://", ""))
    # legacy inline form
    for m in EXT_INLINE_RE.finditer(text):
        rid, res_path = m.group(1), m.group(2)
        if res_path.lower().endswith((".png", ".jpg", ".webp")):
            images.setdefault(rid, os.path.join(
                base, res_path.replace("res://", "")))

    tiles = []
    # Godot 4: each [sub_resource type="AtlasTexture" ...] section runs
    # until the next section header ([...])
    matches = list(SUBRES_HDR_RE.finditer(text))
    for i, m in enumerate(matches):
        end = matches[i + 1].start() if i + 1 < len(matches) else len(text)
        # stop at ANY next section header, not just AtlasTexture ones
        nxt = re.search(r'\n\[', text[m.end():end])
        if nxt:
            end = m.end() + nxt.start()
        _parse_atlas_body(m.group(1), text[m.end():end], tiles)
    # legacy inline blocks
    for m in SUBRES_INLINE_RE.finditer(text):
        _parse_atlas_body(m.group(1), m.group(2), tiles)
    return tiles, images


# ---------------------------------------------------------------- PNG writing

def write_png(path, w, h, px):
    """Minimal PNG writer: 8-bit RGB, filter 0 per scanline."""
    raw = bytearray()
    for y in range(h):
        raw.append(0)
        for x in range(w):
            r, g, b = px[y][x]
            raw += bytes((int(r) & 0xFF, int(g) & 0xFF, int(b) & 0xFF))

    def chunk(t, d):
        c = t + d
        return (struct.pack(">I", len(d)) + c
                + struct.pack(">I", zlib.crc32(c) & 0xFFFFFFFF))

    png = (b"\x89PNG\r\n\x1a\n"
           + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
           + chunk(b"IDAT", zlib.compress(bytes(raw), 9))
           + chunk(b"IEND", b""))
    with open(path, "wb") as f:
        f.write(png)


# ---------------------------------------------------------------- main

def sanitize(name):
    return re.sub(r"[^A-Za-z0-9_\-]+", "_", name).strip("_")


def main(argv):
    if len(argv) < 3:
        print(__doc__)
        return 1
    pack_dir, out_dir = argv[1], argv[2]
    filters = [f.lower() for f in argv[3:]]

    tres_files = [f for f in os.listdir(pack_dir) if f.endswith(".tres")]
    if not tres_files:
        print(f"no .tres file found in {pack_dir}")
        return 1
    tres = os.path.join(pack_dir, sorted(tres_files)[0])
    print(f"reading {tres}")
    tiles, images = parse_tres(tres)
    print(f"{len(tiles)} AtlasTexture regions, {len(images)} image(s)")
    os.makedirs(out_dir, exist_ok=True)

    # cache loaded sheets
    sheets = {}
    written = 0
    skipped = 0
    for t in tiles:
        name = t["key"]
        if filters and not any(f in name.lower() for f in filters):
            skipped += 1
            continue
        img_path = images.get(t["atlas"])
        if not img_path or not os.path.exists(img_path):
            continue
        if t["w"] != TILE or t["h"] != TILE:
            # multi-tile regions (autotiles etc.) are skipped
            skipped += 1
            continue
        if img_path not in sheets:
            sheets[img_path] = gen.load_png(img_path)
        _w, _h, src = sheets[img_path]
        px = [[src[t["y"] + y][t["x"] + x] for x in range(TILE)]
              for y in range(TILE)]
        out = os.path.join(out_dir, f"{sanitize(name)}.png")
        write_png(out, TILE, TILE, px)
        written += 1
    print(f"-> {written} tiles written to {out_dir} ({skipped} skipped)")
    print("next: copy the ones you need into assets/farm/ and run")
    print("      python3 tools/gen_farm_art.py")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
