#include <Arduino.h>
#include <ArduinoLog.h>
#include <ibbq.h>
#include <wifi.h>
#include <awsiot.h>

#define STATUS_PIN 2

Wifi wifi;

void status(bool ok)
{
  digitalWrite(STATUS_PIN, ok ? HIGH : LOW);
}

void temperatureReceivedCallback(uint16_t temperatures[], uint8_t numProbes)
{
  for (uint8_t i = 0; i < numProbes; i++)
  {
    Log.verbose("Temperature %d: %d", i, temperatures[i]);
  }
  // Log.trace("MQTT publish");
  // StaticJsonDocument<200> doc;
  // doc["time"] = millis();
  // doc["sensor_a0"] = millis();
  // char jsonBuffer[512];
  // serializeJson(doc, jsonBuffer); // print to client
}

void mqttMessageHandler(String &topic, String &payload)
{
  Log.trace("incoming: %s - %s", topic.c_str(), payload.c_str());

  // StaticJsonDocument<200> doc;
  // deserializeJson(doc, payload);
  // const char *message = doc["message"];
  // Log.notice("%s", message);
}

void setup()
{
  Serial.begin(115200);
  Log.begin(LOG_LEVEL_TRACE, &Serial, true);
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