#include <ArduinoLog.h>
#include <controller.h>
#include <ibbq.h>
#include <awsiot.h>

void Controller::processTemperatureResult(uint16_t _temperatures[], uint8_t _numProbes)
{
  if (numProbes != _numProbes)
  {
    numProbes = _numProbes;
  }
  bool changed = false;
  for (int i = 0; i < _numProbes; i++)
  {
    // Check if the temperature has changed
    Log.verbose("Processed probe %d, current %d, new %d", i, temperatures[i], _temperatures[i]);
    if (temperatures[i] != _temperatures[i])
    {
      Log.trace("Temperature probe %d updated to %d", i, _temperatures[i]);
      changed = true;
      temperatures[i] = _temperatures[i];
    }

    // Update the temperature for the probe we are monitoring
    if (i == probe)
    {
      temperature = _temperatures[i];
    }
  }
  if (changed)
  {
    updateTemperatureShadow();
  }
}

void Controller::updateTemperatureShadow()
{
  const int capacity = JSON_OBJECT_SIZE(6);
  StaticJsonDocument<capacity> doc;
  JsonObject state = doc.createNestedObject("state");
  JsonObject reported = state.createNestedObject("reported");
  for (int i = 0; i < numProbes; i++)
  {
    if (temperatures[i] != IBBQ_NO_VALUE)
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

void Controller::processDesiredState(JsonObject desired)
{
  if (desired["value"] && (int16_t)desired["value"] != setpoint)
  {
    setpoint = (int16_t)desired["value"];
    Log.notice("Controller setpoint updated to %d", setpoint);
  }

  if (desired["sensor"] && (uint8_t)desired["sensor"] != probe)
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

  // Publish the current state
  updateSetpointShadow();
}

void Controller::updateSetpointShadow()
{
  const int capacity = JSON_OBJECT_SIZE(10);
  StaticJsonDocument<capacity> doc;
  JsonObject state = doc.createNestedObject("state");
  JsonObject reported = state.createNestedObject("reported");
  reported["value"] = setpoint;
  reported["sensor"] = probe;
  reported["fanSpeed"] = fanSpeed;
  reported["servoAngle"] = servoAngle;

  char output[128];
  serializeJson(doc, output);
  AwsIot::publishToShadow("setpoint", "update", output);
}