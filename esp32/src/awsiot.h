#ifndef awsiot_h
#define awsiot_h
#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <MQTTClient.h>

using MqttMessageHandler = void (*)(String &, String &);

class AwsIot
{
public:
  AwsIot();
  static void setMessageHander(MqttMessageHandler);
  static bool connect();
  static void check();
  static bool isConnected();
  static void publishToShadow(const char *, const char *, const char *);

private:
  static bool publish(const char *, const char *);
  static void subscribe();
  static void subscribeToShadow(const char *, const char *);
  static const unsigned long waitTime = 30 * 1000;
};

#endif
