#include <ArduinoLog.h>
#include <controller.h>
#include <ibbq.h>
#include <awsiot.h>
#include <damper.h>

#define SETPOINT_MANUAL_OVERRIDE -1

Controller::Controller() : pid(&pidInput, &pidOutput, &pidSetpoint, Kp = 3, Ki = 0.005, Kd = 6, P_ON_M, REVERSE)
{
}

void Controller::setup()
{
  pid.SetOutputLimits(0, 100);
  pid.SetSampleTime(200);
  if (isAutomaticControl())
  {
    pid.SetMode(AUTOMATIC);
  }
  else
  {
    pid.SetMode(MANUAL);
  }

  damper::setup();
  updateDamper();
}

bool Controller::isAutomaticControl()
{
  return setpoint != SETPOINT_MANUAL_OVERRIDE;
}

void Controller::run()
{
  damper::check();

  if (!isAutomaticControl())
  {

    if (pid.GetMode() == AUTOMATIC)
    {
      Log.notice("Changing PID mode to manual");
      pid.SetMode(MANUAL);
    }
    return;
  }

  // During startup we want to disable PID control, this allows us to
  // tune PID just for stabilising the temperature without worrying
  // about the temperature when we are trying to start the BBQ
  if (isStartupMode())
  {
    if (pid.GetMode() == AUTOMATIC)
    {
      Log.notice("Startup mode enabled, controller PID set to manual");
      pid.SetMode(MANUAL);
    }
    if (fanDuty != 100)
    {
      fanDuty = 100;
      updateSetpointShadow();
    }
  }
  else
  {

    if (pid.GetMode() == MANUAL)
    {
      Log.notice("Changing PID mode to automatic");
      pid.SetMode(AUTOMATIC);
      updatePidShadow();
    }

    // Capture the before value
    double oldOutput = pidOutput;

    // Update variables
    pidSetpoint = setpoint;
    pidInput = temperature;
    pid.Compute();

    // Update outputs
    fanDuty = pidOutput;

    if ((int)oldOutput != (int)pidOutput)
    {
      Log.notice("Temperature is %dC, setpoint is %dC, controller output changed to: Fan duty %dpc, Fan speed %drpm, Servo angle %ddeg", temperature, setpoint, fanDuty, damper::getRPM(), servoAngle);
      updateSetpointShadow();
    }
  }

  // Update fan and servo themselves. Scale the servo angle based on the fan speed.
  scaleServoAngle();
  updateDamper();
}

void Controller::scaleServoAngle()
{
  uint16_t range = SERVO_OPEN - SERVO_CLOSED;
  // Open fully once we get close
  servoAngle = fanDuty > 90 ? SERVO_OPEN : (double)fanDuty / (double)100 * (double)range;
}

void Controller::processTemperatureResult(uint16_t _temperatures[], uint8_t _numProbes)
{
  if (numProbes != _numProbes)
  {
    numProbes = _numProbes;
  }
  bool changed = false;
  bool probesToUpdate[4] = {false, false, false, false};
  for (int i = 0; i < _numProbes; i++)
  {
    // Check if the temperature has changed
    Log.verbose("Processed probe %d, current %d, new %d", i, temperatures[i], _temperatures[i]);
    if (temperatures[i] != _temperatures[i])
    {
      Log.trace("Temperature probe %d updated to %d", i, _temperatures[i]);
      changed = true;
      temperatures[i] = _temperatures[i];
      probesToUpdate[i] = true;
    }

    // Update the temperature for the probe we are monitoring
    if (i == probe)
    {
      temperature = _temperatures[i];
    }
  }
  if (changed)
  {
    updateTemperatureShadow(probesToUpdate);
  }
}

void Controller::updateTemperatureShadow()
{
  bool probes[4] = {true, true, true, true};
  return updateTemperatureShadow(probes);
}

void Controller::updateTemperatureShadow(bool probesToUpdate[])
{
  const int capacity = JSON_OBJECT_SIZE(20);
  StaticJsonDocument<capacity> doc;
  JsonObject state = doc.createNestedObject("state");
  JsonObject reported = state.createNestedObject("reported");
  for (int i = 0; i < numProbes; i++)
  {
    if (temperatures[i] != IBBQ_NO_VALUE && probesToUpdate[i])
    {
      char buffer[10];
      sprintf(buffer, "%d", i);
      reported[buffer] = temperatures[i];
    }
  }
  char output[128];
  serializeJson(doc, output);
  AwsIot::publishToShadow("temperature", "update", output);
}

void Controller::setProbe(uint8_t number)
{
  if (number > numProbes - 1)
  {
    return;
  }
  probe = number;
}

void Controller::processSetpointDesiredState(JsonObject desired)
{
  if (desired.containsKey("value") && (int16_t)desired["value"] != setpoint)
  {
    setpoint = (int16_t)desired["value"];
    Log.notice("Controller setpoint updated to %d", setpoint);
  }

  if (desired.containsKey("sensor") && (uint8_t)desired["sensor"] != probe)
  {
    if ((uint8_t)desired["sensor"] > numProbes - 1)
    {
      Log.warning("Received invalid request to set probe to %d. Ignoring.", (uint8_t)desired["sensor"]);
    }
    else
    {
      probe = (uint8_t)desired["sensor"];
      Log.notice("Controller probe updated to %d", probe);
    }
  }

  if (!isAutomaticControl())
  {
    if (desired.containsKey("fanDuty") && (uint16_t)desired["fanDuty"] != fanDuty)
    {
      fanDuty = (uint16_t)desired["fanDuty"];
      Log.notice("Controller fanDuty manually set to %d", fanDuty);
    }
    if (desired.containsKey("servoAngle") && (uint8_t)desired["servoAngle"] != servoAngle)
    {
      servoAngle = (uint8_t)desired["servoAngle"];
      Log.notice("Controller servoAngle manually set to %d", servoAngle);
    }
    updateDamper();
  }

  // Publish the current state
  updateSetpointShadow();
}

void Controller::processPidDesiredState(JsonObject desired)
{
  if (desired.containsKey("Kp") && (double)desired["Kp"] != pid.GetKp())
  {
    pid.SetTunings((double)desired["Kp"], pid.GetKi(), pid.GetKd());
    Log.notice("Controller pid Kp updated to %F", pid.GetKp());
  }

  if (desired.containsKey("Ki") && (double)desired["Ki"] != pid.GetKi())
  {
    pid.SetTunings(pid.GetKp(), (double)desired["Ki"], pid.GetKd());
    Log.notice("Controller pid Ki updated to %F", pid.GetKi());
  }

  if (desired.containsKey("Kd") && (double)desired["Kd"] != pid.GetKd())
  {
    pid.SetTunings(pid.GetKp(), pid.GetKi(), (double)desired["Kd"]);
    Log.notice("Controller pid Kd updated to %F", pid.GetKd());
  }

  // Publish the current state
  updatePidShadow();
}

void Controller::updateSetpointShadow()
{
  Log.notice("Controller publishing update of setpoint shadow");
  const int capacity = JSON_OBJECT_SIZE(20);
  StaticJsonDocument<capacity> doc;
  JsonObject state = doc.createNestedObject("state");
  JsonObject reported = state.createNestedObject("reported");
  reported["value"] = setpoint;
  reported["sensor"] = probe;
  reported["fanDuty"] = fanDuty;
  reported["fanSpeed"] = damper::getRPM();
  reported["servoAngle"] = servoAngle;
  reported["startupMode"] = isStartupMode();

  char output[128];
  serializeJson(doc, output);
  AwsIot::publishToShadow("setpoint", "update", output);
}

void Controller::updateDamper()
{
  damper::updateFanDuty(fanDuty);
  damper::updateServoAngle(servoAngle);
}

void Controller::updatePidShadow()
{
  Log.notice("Controller publishing update of the pid shadow");
  const int capacity = JSON_OBJECT_SIZE(20);
  StaticJsonDocument<capacity> doc;
  JsonObject state = doc.createNestedObject("state");
  JsonObject reported = state.createNestedObject("reported");
  reported["Kp"] = pid.GetKp();
  reported["Ki"] = pid.GetKi();
  reported["Kd"] = pid.GetKd();

  char output[128];
  serializeJson(doc, output);
  AwsIot::publishToShadow("pid", "update", output);
}

bool Controller::isStartupMode()
{
  // TODO CHANGE DIRECTION
  return STARTUP_MODE_ENABLED && temperature > STARTUP_MODE_PERCENTAGE * (double)setpoint;
}

uint8_t Controller::getFanDuty()
{
  return fanDuty;
}

uint8_t Controller::getServoAngle()
{
  return servoAngle;
}

uint16_t Controller::getMonitoredTemperature()
{
  return temperature;
}