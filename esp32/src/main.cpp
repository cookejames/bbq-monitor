#include <Arduino.h>
#include <ArduinoLog.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "soc/rtc_cntl_reg.h"
#include <config.h>
#ifdef USE_IBBQ
#include <ibbq.h>
#endif
#include <wifi.h>
#include <awsiot.h>
#include <controller.h>
#include <damper.h>
#include <display.h>

#define STATUS_OK true
#define STATUS_BAD false
#define STATUS_PERIOD 5 * 60 * 1000 // 5 minutes

Wifi wifi;
Controller controller;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60 * 60 * 1000); // update every 60 minutes

long lastOkTime = 0;

void status(bool ok)
{
  #ifdef STATUS_PIN
  digitalWrite(STATUS_PIN, ok ? HIGH : LOW);
  #endif
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
    Log.trace("Received desired state from Aws IoT. Topic %s", topic.c_str());
    if (topic.indexOf("/controlstate/") >= 0)
    {
      controller.processControlDesiredState(doc["state"]["desired"]);
    }
    else if (topic.indexOf("/pid/") >= 0)
    {
      controller.processPidDesiredState(doc["state"]["desired"]);
    }
    else if (topic.indexOf("/damper/") >= 0)
    {
      damper::processDamperDesireState(doc["state"]["desired"]);
    }
    else
    {
      Log.warning("Received MQTT message on unknown topic: %s - %s", topic.c_str(), payload.c_str());
    }
  }
}

void setup()
{
  // WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // Prevent brownouts by silencing them. You probably want to keep this.

  Serial.begin(115200);
  Log.begin(LOG_LEVEL, &Serial, true);
  Log.setPrefix([](Print *_logOutput, int level) {   char c[12];sprintf(c, "%s - ", timeClient.getFormattedTime().c_str());_logOutput->print(c); });
  Log.setSuffix([](Print *_logOutput, int level) { _logOutput->print('\n'); });
  Display::init();

  //Setup the status pin
  #ifdef STATUS_PIN
  pinMode(STATUS_PIN, OUTPUT);
  #endif
  status(STATUS_BAD);

  controller.setup();

  // Connect WiFi
  wifi.connect();

  // Setup NTP time
  timeClient.begin();
  timeClient.update();

  #ifdef USE_IBBQ
  // Connect thermometer
  iBBQ::connect(temperatureReceivedCallback);
  #endif

  // Connect to MQTT
  AwsIot::setMessageHander(mqttMessageHandler);
  if (wifi.isConnected())
  {
    AwsIot::connect();
    AwsIot::publishToShadow("controlstate", "get", "");
    damper::updateDamperShadow();
    AwsIot::publishToShadow("damper", "get", "");
  }
  else
  {
    Log.warning("WiFi disconnected, skipping IoT start");
  }
}

long lastReport = 0;
void loop()
{
  // Set the status LED
  bool tempConnected;
  #ifdef USE_IBBQ
  tempConnected = iBBQ::isConnected();
  #else
  tempConnected = true;
  #endif
  Display::setStatus(wifi.isConnected(), AwsIot::isConnected(), tempConnected);

  if (wifi.isConnected() && tempConnected && AwsIot::isConnected())
  {
    status(STATUS_OK);
    lastOkTime = millis();
  }
  else
  {
    status(STATUS_BAD);
    if (millis() > lastOkTime + STATUS_PERIOD)
    {
      Log.fatal("Unable to connect for > %dms. Restarting.", STATUS_PERIOD);
      ESP.restart();
    }
  }

  timeClient.update();

  if (millis() > lastReport + 30000)
  {
    Log.trace("ESP free heap %d/%d, minimum free heap %d, max alloc heap %d",
              ESP.getFreeHeap(), ESP.getHeapSize(), ESP.getMinFreeHeap(), ESP.getMaxAllocHeap());
    Log.trace("Monitored temperature is %dC, rolling average is %dC, fan speed is %drpm, duty %dpc and servo opening is %dpc",
              controller.getMonitoredTemperature(), controller.getMonitoredTemperatureAverage(), damper::getRPM(), controller.getFanDuty(), controller.getServoOpening());
    lastReport = millis();
  }

  #ifdef USE_IBBQ
  iBBQ::check();
  #endif
  wifi.check();
  if (wifi.isConnected())
  {
    AwsIot::check();
  }
  controller.run();

  Display::check();
  delay(10);
}