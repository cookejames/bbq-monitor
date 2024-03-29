#include <secrets.h>
#include "WiFi.h"
#include <wifi.h>
#include <ArduinoLog.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <display.h>

bool Wifi::isConnected()
{
  return WiFi.status() == WL_CONNECTED;
}

bool Wifi::connect()
{
  startTime = millis();
  WiFi.mode(WIFI_OFF);
  delay(100);
  WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
    Display::setIpAddress(WiFi.localIP().toString().c_str());
  },
               WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(THINGNAME);
  WiFi.disconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Log.notice("Connecting to WiFi. SSID: %s", WIFI_SSID);

  while (WiFi.waitForConnectResult() != WL_CONNECTED && millis() < startTime + waitTime)
  {
    delay(1000);
  }

  if (isConnected())
  {
    WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
      Serial.print("WiFi lost connection. Reason: ");
      Serial.println(info.wifi_sta_disconnected.reason);
      Wifi::reset();
    },
                 WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
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
      reset();
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

void Wifi::reset()
{
  Log.warning("WiFi reset invoked, disconnecting, turning WiFi off and restarting.");
  WiFi.persistent(false);
  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_OFF);
  Log.warning("Waiting 15s for disconnect");
  delay(15000);
  Log.fatal("ESP restarting");
  ESP.restart();
}