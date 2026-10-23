#ifndef CORE_INPUT_H
#define CORE_INPUT_H

#include "board_config.h"
#include "buttons.h"

// Einheitliche logische Tasten - unabhaengig vom Board.
// Jedes Board mappt seine physischen Taster in board_config.h darauf.
enum class Key : uint8_t { A, B, KeyCount };

struct InputState {
  // true waehrend des gesamten Tastendrucks
  bool     down[(uint8_t)Key::KeyCount]      = {false, false};
  // true nur im Frame des Loslassens (kurzer Druck)
  bool     pressed[(uint8_t)Key::KeyCount]   = {false, false};
  // true einmalig nach longMs Haltezeit
  bool     longPressed[(uint8_t)Key::KeyCount] = {false, false};
  // Haltezeit in ms (0 = nicht gedrueckt)
  uint32_t holdMs[(uint8_t)Key::KeyCount]    = {0, 0};
};

class InputManager {
public:
  void init() {
    if constexpr (board::HAS_POWER_HOLD) {
      pinMode(board::PIN_POWER_ON, OUTPUT);
      digitalWrite(board::PIN_POWER_ON, HIGH);
    }
    btnA_ = new ButtonTracker(board::PIN_BTN_A, board::LONG_A_MS, board::DEBOUNCE_MS);
    btnB_ = new ButtonTracker(board::PIN_BTN_B, board::LONG_B_MS, board::DEBOUNCE_MS);
  }

  // Muss jeden Frame aufgerufen werden. Fragt beide Taster ab und
  // fuellt den uebergebenen State.
  void update(InputState& s) {
    s = InputState{};
    updateOne(*btnA_, Key::A, s);
    updateOne(*btnB_, Key::B, s);
  }

private:
  void updateOne(ButtonTracker& t, Key k, InputState& s) {
    uint8_t i = (uint8_t)k;
    BtnEvent ev = t.update();
    s.down[i]        = t.isDown();
    s.holdMs[i]      = t.heldMs();
    s.pressed[i]     = (ev == BtnEvent::Short);
    s.longPressed[i] = (ev == BtnEvent::Long);
  }

  ButtonTracker* btnA_ = nullptr;
  ButtonTracker* btnB_ = nullptr;
};

extern InputManager input;

#endif
