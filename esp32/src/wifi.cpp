#include <secrets.h>
#include "WiFi.h"
#include <wifi.h>
#include <ArduinoLog.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>

bool Wifi::isConnected()
{
  return WiFi.status() == WL_CONNECTED;
}

bool Wifi::connect()
{
  startTime = millis();
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(THINGNAME);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Log.notice("Connecting to WiFi. SSID: %s", WIFI_SSID);

  while (WiFi.waitForConnectResult() != WL_CONNECTED && millis() < startTime + waitTime)
  {
    delay(500);
  }

  if (isConnected())
  {
    MDNS.begin(THINGNAME);
    enableOTA();
    Log.notice("WiFi Connected: %s - %s", WiFi.macAddress().c_str(), WiFi.localIP().toString().c_str());
    return true;
  }
  else
  {
    Log.error("WiFi connection failed");
    retryCount++;
    if (retryCount >= 5)
    {
      ESP.restart();
    }
    return false;
  }
}

bool Wifi::check()
{
  if (isConnected())
  {
    ArduinoOTA.handle();
    return true;
  }
  else if (millis() > startTime + waitTime)
  {
    Log.warning("WiFi disconnected, reconnecting!");
    startTime = millis();
    WiFi.disconnect();
    bool connected = Wifi::connect();
    if (connected)
    {
      Log.notice("WiFi reconnected.");
    }
    else
    {
      Log.warning("WiFi reconnect failed.");
    }
    return connected;
  }
  return isConnected();
}

void Wifi::enableOTA()
{
  ArduinoOTA
      .onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
          type = "sketch";
        else // U_SPIFFS
          type = "filesystem";

        Serial.println("Start updating " + type);
        Log.notice("OTA: Starting update %s", type);
      })
      .onEnd([]() {
        Log.notice("OTA: Update complete");
      })
      .onProgress([](unsigned int progress, unsigned int total) {
        Log.trace("OTA Progress: %F", (double)(progress / (total / 100)));
      })
      .onError([](ota_error_t error) {
        Log.error("OTA Error[%s]: ", error);
        if (error == OTA_AUTH_ERROR)
          Log.error("Auth Failed");
        else if (error == OTA_BEGIN_ERROR)
          Log.error("Begin Failed");
        else if (error == OTA_CONNECT_ERROR)
          Log.error("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR)
          Log.error("Receive Failed");
        else if (error == OTA_END_ERROR)
          Log.error("End Failed");
      });

  ArduinoOTA.begin();
}