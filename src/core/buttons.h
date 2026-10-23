#ifndef CORE_BUTTONS_H
#define CORE_BUTTONS_H

#include <Arduino.h>

// Generalisierter Taster mit Entprellung und Short/Long-Erkennung.
// Wird von InputManager verwendet; Spiele kennen keine GPIOs.
struct Button {
  uint8_t  pin       = 0;
  bool     pressed   = false;
  uint32_t t0        = 0;
  bool     longFired = false;
  uint32_t deb       = 0;
};

enum class BtnEvent : uint8_t {
  None,
  Short,   // kurzer Tastendruck (Loslassen vor longMs)
  Long     // langer Tastendruck (longMs erreicht, feuert einmalig)
};

class ButtonTracker {
public:
  ButtonTracker(uint8_t pin, uint32_t longMs, uint32_t debounceMs = 30)
    : longMs_(longMs), debounceMs_(debounceMs) {
    btn_.pin = pin;
    pinMode(pin, INPUT_PULLUP);
  }

  BtnEvent update() {
    uint32_t now = millis();
    BtnEvent ev = BtnEvent::None;
    bool raw = digitalRead(btn_.pin) == LOW;  // aktiv LOW

    if (raw && !btn_.pressed && now - btn_.deb > debounceMs_) {
      btn_.pressed = true;
      btn_.t0 = now;
      btn_.longFired = false;
      btn_.deb = now;
    }
    if (raw && btn_.pressed && !btn_.longFired && now - btn_.t0 >= longMs_) {
      btn_.longFired = true;
      ev = BtnEvent::Long;
    }
    if (!raw && btn_.pressed) {
      btn_.pressed = false;
      if (!btn_.longFired && ev == BtnEvent::None) ev = BtnEvent::Short;
    }
    return ev;
  }

  bool     isDown() const { return btn_.pressed; }
  uint32_t heldMs() const { return btn_.pressed ? millis() - btn_.t0 : 0; }

private:
  Button   btn_;
  uint32_t longMs_;
  uint32_t debounceMs_;
};

#endif
