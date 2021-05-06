#ifndef buttons_h
#define buttons_h
#include <Arduino.h>
#include <controller.h>

namespace buttons
{
  void setup(Controller *);
  void check();
}

#endif