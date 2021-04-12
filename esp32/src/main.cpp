#include <Arduino.h>
#include <ArduinoLog.h>
#include <ibbq.h>
#include <wifi.h>
#include <mqtt.h>

void temperatureReceivedCallback(uint16_t temperatures[], uint8_t numProbes)
{
  std::string str = "";
  for (uint8_t i = 0; i < numProbes; i++)
  {
    Log.verbose("Temperature %d: %d", i, temperatures[i]);
  }
}

void setup()
{
  Serial.begin(115200);
  Log.begin(LOG_LEVEL_TRACE, &Serial, true);
  Log.setSuffix([](Print *_logOutput) { _logOutput->print('\n'); });

  // Connect WiFi
  wifiConnect();

  // Connect thermometer
  iBBQ::connect(temperatureReceivedCallback);

  connectAWS();
}

void loop()
{
  iBBQ::check();
  mqttCheck();
  delay(1000);
}