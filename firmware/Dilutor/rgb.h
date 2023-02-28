#pragma once
// класс управления RGB светодиодом
// с возможностью плавного асинхронного мигания

#include <Arduino.h>
#include "timer.h"

struct RGBled {
  RGBled(uint8_t nr, uint8_t ng, uint8_t nb) {
    rpin = nr;
    gpin = ng;
    bpin = nb;
    pinMode(rpin, OUTPUT);
    pinMode(gpin, OUTPUT);
    pinMode(bpin, OUTPUT);

    tmr.setMode(TMR_PERIOD);
    tmr.setPrd(30);
  }

  void set(uint8_t nr, uint8_t ng, uint8_t nb) {
    r = nr;
    g = ng;
    b = nb;
    update();
  }

  void setBright(uint8_t nbr) {
    br = ((uint16_t)(nbr) * (nbr) + 255) >> 8;  // gamma
    update();
  }

  void update() {
    analogWrite(rpin, fade8(r, br));
    analogWrite(gpin, fade8(g, br));
    analogWrite(bpin, fade8(b, br));
  }

  uint8_t fade8(uint8_t col, uint8_t br) {
    return ((uint16_t)col * (br + 1)) >> 8;
  }

  void breath(uint8_t nbre) {
    brStep = nbre;
    curBre = 0;
    if (brStep) tmr.start();
    else tmr.stop();
  }

  void breathMax(uint8_t nmax) {
    maxBr = nmax;
  }
  void breathMin(uint8_t nmin) {
    minBr = nmin;
  }

  void tick() {
    if (brStep && tmr.ready()) {
      curBre += brStep * dir;
      if (curBre < minBr) {
        curBre = minBr;
        dir = 1;
      }
      if (curBre > maxBr) {
        curBre = maxBr;
        dir = -1;
      }
      setBright(curBre);
    }
  }

  uint8_t rpin, gpin, bpin;
  uint8_t r, g, b;
  uint8_t br;
  uint8_t maxBr = 255, minBr = 0;
  uint8_t brStep;
  int16_t curBre;
  int8_t dir = 1;
  Timer tmr;
};
