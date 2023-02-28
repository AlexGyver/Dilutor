#pragma once
// простенький таймер с режимом таймера и периода

#include <Arduino.h>
#define TMR_PERIOD 0
#define TMR_TIMER 1

struct Timer {
  Timer() {}
  Timer(bool nmode, uint16_t nprd) {
    setMode(nmode);
    setPrd(nprd);
  }

  void setPrd(uint16_t nprd) {
    prd = nprd;
  }

  void start() {
    tmr = millis();
    state = 1;
  }
  void stop() {
    state = 0;
  }
  bool running() {
    return state;
  }

  void setMode(bool nmode) {
    mode = nmode;
  }

  bool ready() {
    if (state && (uint16_t)millis() - tmr >= prd) {
      tmr = millis();
      if (mode) state = 0;
      return 1;
    }
    return 0;
  }

  uint16_t prd;
  uint16_t tmr;
  bool mode = 0;
  bool state = 0;
};
