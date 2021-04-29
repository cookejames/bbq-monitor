#include <damper.h>
#include <config.h>
#include <ArduinoLog.h>
#include <ESP32Servo.h>
#include <movingAvg.h>

namespace damper
{
#define FAN_FREQUENCY 25000
#define FAN_CHANNEL 0
#define FAN_RESOLUTION 8

#define RPM_CALCULATION_PERIOD 1000

  static Servo servo;
  static ESP32PWM fan;
  static movingAvg servoAverage(10);

  static volatile uint16_t interruptCounter = 0; //counter use to detect hall sensor in fan
  static uint64_t previousmills = 0;
  static uint16_t lastRpm = 0;
  static uint8_t fanDuty = 0;
  static uint8_t servoOpening = 0;

  void ICACHE_RAM_ATTR handleInterrupt()
  {
    interruptCounter++;
  }

  void setup()
  {
    servo.attach(SERVO_PIN, SERVO_MIN_PULSE_WIDTH, SERVO_MAX_PULSE_WIDTH);
    fan.attachPin(FAN_PWM_PIN, FAN_FREQUENCY, FAN_RESOLUTION);
    pinMode(FAN_RPM_PIN, INPUT);
    attachInterrupt(FAN_RPM_PIN, handleInterrupt, RISING);
    servoAverage.begin();
  }

  uint16_t computeFanSpeed(uint16_t count, uint16_t elapsedTime)
  {
    //interruptCounter counts 2 pulses per revolution of the fan over a one second period
    return count / 2 * 60 * ((double)RPM_CALCULATION_PERIOD / (double)elapsedTime);
  }

  void check()
  {
    if ((millis() - previousmills) > RPM_CALCULATION_PERIOD)
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

  void updateFanDuty(uint8_t duty, bool scale)
  {
    if (scale)
    {
      uint16_t range = FAN_MAX_PWM - FAN_MIN_PWM;
      duty = FAN_MIN_PWM + (uint16_t)((double)duty / (double)100 * (double)range);
    }
    fanDuty = duty;
    double scaledDuty = ((double)duty / (double)100) * (double)255;
    scaledDuty = scaledDuty > 255 ? 255 : scaledDuty;
    fan.write(scaledDuty);
  }

  void updateFanDuty(uint8_t fanDuty)
  {
    return updateFanDuty(fanDuty, true);
  }

  uint16_t servoPercentToAngle(uint8_t percent)
  {
    uint16_t range = SERVO_OPEN - SERVO_CLOSED;
    // Open fully once we get close
    return SERVO_CLOSED + (uint16_t)((double)percent / (double)100 * (double)range);
  }

  void updateServoPercent(uint8_t percent, bool averaged)
  {
    servoAverage.reading(percent);
    uint16_t opening = averaged ? servoAverage.getAvg() : percent;
    uint16_t angle = servoPercentToAngle(opening);
    servo.write(angle);
    servoOpening = opening;
  }

  void updateServoPercent(uint8_t percent)
  {
    updateServoPercent(percent, true);
  }

  uint8_t getFanDuty()
  {
    return fanDuty;
  }

  uint8_t getServoPercent()
  {
    return servoOpening;
  }
}