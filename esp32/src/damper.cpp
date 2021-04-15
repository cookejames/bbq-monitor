#include <damper.h>
#include <config.h>
#include <ArduinoLog.h>

namespace damper
{
#define CALCULATION_PERIOD 1000
  volatile uint16_t interruptCounter = 0; //counter use to detect hall sensor in fan
  uint64_t previousmills = 0;
  uint16_t lastRpm = 0;

  void ICACHE_RAM_ATTR handleInterrupt()
  {
    interruptCounter++;
  }

  void setup()
  {
    pinMode(FAN_RPM_PIN, INPUT);
    attachInterrupt(FAN_RPM_PIN, handleInterrupt, RISING);
  }

  uint16_t computeFanSpeed(uint16_t count, uint16_t elapsedTime)
  {
    //interruptCounter counts 2 pulses per revolution of the fan over a one second period
    return count / 2 * 60 * ((double)CALCULATION_PERIOD / (double)elapsedTime);
  }

  void check()
  {
    if ((millis() - previousmills) > CALCULATION_PERIOD)
    { // Process counters once every second
      lastRpm = computeFanSpeed(interruptCounter, millis() - previousmills);
      interruptCounter = 0;
      previousmills = millis();
    }
  }

  uint16_t getRPM()
  {
    return lastRpm;
  }
}