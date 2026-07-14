#ifndef CODEXLIGHT_LED_H
#define CODEXLIGHT_LED_H

#include <Arduino.h>

class LedController {
 public:
  void begin();

  void showRed();
  void showGreen();
  void showYellow();

  void redOn();
  void redOff();

  void greenOn();
  void greenOff();

  void yellowOn();
  void yellowOff();

  void allOn();
  void allOff();

  void setBrightness(uint8_t brightness);
};

#endif  // CODEXLIGHT_LED_H
