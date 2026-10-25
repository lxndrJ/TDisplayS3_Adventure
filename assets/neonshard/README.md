# NEON SHARD - eigene Szenengrafiken

Lege hier PNGs mit diesen Namen ab und fuehre danach aus:

    python3 tools/gen_neonshard_art.py

Existiert eine Datei, ersetzt sie die prozedural erzeugte Szene; fehlt sie,
wird die alte Grafik weiterverwendet.

| Datei      | Szene                                  | Hotspots (Klickzonen, siehe neonshard.cpp)   |
|------------|----------------------------------------|----------------------------------------------|
| Title.png  | Titelbild                              | (keine, nur "B lang: Start")                 |
| Alley.png  | Gasse: Drone rechts, Riko links        | Drone 238,108 + Riko 48,86                   |
| Bar.png    | Mama Bytes Bar, Figur Mitte            | Mama 126,78                                  |
| Subway.png | Bahnsteig, Chen Mitte                  | Chen 118,84                                  |
| Roof.png   | Turmdach, Antenne Mitte                | Antenne 128,16                               |
| EndA.png   | Ende A: Morgen                         | ganzflaechig                                 |
| EndB.png   | Ende B: Neonregen                      | ganzflaechig                                 |

Hinweise:
- Beliebiges Format/Massstab; skaliert automatisch auf 320x170 (nearest
  neighbour). Bestes Ergebnis: vorher selbst auf 320x170 (oder 16:9) crop
  und skalieren, dann ohne staerke Detailverluste.
- RGB, RGBA, Graustufen und Palette, 8 Bit, nicht interlaced.
- Das untere Drittel wird vom Dialogband (dunkel, ab y=100) ueberdeckt -
  Motive am oberen Bildrand platzieren.
- Die RGB565-Quantisierung fuegt ein leichtes Dither-Rauschen hinzu; das
  kaschiert Banding bei Verlaeufen und stoert bei Fotos nicht.
