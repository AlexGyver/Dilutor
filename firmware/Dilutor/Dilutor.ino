// пины
#define POT_PIN A0
#define HX_SCK 2
#define HX_DT 3
#define VALVE_1 4
#define VALVE_2 5
#define R_PIN 9
#define G_PIN 10
#define B_PIN 11
#define ARGB_PIN 12
#define ARGB_NUM 16

// раскомментируй для отладки
#define DEBUG_EN

#ifdef DEBUG_EN
#define DEBUG(x) Serial.print(x)
#define DEBUGLN(x) Serial.println(x)
#else
#define DEBUG(x)
#define DEBUGLN(x)
#endif

// либы
#include <FastLED.h>
#include "ring.h"
#include "EEManager.h"
#include "GyverHX711.h"
#include "hyster.h"
#include "valve.h"
#include "rgb.h"
#include "filter.h"
#include "timer.h"
#include "waiter.h"

// данные
struct Data {
  bool calibr = 0;
  int32_t glass = 0;
  int32_t liquid = 0;
};

Data data;
uint8_t mode = 0;
uint16_t curGlass, curPot;

Timer startup(TMR_TIMER, 2000);
Timer debTmr(TMR_PERIOD, 100);
GyverHX711 hx(HX_DT, HX_SCK, HX_GAIN64_A);
EEManager mem(data);
Waiter waiter(1500);
RGBled led(R_PIN, G_PIN, B_PIN);
Valve v1(VALVE_1);
Valve v2(VALVE_2);
Hyster pot(POT_PIN);
Filter fil;

CRGB leds[ARGB_NUM];
Ring ring(leds, ARGB_NUM);

void setup() {
#ifdef DEBUG_EN
  Serial.begin(115200);
  debTmr.start();
#endif

  mem.begin(0, 'b');
  startup.start();
  led.breathMax(150);
  led.breathMin(5);
  led.setBright(80);
  led.set(255, 255, 0); // жёлтый

  FastLED.addLeds<WS2812, ARGB_PIN, GRB>(leds, ARGB_NUM);
  FastLED.setBrightness(100);
  ring.stop();
}

void loop() {
  ring.tick();  // эффекты кольца
  led.tick();   // эффекты светодиода
  fil.tick();   // фильтр

  // даём весам разогреться перед запуском основной программы
  if (startup.ready()) {
    led.breath(5);            // медленно мигать
    //hx.tare();              // калибровать 0 у весов
    hx.setOffset(-fil.get()); // калибровать 0 у весов (по отфильтрованному)
    ring.start();
    ring.setEffect(RING_RAINBOW, 30);
  }

  // читаем вес и отправляем в фильтр
  if (hx.available()) fil.set(hx.read());

  // таймер отладки
  if (debTmr.ready()) {
    DEBUG(fil.get());
    DEBUG(',');
    DEBUGLN(analogRead(POT_PIN));
  }

  // основная стейт машина программы
  // если таймер запуска отработал и отключился
  if (!startup.running()) {
    switch (mode) {

      case 0:   // idle
        if (pot.tick()) {           // было вращение туда-сюда
          mode = 1;                 // идём калиброваться
          hx.tare();                // на всякий случай перекалибруемся
          led.breath(15);           // быстрый
          led.set(255, 0, 0);       // красный
          ring.setEffect(RING_SPINNER, 70, 2, 110);
          ring.color = CRGB::Red;
          DEBUGLN("start calibr");
        }

        if (data.calibr) {    // вес коктейля откалиброван
          // waiter ждёт когда вес будет больше чем 2/3 стакана в течение неск секунд
          if (waiter.wait((fil.get() > data.glass * 2 / 3))) {
            curGlass = fil.get();         // текущий вес стакана
            curPot = analogRead(POT_PIN); // положение крутилки
            v1.on();                // вкл клапан 1
            led.breath(15);         // быстрый
            led.set(0, 255, 255);   // бирюзовый
            mode = 4;               // идём ждать наливание
            ring.setEffect(RING_FILLER, 100);
            ring.color = CRGB::Cyan;
            DEBUGLN("start pour");
          }
        }
        break;

      case 1:   // calibr step 1
        if (pot.tick()) {           // было вращение туда-сюда
          mode = 2;                 // идём измерять вес жидкости
          data.glass = fil.get();   // запоминаем вес стакана
          led.set(0, 255, 255);     // (быстрый) бирюзовый
          ring.color = CRGB::Cyan;
          DEBUGLN("calibr glass end");
        }
        break;

      case 2:   // calibr step 2
        if (pot.tick()) {           // было вращение туда-сюда
          mode = 3;                 // идём ждать когда уберут стакан
          data.calibr = 1;          // жидкость откалибрована, запомнили
          data.liquid = fil.get() - data.glass;   // вес жидкости
          mem.updateNow();          // сохранили себе
          led.set(0, 255, 0);       // (быстрый) зелёный
          ring.setEffect(RING_BREATH, 30, 20, 255, 15);
          ring.color = CRGB::Green;
          DEBUGLN("calibr liquid end");
        }
        break;

      case 3:   // calibr end
        // waiter ждёт когда вес будет меньше чем 2/3 стакана в течение неск секунд
        if (waiter.wait(fil.get() < data.glass * 2 / 3)) {
          mode = 0;             // идём в режим ожидания
          led.breath(5);        // медленный
          led.set(255, 255, 0); // жёлтый
          ring.setEffect(RING_RAINBOW, 30);
          hx.tare();
          DEBUGLN("calibr end");
        }
        break;

      case 4:   // wait liquid 1
        // ждём когда вес будет больше чем вес первой половины коктейля
        if (fil.get() - curGlass > data.liquid * curPot / 1023) {
          mode = 5;
          v1.off();   // клапан 1 выкл
          v2.on();    // клапан 2 вкл
          DEBUGLN("pour step 1 end");
        }

        ring.p1 = (fil.get() - curGlass) * 16L / data.liquid;
        ring.p1 = max(ring.p1, 1);

        // если стакан убрали - выходим
        if (fil.get() < data.glass / 2) {
          mode = 0;
          v1.off();
          led.breath(5);          // медленный
          led.set(255, 255, 0);   // жёлтый
          ring.setEffect(RING_RAINBOW, 30);
          DEBUGLN("glass error");
        }
        break;

      case 5:   // wait liquid 2
        // ждём когда вес будет больше вес коктейля
        if (fil.get() - curGlass > data.liquid) {
          mode = 6;     // идём ждать когда уберут стакан
          v2.off();     // клапан 2 выкл
          led.set(0, 255, 0);     // (быстрый) зелёный
          ring.setEffect(RING_BREATH, 30, 20, 255, 15);
          ring.color = CRGB::Green;
          DEBUGLN("pour step 2 end");
        }

        ring.p1 = (fil.get() - curGlass) * 16L / data.liquid;
        ring.p1 = max(ring.p1, 1);

        // если стакан убрали - выходим
        if (fil.get() < data.glass / 2) {
          mode = 0;
          v2.off();
          led.breath(5);          // медленный
          led.set(255, 255, 0);   // жёлтый
          ring.setEffect(RING_RAINBOW, 30);
          DEBUGLN("glass error");
        }
        break;

      case 6:   // pour end
        // waiter ждёт когда вес будет меньше чем 2/3 стакана в течение неск секунд
        if (waiter.wait(fil.get() < data.glass * 2 / 3)) {
          mode = 0;             // идём в режим ожидания
          hx.tare();            // калибруем 0
          led.breath(5);        // медленный
          led.set(255, 255, 0); // жёлтый
          ring.setEffect(RING_RAINBOW, 30);
          DEBUGLN("pour end");
        }
        break;
    }
  }
}
