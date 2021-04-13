#ifndef controller_h
#define controller_h
#include <Arduino.h>
#include <ArduinoJson.h>
#include <PID_v1.h>
#include <config.h>

class Controller
{
public:
  Controller();
  void processTemperatureResult(uint16_t[], uint8_t);
  void processDesiredState(JsonObject);
  void setProbe(uint8_t);
  void run();

private:
  uint8_t probe = 0;
  int16_t setpoint = 0;
  uint8_t servoAngle = SERVO_OPEN;
  uint8_t fanSpeed = 100;
  uint8_t numProbes = 4;
  uint16_t temperature = 0;
  uint16_t temperatures[4] = {0, 0, 0, 0};
  PID pid;
  double pidInput = 0;
  double pidOutput = 0;
  double pidSetpoint = 0;
  double Kp;
  double Ki;
  double Kd;
  void updateTemperatureShadow();
  void updateSetpointShadow();
  void updateFanSpeed();
  void updateServoAngle();
};

#endif