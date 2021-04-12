#ifndef wifi_h
#define wifi_h
#include <Arduino.h>

class Wifi
{
public:
  bool isConnected();
  bool connect();
  bool check();

private:
  unsigned long startTime;
  const unsigned long waitTime = 60 * 1000;
};

#endif
