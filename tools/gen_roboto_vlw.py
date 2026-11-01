#!/usr/bin/env python3
"""Generate TFT_eSPI .vlw smooth fonts from Roboto-Regular.ttf.

.vlw layout (reverse-engineered in TFT_eSPI Extensions/Smooth_font.cpp):
  header: 6 x int32 BE  -> gCount, version(11), fontSizePx, 0, ascent, descent
  per glyph (gCount x 7 x int32 BE): unicode, height, width, xAdvance,
                                     gdY (baseline->bitmap top), gdX (cursor->bitmap left), 0
  bitmaps: gCount x (width*height) bytes, 8-bit alpha, row-major
  trailer: len(u8)+name\\0, len(u8)+postscript\\0, smooth flag u8 (1)

Charset: 0x20-0x7E (Basic Latin) + 0xA0-0xFF (Latin-1 Supplement, incl.
umlauts) so UTF-8 text from the LLM renders with real Umlaute.

Usage: gen_roboto_vlw.py <Roboto-Regular.ttf> <out.vlw> <sizePx> <name>
"""
import struct
import sys

from fontTools.ttLib import TTFont
from PIL import Image, ImageDraw, ImageFont


def glyph_metrics(tt, pil_font, upm, code, size):
    cmap = tt.getBestCmap()
    if code not in cmap:
        return None
    gname = cmap[code]
    hmtx = tt["hmtx"]
    advance = hmtx[gname][0] * size / upm
    bbox = pil_font.getbbox(chr(code))  # (x0, y0, x1, y1), y down from ascender top
    if bbox is None:
        bbox = (0, 0, 0, 0)
    x0, y0, x1, y1 = bbox
    w = max(0, x1 - x0)
    h = max(0, y1 - y0)
    ascent, _descent = pil_font.getmetrics()
    # gdY: distance from baseline up to top edge of bitmap
    gdY = ascent - y0
    gdX = x0
    return int(round(advance)), w, h, gdY, gdX


def render_glyph(pil_font, code, w, h, x0, y0):
    if w == 0 or h == 0:
        return bytes(w * h)
    img = Image.new("L", (w + 8, h + 8), 0)
    d = ImageDraw.Draw(img)
    d.text((4 - x0, 4 - y0), chr(code), font=pil_font, fill=255)
    return img.crop((4, 4, 4 + w, 4 + h)).tobytes()


def main():
    ttf_path, out_path, size, name = sys.argv[1], sys.argv[2], int(sys.argv[3]), sys.argv[4]
    tt = TTFont(ttf_path)
    upm = tt["head"].unitsPerEm
    pil = ImageFont.truetype(ttf_path, size)
    ascent, descent = pil.getmetrics()

    codes = list(range(0x20, 0x7F)) + list(range(0xA0, 0x100))
    glyphs = []
    for c in codes:
        m = glyph_metrics(tt, pil, upm, c, size)
        if m is None:
            continue
        adv, w, h, gdY, gdX = m
        bbox = pil.getbbox(chr(c))
        bmp = render_glyph(pil, c, w, h, bbox[0], bbox[1]) if (w and h) else b""
        glyphs.append((c, h, w, adv, gdY, gdX, bmp))

    gcount = len(glyphs)
    with open(out_path, "wb") as f:
        f.write(struct.pack(">6i", gcount, 11, size, 0, ascent, descent))
        for c, h, w, adv, gdY, gdX, _ in glyphs:
            f.write(struct.pack(">7i", c, h, w, adv, gdY, gdX, 0))
        for g in glyphs:
            f.write(g[6])
        f.write(bytes([len(name)]) + name.encode() + b"\x00")
        f.write(bytes([len(name)]) + name.encode() + b"\x00")
        f.write(bytes([1]))
    import os
    print(f"{out_path}: {gcount} glyphs, size {size}px, "
          f"ascent {ascent} descent {descent}, {os.path.getsize(out_path)} bytes")


if __name__ == "__main__":
    main()
