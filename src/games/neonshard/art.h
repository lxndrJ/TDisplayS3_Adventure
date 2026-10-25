// Generiert von tools/gen_neonshard_art.py - NICHT manuell editieren.
// 320x170 RGB565, zeilenweise RLE (Laeufe enden am Zeilenende).
// Daten liegen in art.cpp, damit das Header nicht aufblaht.
#ifndef GAMES_NEONSHARD_ART_H
#define GAMES_NEONSHARD_ART_H

#include <Arduino.h>

struct RleRun { uint16_t color; uint16_t count; };
struct RleArt  { const RleRun* data; uint16_t runs; };

extern const RleArt kArtTitle;
extern const RleArt kArtAlley;
extern const RleArt kArtBar;
extern const RleArt kArtSubway;
extern const RleArt kArtRoof;
extern const RleArt kArtEndA;
extern const RleArt kArtEndB;

#endif  // GAMES_NEONSHARD_ART_H