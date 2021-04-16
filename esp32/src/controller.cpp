#include <ArduinoLog.h>
#include <controller.h>
#include <ibbq.h>
#include <awsiot.h>
#include <damper.h>

#define SETPOINT_MANUAL_OVERRIDE -1
#define TIME_BETWEEN_AVERAGE_READINGS 10000
#define TIME_BETWEEN_SETPOINT_PUBLISH 5 * 60 * 100

Controller::Controller() : pid(&pidInput, &pidOutput, &pidSetpoint, Kp = 10, Ki = 0.15, Kd = 15, P_ON_E, DIRECT),
                           temperatureAverage(30)
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

  temperatureAverage.begin();
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

  bool shouldLidOpen = shouldLidOpenMode();
  if (shouldLidOpen && lidOpenMode)
  {
    // Lid open mode should be and is enabled
    return;
  }
  else if (shouldLidOpen && !lidOpenMode)
  {
    // Lid open mode should be enabled but isn't
    Log.notice("Enabling lid open mode until %d. Temperature %dC is more than %Fpc below the temperature average of %F.",
               millis() + LID_OPEN_MODE_DURATION, temperature, ((1 - LID_OPEN_MODE_THRESHOLD) * 100), temperatureAverage.getAvg());
    lidOpenModeStartTime = millis();
    lidOpenMode = true;
    pid.SetMode(MANUAL);
    fanDuty = 0;
    servoOpening = servoOpening < 50 ? servoOpening : 50;
    updateControlStateShadow();
  }
  else if (!shouldLidOpen && lidOpenMode)
  {
    // Lid open mode should be disabled but is enabled
    lidOpenMode = false;
    lidOpenModeNextEligibleStart = millis() + LID_OPEN_MODE_DURATION;
    Log.notice("Lid open mode duration has now passed - disabling until %d", lidOpenModeNextEligibleStart);
    pid.SetMode(AUTOMATIC);
    updateControlStateShadow();
  }
  // During startup we want to disable PID control, this allows us to
  // tune PID just for stabilising the temperature without worrying
  // about the temperature when we are trying to start the BBQ
  else if (isStartupMode())
  {
    if (pid.GetMode() == AUTOMATIC)
    {
      Log.notice("Startup mode enabled, controller PID set to manual");
      pid.SetMode(MANUAL);
    }
    if (fanDuty != 100)
    {
      fanDuty = 100;
      servoOpening = 100;
      updateControlStateShadow();
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
    servoOpening = fanDuty > 90 ? 100 : fanDuty;

    if ((int)oldOutput != (int)pidOutput)
    {
      Log.notice("Temperature is %dC, setpoint is %dC, controller output changed to: Fan duty %dpc, Fan speed %drpm, Servo opening %dpc",
                 temperature, setpoint, fanDuty, damper::getRPM(), servoOpening);
      updateControlStateShadow();
    }
  }

  updateDamper();
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
      if (temperatureAverage.getCount() == 0 || millis() > lastAverageReadingTime + TIME_BETWEEN_AVERAGE_READINGS)
      {
        temperatureAverage.reading(temperature);
        lastAverageReadingTime = millis();
      }
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

void Controller::processControlDesiredState(JsonObject desired)
{
  if (desired.containsKey("setpoint") && (int16_t)desired["setpoint"] != setpoint)
  {
    setpoint = (int16_t)desired["setpoint"];
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
    if (desired.containsKey("servoOpening") && (uint8_t)desired["servoOpening"] != servoOpening)
    {
      servoOpening = (uint8_t)desired["servoOpening"];
      Log.notice("Controller servoOpening manually set to %d", servoOpening);
    }
    updateDamper();
  }

  // Publish the current state
  updateControlStateShadow();
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

void Controller::updateControlStateShadow()
{
  Log.notice("Controller publishing update of setpoint shadow");
  const int capacity = JSON_OBJECT_SIZE(20);
  StaticJsonDocument<capacity> doc;
  JsonObject state = doc.createNestedObject("state");
  JsonObject reported = state.createNestedObject("reported");

  // Keep track of the last reported values so that we only send results that have changed.
  if (lastDeviceState.setpoint != setpoint || millis() > lastSetpointPublishTime + TIME_BETWEEN_SETPOINT_PUBLISH)
  {
    reported["setpoint"] = setpoint;
    lastDeviceState.setpoint = setpoint;
    // Periodically publish the setpoint for graphing purposes
    lastSetpointPublishTime = millis();
  }
  if (lastDeviceState.sensor != probe)
  {
    reported["sensor"] = probe;
    lastDeviceState.sensor = probe;
  }
  if (lastDeviceState.fanDuty != fanDuty)
  {
    reported["fanDuty"] = fanDuty;
    lastDeviceState.fanDuty = fanDuty;
  }
  uint16_t rpm = damper::getRPM();
  if (lastDeviceState.fanSpeed != rpm)
  {
    reported["fanSpeed"] = rpm;
    lastDeviceState.fanSpeed = rpm;
  }
  if (lastDeviceState.servoOpening != servoOpening)
  {
    reported["servoOpening"] = servoOpening;
    lastDeviceState.servoOpening = servoOpening;
  }
  bool startupMode = isStartupMode();
  if (lastDeviceState.startupMode != startupMode)
  {
    reported["startupMode"] = startupMode;
    lastDeviceState.startupMode = startupMode;
  }

  if (lastDeviceState.lidOpenMode != lidOpenMode)
  {
    reported["lidOpenMode"] = lidOpenMode;
    lastDeviceState.lidOpenMode = lidOpenMode;
  }

  char output[128];
  serializeJson(doc, output);
  AwsIot::publishToShadow("controlstate", "update", output);
}

void Controller::updateDamper()
{
  damper::updateFanDuty(fanDuty);
  damper::updateServoPercent(servoOpening);
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
  return STARTUP_MODE_ENABLED && !lidOpenMode && temperature < STARTUP_MODE_PERCENTAGE * (double)setpoint;
}

bool Controller::shouldLidOpenMode()
{
  // We need readings
  if (temperatureAverage.getCount() == 0)
  {
    return false;
  }

  uint32_t currentTime = millis();

  // Too soon to enable lid open mode again
  if (currentTime < lidOpenModeNextEligibleStart)
  {
    return false;
  }

  // If we are in the mode and less than the duration return true
  if (currentTime < lidOpenModeStartTime + LID_OPEN_MODE_DURATION)
  {
    return true;
  }

  // If we have passed the lid open mode duration threshold reset.
  if (lidOpenMode && currentTime > lidOpenModeStartTime + LID_OPEN_MODE_DURATION)
  {
    return false;
  }

  // Temperature has dropped, lid open mode should be enabled
  if (temperature < LID_OPEN_MODE_THRESHOLD * temperatureAverage.getAvg())
  {
    return true;
  }

  return false;
}

uint8_t Controller::getFanDuty()
{
  return fanDuty;
}

uint8_t Controller::getServoOpening()
{
  return servoOpening;
}

uint16_t Controller::getMonitoredTemperature()
{
  return temperature;
}

uint16_t Controller::getMonitoredTemperatureAverage()
{
  if (temperatureAverage.getCount() == 0)
  {
    return 0;
  }
  return temperatureAverage.getAvg();
}