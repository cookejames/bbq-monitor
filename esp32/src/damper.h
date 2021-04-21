#ifndef damper_h
#define damper_h
#include <Arduino.h>

namespace damper
{
  void setup();
  void check();
  uint16_t getRPM();
  void updateFanDuty(uint8_t);
  void updateServoPercent(uint8_t);
  void updateServoPercent(uint8_t, bool);
}

#endif