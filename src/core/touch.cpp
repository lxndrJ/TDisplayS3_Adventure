#include "touch.h"

TouchPanel touchPanel;

void touchNotifyRotation(uint8_t r) {
  touchPanel.setRotation(r);
}
