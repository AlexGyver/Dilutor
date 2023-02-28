#include <FastLED.h>
#include "timer.h"

#define RING_RAINBOW 0
#define RING_SPINNER 1
#define RING_FILLER 2
#define RING_BREATH 3

struct Ring {
  Ring(CRGB* nleds, int namount) {
    leds = nleds;
    amount = namount;
  }

  void start() {
    timer.start();
  }
  void stop() {
    timer.stop();
    FastLED.clear();
    FastLED.show();
  }

  void rainbow() {
    for (int i = 0; i < amount; i++) {
      CRGB col;
      col.setHue(counter + i * 255 / amount);
      leds[i] = fadeCount ? blend(leds[i], col, 40) : col;
    }
    counter += 3;
  }
  void spinner() {
    for (int p = 0; p < p1; p++) {
      int curCount = counter + amount * p / p1;
      leds[get(curCount)] = color;
      for (int i = 1; i < amount / p1; i++) {
        leds[get(curCount - i)] = leds[get(curCount - i + 1)].fadeToBlackBy(p2);
      }
    }
    if (++counter >= amount) counter = 0;
  }
  void filler() {
    FastLED.clear();
    if (p1 > amount) p1 = amount;
    for (int i = 0; i < p1; i++) {
      leds[get(counter + i)] = color;
    }
    counter++;
  }
  void breath() {
    bri += p3 * dir;
    if (bri < p1) {
      bri = p1;
      dir = 1;
    }
    if (bri > p2) {
      bri = p2;
      dir = -1;
    }
    CHSV col = rgb2hsv_approximate(color);
    col.value = bri;
    fill_solid(leds, amount, col);
  }

  void setEffect(byte neff, uint16_t speed, byte np1 = 0, byte np2 = 0, byte np3 = 0) {
    eff = neff;
    timer.setPrd(speed);
    p1 = np1;
    p2 = np2;
    p3 = np3;
    fadeCount = 25;
  }

  void tick() {
    if (timer.ready()) {
      switch (eff) {
        case 0: rainbow(); break;
        case 1: spinner(); break;
        case 2: filler(); break;
        case 3: breath(); break;
      }
      if (fadeCount) fadeCount--;
      FastLED.show();
    }
  }

  int get(int i) {
    return (i + amount) % amount;
  }

  int8_t dir = 1;
  int bri;
  byte counter = 0;
  byte p1, p2, p3;
  CRGB color;
  CRGB* leds;
  int amount;
  int8_t eff = 0;
  int8_t fadeCount;
  Timer timer;
};
