#ifndef ibbq_h
#define ibbq_h
#include <Arduino.h>

using TemperatureResultCallback = void (*)(uint16_t[], uint8_t);

class iBBQ
{
public:
  static void connect(TemperatureResultCallback temperatureResultCallback);
  static void check();
  static bool isConnected();
};

#endif