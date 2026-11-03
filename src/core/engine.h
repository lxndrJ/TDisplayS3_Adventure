#ifndef CORE_ENGINE_H
#define CORE_ENGINE_H

#include <Arduino.h>
#include "display.h"
#include "input.h"
#include "game.h"
#include "games/games.h"

// Launcher: shows the game menu and starts the selected game. A game
// sets wantsMenu to return here (B long is the convention).
// Games may set their own rotation in init() (e.g. landscape);
// the engine restores the board's portrait rotation before each start
// and in the menu.
class Engine {
public:
  void begin(Game* boot) {
    Serial.begin(115200);
    // Power-hold BEFORE display init: without HIGH on GPIO15 the
    // panel on the T-Display-S3 stays unpowered (black screen, backlight off).
    boardPowerOn();
    gfx.init();
    input.init();
    randomSeed(esp_random());
    active_ = boot;
    if (active_) active_->init();
    else         menuDirty_ = true;
  }

  void loop() {
    uint32_t now = millis();
    uint32_t dt = now - lastMs_;
    lastMs_ = now;

    InputState in;
    input.update(in);

    bool drew = false;
    if (active_) {
      active_->update(in, dt);
      active_->draw();
      drew = true;
      if (active_->wantsMenu) {
        active_->quit();
        active_ = nullptr;
        menuDirty_ = true;
      }
    } else {
      menuUpdate(in);
      drew = menuDraw();
    }

    // QSPI boards: push the frame canvas to the panel - but only when
    // something was actually drawn (a full flush costs ~60 ms over QSPI and
    // would otherwise slow down touch polling in the idle menu).
    if (drew) gfx.present();

    // Frame pacing (~60 FPS): without a cap the engine renders as fast
    // as the bus allows - animations flicker, dt becomes erratic.
    uint32_t used = millis() - now;
    if (used < FRAME_MS) delay(FRAME_MS - used);
  }

private:
  static constexpr uint32_t FRAME_MS = 16;  // ~60 FPS

  static void boardPowerOn() {
    if constexpr (board::HAS_POWER_HOLD) {
      pinMode(board::PIN_POWER_ON, OUTPUT);
      digitalWrite(board::PIN_POWER_ON, HIGH);
      delay(10);  // let the panel power stabilize
    }
  }

  void menuUpdate(const InputState& in) {
    if (in.pressed[(uint8_t)Key::A]) {
      menuSel_ = (menuSel_ + GAME_COUNT - 1) % GAME_COUNT;
      menuDirty_ = true;
    }
    if (in.pressed[(uint8_t)Key::B]) {
      menuSel_ = (menuSel_ + 1) % GAME_COUNT;
      menuDirty_ = true;
    }
    if (in.longPressed[(uint8_t)Key::B]) {
      active_ = makeGame(menuSel_);
      if (active_) active_->init();
    }
    // Touch boards: a tap directly on a menu entry starts that game.
    if constexpr (board::HAS_TOUCH) {
      if (in.touchTap) {
        Serial.printf("[touch] tap x=%d y=%d\n", in.touchX, in.touchY);
        // nearest row (round, not floor) so slightly-off coordinates still hit
        if (in.touchX >= 0 && in.touchX < gfx.width() &&
            in.touchY >= MENU_Y0 - MENU_ROW_H / 2 &&
            in.touchY < MENU_Y0 + GAME_COUNT * MENU_ROW_H) {
          int16_t row = (in.touchY - MENU_Y0 + MENU_ROW_H / 2) / MENU_ROW_H;
          if (row < 0) row = 0;
          if (row >= GAME_COUNT) row = GAME_COUNT - 1;
          Serial.printf("[touch] start game %d\n", row);
          menuSel_ = (uint8_t)row;
          active_ = makeGame(menuSel_);
          if (active_) active_->init();
        }
      }
    }
  }

  // Returns true when the menu was redrawn (frame needs a present()).
  bool menuDraw() {
    if (!menuDirty_) return false;
    menuDirty_ = false;
    TFT_eSPI& tft = gfx.tft();
    tft.setRotation(board::TFT_ROTATION);
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(TL_DATUM);
    tft.setTextFont(2);  // title: 16px on TFT_eSPI, ~44px (FreeSans x2) on touch
    tft.setTextColor(0x07FF);  // Neon-Cyan
    tft.drawString(board::BOARD_NAME, 6, 8);
    tft.setTextFont(MENU_ROW_FONT);
    tft.setTextColor(0x7BEF);
    tft.drawString("Games", 6, MENU_SUB_Y);
    tft.drawFastHLine(0, MENU_LINE_Y, gfx.width(), 0x8010);
    for (uint8_t i = 0; i < GAME_COUNT; i++) {
      bool sel = (i == menuSel_);
      tft.setTextFont(MENU_ROW_FONT);
      tft.setTextColor(sel ? 0xFFE0 : 0x7BEF);  // Gelb = ausgewaehlt
      tft.drawString(sel ? "> " : "  ", 12, MENU_Y0 + i * MENU_ROW_H);
      tft.drawString(makeGame(i)->name(), 34, MENU_Y0 + i * MENU_ROW_H);
    }
    tft.setTextFont(1);
    tft.setTextColor(0x7BEF);
    if constexpr (board::HAS_TOUCH && !board::HAS_BTN_B) {
      tft.drawString("Touch: weiter | Tippen = Start", 6, MENU_HINT_Y);
    } else {
      tft.drawString("A/B: select | B long: Start", 6, MENU_HINT_Y);
    }
    return true;
  }

  // Menu geometry. On touch boards the facade font is bigger (FreeSans:
  // font 2 ~44px line, font 1 ~22px), so rows use font 1 with a wider pitch.
  static constexpr uint8_t MENU_ROW_FONT = board::HAS_TOUCH ? 1 : 2;
  static constexpr int16_t MENU_SUB_Y    = board::HAS_TOUCH ? 52 : 26;
  static constexpr int16_t MENU_Y0       = board::HAS_TOUCH ? 84 : 56;  // first menu row
  static constexpr int16_t MENU_ROW_H    = board::HAS_TOUCH ? 28 : 24;  // row pitch (px)
  static constexpr int16_t MENU_LINE_Y   = MENU_Y0 - 6;
  // hint line: font 1 is ~22px on touch boards, so keep it clear of the edge
  static constexpr int16_t MENU_HINT_Y   = board::SCR_H - (board::HAS_TOUCH ? 26 : 12);

  Game*    active_  = nullptr;
  uint8_t  menuSel_  = 0;
  bool     menuDirty_ = false;
  uint32_t lastMs_   = 0;
};

extern Engine engine;

#endif
