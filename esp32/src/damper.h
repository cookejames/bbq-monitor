#ifndef damper_h
#define damper_h
#include <Arduino.h>

namespace damper
{
  void setup();
  void check();
  uint16_t getRPM();
}

#endif