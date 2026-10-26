#ifndef CORE_INPUT_H
#define CORE_INPUT_H

#include "board_config.h"
#include "buttons.h"

// Unified logical keys - independent of the board.
// Each board maps its physical buttons onto these in board_config.h.
enum class Key : uint8_t { A, B, KeyCount };

struct InputState {
  // true for the whole duration of the press
  bool     down[(uint8_t)Key::KeyCount]      = {false, false};
  // true only in the frame of release (short press)
  bool     pressed[(uint8_t)Key::KeyCount]   = {false, false};
  // true once after longMs of holding
  bool     longPressed[(uint8_t)Key::KeyCount] = {false, false};
  // hold time in ms (0 = not pressed)
  uint32_t holdMs[(uint8_t)Key::KeyCount]    = {0, 0};
};

class InputManager {
public:
  void init() {
    // Power-hold is set by Engine::begin() before gfx.init().
    btnA_ = new ButtonTracker(board::PIN_BTN_A, board::LONG_A_MS, board::DEBOUNCE_MS);
    btnB_ = new ButtonTracker(board::PIN_BTN_B, board::LONG_B_MS, board::DEBOUNCE_MS);
  }

  // Must be called every frame. Polls both buttons and
  // fills the passed state.
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
