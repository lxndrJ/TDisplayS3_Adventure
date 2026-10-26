#ifndef CORE_BOARD_CONFIG_H
#define CORE_BOARD_CONFIG_H

#include <Arduino.h>

// Create exactly one header file per board in src/boards/ and include it here.
// Games and engine may only use these constants,
// never direct GPIO numbers.
#if defined(BOARD_TDISPLAY_S3)
  #include "boards/board_tdisplay_s3.h"
#else
  #error "No board selected - set a -D BOARD_<NAME> in platformio.ini"
#endif

#endif
