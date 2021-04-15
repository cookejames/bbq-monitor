#include <damper.h>
#include <config.h>
#include <ArduinoLog.h>
#include <ESP32Servo.h>

namespace damper
{
#define FAN_FREQUENCY 25000
#define FAN_CHANNEL 0
#define FAN_RESOLUTION 8

#define RPM_CALCULATION_PERIOD 1000

  static Servo servo;
  static ESP32PWM fan;

  static volatile uint16_t interruptCounter = 0; //counter use to detect hall sensor in fan
  static uint64_t previousmills = 0;
  static uint16_t lastRpm = 0;

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

  void updateFanDuty(uint8_t fanDuty)
  {
    double duty = ((double)fanDuty / (double)100) * (double)255;
    duty = duty > 255 ? 255 : duty;
    fan.write(duty);
  }

  void updateServoAngle(uint8_t servoAngle)
  {
    servo.write(servoAngle);
  }
}