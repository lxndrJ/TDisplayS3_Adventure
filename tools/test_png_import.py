"""Quick-Test des PNG-Imports in gen_neonshard_art.py (nicht Teil des Builds)."""
import importlib.util
import os
import struct
import zlib

spec = importlib.util.spec_from_file_location("gen", "tools/gen_neonshard_art.py")
gen = importlib.util.module_from_spec(spec)
spec.loader.exec_module(gen)

os.makedirs(gen.ASSET_DIR, exist_ok=True)

def chunk(t, d):
    c = t + d
    return struct.pack(">I", len(d)) + c + struct.pack(">I", zlib.crc32(c) & 0xffffffff)

def write_png(path, w, h, pix_fn):
    bpp = 3
    raw = b""
    for y in range(h):
        orig = bytes(v for x in range(w) for v in pix_fn(x, y))
        # Sub-Filter korrekt anwenden: byte[i] = orig[i] - orig[i-bpp]
        line = bytearray(orig)
        for i in range(bpp, len(line)):
            line[i] = (line[i] - orig[i - bpp]) & 0xFF
        raw += bytes([1]) + line
    png = (b"\x89PNG\r\n\x1a\n"
           + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
           + chunk(b"IDAT", zlib.compress(raw))
           + chunk(b"IEND", b""))
    open(path, "wb").write(png)

# RGB-Verlauf mit Filter 1 (Sub), skaliert 16x8 -> 320x170
write_png(os.path.join(gen.ASSET_DIR, "__test.png"), 16, 8,
          lambda x, y: (x * 16, y * 32, 128))
cv = gen.png_canvas("__test")
assert len(cv.px) == 170 and len(cv.px[0]) == 320
assert cv.px[0][0] == (0.0, 0.0, 128.0), cv.px[0][0]
assert cv.px[169][319] == (240.0, 224.0, 128.0), cv.px[169][319]
print("RGB-Filter1: OK", cv.px[0][0], cv.px[169][319])

# Graustufen + Paletten-Bild
def write_png_ct(path, w, h, ct, bd, extra, raw_fn):
    ihdr = struct.pack(">IIBBBBB", w, h, bd, ct, 0, 0, 0)
    raw = b""
    for y in range(h):
        raw += b"\x00" + raw_fn(y, w)
    png = (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", ihdr) + extra
           + chunk(b"IDAT", zlib.compress(raw)) + chunk(b"IEND", b""))
    open(path, "wb").write(png)

write_png_ct(os.path.join(gen.ASSET_DIR, "__test.png"), 8, 8, 0, 8, b"",
             lambda y, w: bytes(range(0, w * 1)))
cv = gen.png_canvas("__test")
assert cv.px[0][0] == (0.0, 0.0, 0.0), cv.px[0][0]
assert cv.px[85][319] == (7.0, 7.0, 7.0), cv.px[85][319]
print("Graustufen: OK")

plte = chunk(b"PLTE", bytes([255, 0, 160,  0, 200, 255]))
write_png_ct(os.path.join(gen.ASSET_DIR, "__test.png"), 8, 8, 3, 8, plte,
             lambda y, w: bytes((x + y) % 2 for x in range(w)))
cv = gen.png_canvas("__test")
assert cv.px[0][0] == (255.0, 0.0, 160.0), cv.px[0][0]
print("Palette: OK")

os.remove(os.path.join(gen.ASSET_DIR, "__test.png"))
print("Alle PNG-Import-Tests bestanden.")
