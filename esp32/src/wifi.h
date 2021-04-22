#ifndef wifi_h
#define wifi_h
#include <Arduino.h>

class Wifi
{
public:
  bool isConnected();
  bool connect();
  bool check();
  static void reset();

private:
  unsigned long startTime;
  const unsigned long waitTime = 60 * 1000;
  uint8_t retryCount = 0;
  void enableOTA();
};

#endif
