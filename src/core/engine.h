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
      // Touch boards: a small "Menu" button in the top-right corner of every
      // game = back to the launcher (like A long on button boards).
      if constexpr (board::HAS_TOUCH) {
        if (in.touchTap && in.touchX >= gfx.width() - BACK_W && in.touchY < BACK_H) {
          active_->wantsMenu = true;
        }
        TFT_eSPI& tft = gfx.tft();
        tft.fillRect(gfx.width() - BACK_W, 0, BACK_W, BACK_H, 0x2104);
        tft.drawRect(gfx.width() - BACK_W, 0, BACK_W, BACK_H, 0x07FF);
        tft.setTextDatum(MC_DATUM);
        tft.setTextFont(1);
        tft.setTextColor(0x07FF);
        tft.drawString("Menu", gfx.width() - BACK_W / 2, BACK_H / 2 - 1);
      }
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
    // Touch boards: tap directly on a menu button starts that game.
    if constexpr (board::HAS_TOUCH) {
      if (in.touchTap) {
        Serial.printf("[touch] tap x=%d y=%d\n", in.touchX, in.touchY);
        int16_t col = (in.touchX - GRID_X) / (GRID_CW + GRID_GAP);
        int16_t row = (in.touchY - GRID_Y) / (GRID_CH + GRID_GAP);
        if (col >= 0 && col < GRID_COLS && row >= 0 && row < GRID_ROWS &&
            in.touchX >= GRID_X && in.touchX < GRID_X + GRID_COLS * GRID_CW + (GRID_COLS - 1) * GRID_GAP &&
            in.touchY >= GRID_Y && in.touchY < GRID_Y + GRID_ROWS * GRID_CH + (GRID_ROWS - 1) * GRID_GAP) {
          uint8_t idx = (uint8_t)(row * GRID_COLS + col);
          if (idx < GAME_COUNT) {
            Serial.printf("[touch] start game %d\n", idx);
            menuSel_ = idx;
            active_ = makeGame(menuSel_);
            if (active_) active_->init();
          }
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
    if constexpr (board::HAS_TOUCH) {
      // Touch board: 3x3 grid of big tappable buttons.
      for (uint8_t i = 0; i < GAME_COUNT; i++) {
        uint8_t col = i % GRID_COLS, row = i / GRID_COLS;
        int16_t x = GRID_X + col * (GRID_CW + GRID_GAP);
        int16_t y = GRID_Y + row * (GRID_CH + GRID_GAP);
        bool sel = (i == menuSel_);
        tft.fillRect(x, y, GRID_CW, GRID_CH, sel ? 0x0861 : 0x2104);
        tft.drawRect(x, y, GRID_CW, GRID_CH, sel ? 0xFFE0 : 0x07FF);
        tft.setTextDatum(MC_DATUM);
        tft.setTextFont(1);
        tft.setTextColor(sel ? 0xFFE0 : 0xCFF7);
        tft.drawString(makeGame(i)->name(), x + GRID_CW / 2, y + GRID_CH / 2 - 1);
      }
      tft.setTextDatum(TL_DATUM);
      tft.setTextFont(1);
      tft.setTextColor(0x7BEF);
      tft.drawString("Tippen = Spiel starten", 6, MENU_HINT_Y);
    } else {
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
      tft.drawString("A/B: select | B long: Start", 6, MENU_HINT_Y);
    }
    return true;
  }

  // Menu geometry. Touch board (JC3248W535, landscape 480x320): 3x3 button
  // grid (big tap targets). S3 (portrait 170x320, TFT_eSPI) keeps the
  // classic text rows.
  static constexpr uint8_t MENU_ROW_FONT = board::HAS_TOUCH ? 1 : 2;
  static constexpr int16_t MENU_SUB_Y    = 26;
  static constexpr int16_t MENU_Y0       = 56;   // first text row (S3 only)
  static constexpr int16_t MENU_ROW_H    = 24;   // row pitch (S3 only)
  static constexpr int16_t MENU_LINE_Y   = MENU_Y0 - 6;
  static constexpr int16_t MENU_HINT_Y   = board::SCR_H - (board::HAS_TOUCH ? 26 : 12);
  static constexpr int16_t GRID_COLS  = 3;
  static constexpr int16_t GRID_ROWS  = 3;
  static constexpr int16_t GRID_X     = 10;
  static constexpr int16_t GRID_Y     = 62;
  static constexpr int16_t GRID_GAP   = 8;
  static constexpr int16_t GRID_CW    = (board::SCR_W - 2 * GRID_X - (GRID_COLS - 1) * GRID_GAP) / GRID_COLS;
  static constexpr int16_t GRID_CH    = 68;
  // global "back to menu" button zone, top-right corner (touch boards)
  static constexpr int16_t BACK_W = 64;
  static constexpr int16_t BACK_H = 28;

  Game*    active_  = nullptr;
  uint8_t  menuSel_  = 0;
  bool     menuDirty_ = false;
  uint32_t lastMs_   = 0;
};

extern Engine engine;

#endif
