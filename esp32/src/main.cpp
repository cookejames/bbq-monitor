#include <Arduino.h>
#include <ArduinoLog.h>
#include <ArduinoJson.h>
#include <ibbq.h>
#include <wifi.h>
#include <awsiot.h>
#include <controller.h>
#include <config.h>

Wifi wifi;
Controller controller;

void status(bool ok)
{
  digitalWrite(STATUS_PIN, ok ? HIGH : LOW);
}

void temperatureReceivedCallback(uint16_t temperatures[], uint8_t numProbes)
{
  controller.processTemperatureResult(temperatures, numProbes);
}

void mqttMessageHandler(String &topic, String &payload)
{
  Log.verbose("MQTT message: %s - %s", topic.c_str(), payload.c_str());
  StaticJsonDocument<1000> doc;
  DeserializationError error = deserializeJson(doc, payload);
  if (error)
  {
    Log.error("Error deserialising MQTT: %s", error.c_str());
    return;
  }
  if (doc["state"]["desired"])
  {
    Log.trace("Received desired state from Aws IoT");
    controller.processDesiredState(doc["state"]["desired"]);
  }
}

void setup()
{
  Serial.begin(115200);
  Log.begin(LOG_LEVEL, &Serial, true);
  Log.setSuffix([](Print *_logOutput) { _logOutput->print('\n'); });

  //Setup the status pin
  pinMode(STATUS_PIN, OUTPUT);
  status(false);

  // Connect WiFi
  wifi.connect();

  // Connect thermometer
  iBBQ::connect(temperatureReceivedCallback);

  // Connect to MQTT
  if (wifi.isConnected())
  {
    AwsIot::setMessageHander(mqttMessageHandler);
    AwsIot::connect();
    AwsIot::publishToShadow("setpoint", "get", "");
  }
  else
  {
    Log.warning("WiFi disconnected, skipping IoT start");
  }
}

void loop()
{
  // Set the status LED
  status(wifi.isConnected() && iBBQ::isConnected() && AwsIot::isConnected());

  iBBQ::check();
  wifi.check();
  AwsIot::check();
  delay(1000);
}