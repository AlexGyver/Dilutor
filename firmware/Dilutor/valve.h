#pragma once

#include <Arduino.h>

struct Valve {
  Valve(uint8_t npin) {
    pin = npin;
    digitalWrite(pin, 1);
    pinMode(pin, OUTPUT);
  }

  void on() {
    digitalWrite(pin, 0);
  }
  void off() {
    digitalWrite(pin, 1);
  }

  uint8_t pin;
};
