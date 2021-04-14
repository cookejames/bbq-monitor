#ifndef controller_h
#define controller_h
#include <Arduino.h>
#include <ArduinoJson.h>
#include <PID_v1.h>
#include <config.h>
#include <ESP32Servo.h>

class Controller
{
public:
  Controller();
  void processTemperatureResult(uint16_t[], uint8_t);
  void processSetpointDesiredState(JsonObject);
  void processPidDesiredState(JsonObject);
  void setProbe(uint8_t);
  void run();

private:
  uint8_t probe = 0;
  int16_t setpoint = 110;
  uint8_t servoAngle = SERVO_OPEN;
  uint8_t fanSpeed = 0;
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
  Servo servo;
  ESP32PWM fan;
  void updateTemperatureShadow();
  void updateTemperatureShadow(bool *);
  void updateSetpointShadow();
  void updatePidShadow();
  void scaleServoAngle();
  void updateFanSpeed();
  void updateServoAngle();
  bool isAutomaticControl();
};

#endif