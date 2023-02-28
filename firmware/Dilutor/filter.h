#pragma once
// фильтр скользящее среднее

#include <Arduino.h>
#include "timer.h"

struct Filter {
  Filter() {
    tmr.setPrd(30);
    tmr.setMode(TMR_PERIOD);
    tmr.start();
  }

  void tick() {
    if (tmr.ready()) fvalue += (value - fvalue) / 2L;
  }

  void set(int32_t v) {
    if (!start) {
      fvalue = v;
      start = 1;
    }
    value = v;
  }

  int32_t get() {
    return fvalue;
  }

  int32_t value;
  int32_t fvalue;
  bool start = 0;
  Timer tmr;
};
