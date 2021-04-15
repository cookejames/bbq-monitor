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
  void setup();
  void processTemperatureResult(uint16_t[], uint8_t);
  void processSetpointDesiredState(JsonObject);
  void processPidDesiredState(JsonObject);
  void setProbe(uint8_t);
  uint8_t getFanDuty();
  uint8_t getServoAngle();
  uint16_t getMonitoredTemperature();
  void run();

private:
  PID pid;
  uint8_t probe = 0;
  int16_t setpoint = 110;
  uint8_t servoAngle = SERVO_OPEN;
  uint8_t fanDuty = 100;
  uint8_t numProbes = 4;
  uint16_t temperature = 0;
  uint16_t temperatures[4] = {0, 0, 0, 0};
  double pidInput = 0;
  double pidOutput = 0;
  double pidSetpoint = 0;
  double Kp;
  double Ki;
  double Kd;
  bool isStartupMode();
  void updateTemperatureShadow();
  void updateTemperatureShadow(bool *);
  void updateSetpointShadow();
  void updatePidShadow();
  void scaleServoAngle();
  void updateDamper();
  bool isAutomaticControl();
};

#endif