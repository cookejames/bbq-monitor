#ifndef controller_h
#define controller_h
#include <Arduino.h>

class Controller
{
public:
  // Controller();
  void processTemperatureResult(uint16_t[], uint8_t);

private:
  uint8_t probe = 0;
  uint16_t setpoint = 0;
  uint16_t temperature = 0;
  uint16_t temperatures[4] = {0, 0, 0, 0};
  uint8_t numProbes = 4;
  void publishTemperatures();
};

#endif