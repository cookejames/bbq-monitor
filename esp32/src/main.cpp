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
  Log.setPrefix([](Print *_logOutput) {   char c[12];int m = sprintf(c, "%10lu ", millis());_logOutput->print(c); });
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

long lastMemoryReport = 0;
void loop()
{
  // Set the status LED
  status(wifi.isConnected() && iBBQ::isConnected() && AwsIot::isConnected());

  if (millis() > lastMemoryReport + 30000)
  {
    Log.trace("ESP free heap %d/%d, minimum free heap %d, max alloc heap %d", ESP.getFreeHeap(), ESP.getHeapSize(), ESP.getMinFreeHeap(), ESP.getMaxAllocHeap());
    lastMemoryReport = millis();
  }

  iBBQ::check();
  wifi.check();
  AwsIot::check();
  controller.run();
  delay(1000);
}