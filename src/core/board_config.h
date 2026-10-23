#ifndef CORE_BOARD_CONFIG_H
#define CORE_BOARD_CONFIG_H

#include <Arduino.h>

// Pro Board genau eine Header-Datei in src/boards/ anlegen und hier einbinden.
// Spiele und Engine duerfen ausschliesslich diese Konstanten verwenden,
// niemals direkte GPIO-Nummern.
#if defined(BOARD_TDISPLAY_S3)
  #include "boards/board_tdisplay_s3.h"
#else
  #error "Kein Board gewaehlt - in platformio.ini ein -D BOARD_<NAME> setzen"
#endif

#endif
