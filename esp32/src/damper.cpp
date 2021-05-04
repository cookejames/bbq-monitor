#include <damper.h>
#include <config.h>
#include <ArduinoLog.h>
#include <ESP32Servo.h>
#include <movingAvg.h>
#include <awsiot.h>

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
  static uint8_t servoOpenAngle = SERVO_OPEN_ANGLE;
  static uint8_t servoClosedAngle = SERVO_CLOSED_ANGLE;
  static uint8_t fanMinPwm = FAN_MIN_PWM;
  static uint8_t fanMaxPwm = FAN_MAX_PWM;

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
    // Allow the fan to actually reach 0
    if (scale && duty > 0)
    {
      uint16_t range = fanMaxPwm - fanMinPwm;
      duty = fanMinPwm + (uint16_t)((double)duty / (double)100 * (double)range);
    }
    // If the fan was at 0 it might need a little kick to get up to speed
    if (fanDuty == 0 && duty > 0)
    {
      fan.write(255);
      delay(50);
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
    uint16_t range = servoOpenAngle - servoClosedAngle;
    // Open fully once we get close
    return servoClosedAngle + (uint16_t)((double)percent / (double)100 * (double)range);
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

  void updateDamperShadow()
  {
    Log.notice("Damper publishing update of its shadow");
    const int capacity = JSON_OBJECT_SIZE(20);
    StaticJsonDocument<capacity> doc;
    JsonObject state = doc.createNestedObject("state");
    JsonObject reported = state.createNestedObject("reported");
    reported["servoOpenAngle"] = servoOpenAngle;
    reported["servoClosedAngle"] = servoClosedAngle;
    reported["fanMinPwm"] = fanMinPwm;
    reported["fanMaxPwm"] = fanMaxPwm;

    char output[128];
    serializeJson(doc, output);
    AwsIot::publishToShadow("damper", "update", output);
  }

  void processDamperDesireState(JsonObject desired)
  {
    if (desired.containsKey("servoOpenAngle"))
    {
      servoOpenAngle = desired["servoOpenAngle"];
    }
    if (desired.containsKey("servoClosedAngle"))
    {
      servoClosedAngle = desired["servoClosedAngle"];
    }
    if (desired.containsKey("fanMinPwm"))
    {
      fanMinPwm = desired["fanMinPwm"];
    }
    if (desired.containsKey("fanMaxPwm"))
    {
      fanMaxPwm = desired["fanMaxPwm"];
    }

    Log.notice("Damper config updated to servoOpenAngle: %d, servoClosedAngle: %d, fanMinPwm: %d,  fanMaxPwm %d",
               servoOpenAngle, servoClosedAngle, fanMinPwm, fanMaxPwm);
    updateDamperShadow();
  }
}