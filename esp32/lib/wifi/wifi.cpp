#include <secrets.h>
#include "WiFi.h"
#include <ArduinoLog.h>

unsigned long startTime;
unsigned long waitTime = 60*1000;

bool wifiConnected()
{
  return WiFi.status() == WL_CONNECTED;
}

bool wifiConnect()
{
  startTime = millis();
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Log.notice("Connecting to WiFi SSID %s", WIFI_SSID);

  while (!wifiConnected() && millis() < startTime + waitTime)
  {
    delay(500);
  }

  if (wifiConnected())
  {
    Log.notice("WiFi Connected: %s - %s", WiFi.macAddress().c_str(), WiFi.localIP().toString());
    return true;
  }
  else
  {
    Log.error("WiFi connection failed");
    return false;
  }
}