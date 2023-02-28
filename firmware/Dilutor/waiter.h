#pragma once

#include <Arduino.h>
#include "timer.h"

struct Waiter {
  Waiter(uint16_t prd) {
    tout.setMode(TMR_TIMER);
    tout.setPrd(prd);
  }

  bool wait(bool value) {
    if (!value) tout.start();
    return tout.ready();
  }

  void stop() {
    tout.stop();
  }

  Timer tout;
};
