# NEON SHARD - Custom Scene Graphics

Drop PNGs with these names here, then run:

    python3 tools/gen_neonshard_art.py

If a file exists it replaces the procedurally generated scene; if it is
missing, the existing graphic is kept.

| File       | Scene                                  | Hotspots (click zones, see neonshard.cpp)  |
|------------|----------------------------------------|--------------------------------------------|
| Title.png  | Title screen                           | (none, only "B long: Start")               |
| Alley.png  | Alley: drone right, Riko left          | Drone 238,108 + Riko 48,86                 |
| Bar.png    | Mama Bytes Bar, character center       | Mama 126,78                                |
| Subway.png | Platform, Chen center                  | Chen 118,84                                |
| Roof.png   | Rooftop, antenna center                | Antenna 128,16                             |
| EndA.png   | Ending A: morning                      | full area                                  |
| EndB.png   | Ending B: neon rain                    | full area                                  |

Notes:
- Any format/size; automatically scaled to 320x170 (nearest neighbour).
  Best result: crop and scale to 320x170 (or 16:9) yourself first, then
  without heavy detail loss.
- RGB, RGBA, grayscale and palette, 8 bit, not interlaced.
- The bottom third is covered by the dialog band (dark, from y=100) -
  place motifs near the top edge of the image.
- RGB565 quantization adds slight dither noise; it hides banding in
  gradients and does not disturb photos.
