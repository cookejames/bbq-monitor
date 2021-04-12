#include <secrets.h>
#include "WiFi.h"
#include <wifi.h>
#include <ArduinoLog.h>

bool Wifi::isConnected()
{
  return WiFi.status() == WL_CONNECTED;
}

bool Wifi::connect()
{
  startTime = millis();
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Log.notice("Connecting to WiFi. SSID: %s", WIFI_SSID);

  while (!isConnected() && millis() < startTime + waitTime)
  {
    delay(500);
  }

  if (isConnected())
  {
    Log.notice("WiFi Connected: %s - %s", WiFi.macAddress().c_str(), WiFi.localIP().toString().c_str());
    return true;
  }
  else
  {
    Log.error("WiFi connection failed");
    return false;
  }
}

bool Wifi::check()
{
  if (isConnected())
  {
    return true;
  }
  else if (millis() > startTime + waitTime)
  {
    Log.warning("WiFi disconnected, reconnecting!");
    startTime = millis();
    WiFi.disconnect();
    WiFi.reconnect();
    bool connected = isConnected();
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