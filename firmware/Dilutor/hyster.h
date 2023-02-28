#pragma once
// "гистерезис" для обработки крутилки

#include <Arduino.h>
#include "timer.h"

#define HYST_MIN_V 50
#define HYST_MAX_V 950

struct Hyster {
  Hyster(uint8_t npin) : pin(npin) {
    tmr.setMode(TMR_PERIOD);
    tmr.setPrd(100);
    tmr.start();

    tout.setMode(TMR_TIMER);
    tout.setPrd(3000);
  }

  bool tick() {
    if (tmr.ready()) {
      if (tout.ready()) state = 0;
      int v = analogRead(pin);

      switch (state) {
        case 0:
          if (v < HYST_MIN_V) {
            state = 1;
            tout.start();
          }
          break;
        case 1:
          if (v > HYST_MAX_V) state = 2;
          break;
        case 2:
          if (v > HYST_MIN_V && v < HYST_MAX_V) {
            state = 0;
            tout.stop();
            return 1;
          }
          break;
      }
    }
    return 0;
  }

  uint8_t state;
  uint8_t pin;
  Timer tmr, tout;
};
