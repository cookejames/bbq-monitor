#ifndef controller_h
#define controller_h
#include <Arduino.h>
#include <ArduinoJson.h>
#include <movingAvg.h>
#include <PID_v1.h>
#include <config.h>

struct ControlState
{
  int16_t setpoint;
  uint8_t sensor;
  uint8_t fanDuty;
  int16_t fanSpeed;
  uint8_t servoOpening;
  bool startupMode;
  bool lidOpenMode;
};

class Controller
{
public:
  Controller();
  void setup();
  void processTemperatureResult(uint16_t[], uint8_t);
  void processControlDesiredState(JsonObject);
  void processPidDesiredState(JsonObject);
  void setProbe(uint8_t);
  uint8_t getFanDuty();
  uint8_t getServoOpening();
  uint16_t getMonitoredTemperature();
  uint16_t getMonitoredTemperatureAverage();
  void run();

private:
  PID pid;
  movingAvg temperatureAverage;
  uint8_t probe = 0;
  int16_t setpoint = 100;
  uint8_t servoOpening = 100;
  uint8_t fanDuty = 100;
  uint8_t numProbes = 4;
  uint16_t temperature = 0;
  uint16_t temperatures[4] = {0, 0, 0, 0};
  uint32_t lastAverageReadingTime = 0;
  uint32_t lastSetpointPublishTime = 0;
  ControlState lastDeviceState = {0,0,0,0,0,false, false};
  bool lidOpenMode = false;
  uint32_t lidOpenModeStartTime = 0;
  // Don't enable lid open mode at startup
  uint32_t lidOpenModeNextEligibleStart = LID_OPEN_MODE_DURATION;
  double pidInput = 0;
  double pidOutput = 0;
  double pidSetpoint = 0;
  double Kp;
  double Ki;
  double Kd;
  bool isStartupMode();
  bool shouldLidOpenMode();
  void updateTemperatureShadow();
  void updateTemperatureShadow(bool *);
  void updateControlStateShadow();
  void updatePidShadow();
  void updateDamper();
  bool isAutomaticControl();
};

#endif