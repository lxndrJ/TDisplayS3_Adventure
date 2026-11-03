#ifndef CORE_INPUT_H
#define CORE_INPUT_H

#include "board_config.h"
#include "buttons.h"
#include "touch.h"

// Unified logical keys - independent of the board.
// Each board maps its physical buttons onto these in board_config.h.
// On touch boards without a second button (HAS_BTN_B == false), a touch
// tap/hold is mapped onto Key::B.
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
  // Touch state (boards with HAS_TOUCH). Coordinates are mapped to the
  // current display rotation.
  bool     touchDown = false;  // finger currently down
  bool     touchTap  = false;  // short tap released this frame
  bool     touchLong = false;  // held >= LONG_B_MS, fires once
  int16_t  touchX = 0;
  int16_t  touchY = 0;
};

class InputManager {
public:
  void init() {
    // Power-hold is set by Engine::begin() before gfx.init().
    btnA_ = new ButtonTracker(board::PIN_BTN_A, board::LONG_A_MS, board::DEBOUNCE_MS);
    if constexpr (board::HAS_BTN_B) {
      btnB_ = new ButtonTracker(board::PIN_BTN_B, board::LONG_B_MS, board::DEBOUNCE_MS);
    }
    if constexpr (board::HAS_TOUCH) {
      touchPanel.init();
      touchPanel.setRotation(board::TFT_ROTATION);
    }
  }

  // Must be called every frame. Polls the buttons (and the touch panel)
  // and fills the passed state.
  void update(InputState& s) {
    s = InputState{};
    updateOne(*btnA_, Key::A, s);
    if constexpr (board::HAS_BTN_B) updateOne(*btnB_, Key::B, s);
    if constexpr (board::HAS_TOUCH) updateTouch(s);
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

  // Polls the touch panel with debounce + long-press detection. On boards
  // without a physical B button, tap/hold also drive Key::B so that every
  // game stays playable with BOOT + touch only.
  void updateTouch(InputState& s) {
    bool down = false;
    int16_t x = lastX_, y = lastY_;  // poll() keeps these when not touched
    touchPanel.poll(down, x, y);
    if (down) {
      lastX_ = x;
      lastY_ = y;
    }
    uint32_t now = millis();
    s.touchDown = down;
    s.touchX = lastX_;  // last known position - also valid for the tap event
    s.touchY = lastY_;
    if (down && !touchDown_ && now - touchDebMs_ > board::DEBOUNCE_MS) {
      touchDown_ = true;
      touchT0_ = now;
      touchLongFired_ = false;
      touchDebMs_ = now;
      Serial.printf("[touch] down x=%d y=%d\n", lastX_, lastY_);
    }
    if (down && touchDown_ && !touchLongFired_ && now - touchT0_ >= board::LONG_B_MS) {
      touchLongFired_ = true;
      s.touchLong = true;
    }
    if (!down && touchDown_) {
      touchDown_ = false;
      if (!touchLongFired_) s.touchTap = true;
    }
    if constexpr (!board::HAS_BTN_B) {
      uint8_t i = (uint8_t)Key::B;
      s.down[i]        = touchDown_;
      s.holdMs[i]      = touchDown_ ? now - touchT0_ : 0;
      s.pressed[i]     = s.touchTap;
      s.longPressed[i] = s.touchLong;
    }
  }

  ButtonTracker* btnA_ = nullptr;
  ButtonTracker* btnB_ = nullptr;
  bool     touchDown_ = false;
  bool     touchLongFired_ = false;
  uint32_t touchT0_ = 0;
  uint32_t touchDebMs_ = 0;
  int16_t  lastX_ = 0;
  int16_t  lastY_ = 0;
};

extern InputManager input;

#endif
