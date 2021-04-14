#include <awsiot.h>
#include <secrets.h>
#include <ArduinoLog.h>

#define AWS_SHADOW_TOPIC "$aws/things/%s/shadow/name/%s/%s"
#define CONNECTION_MESSAGE "{\"state\": {\"reported\": {\"connection\": \"Connected\"}}}"
#define DISCONNECTION_MESSAGE "{\"state\": {\"reported\": {\"connection\": \"Disconnected\"}}}"

static WiFiClientSecure net = WiFiClientSecure();
static MQTTClient client = MQTTClient(10000);
static MqttMessageHandler messageHandler = [](String &topic, String &payload) { return; };
static unsigned long startTime = 0;

bool AwsIot::publish(const char *topic, const char *message)
{
  Log.trace("IoT publishing to: %s - %s", topic, message);
  if (!isConnected())
  {
    Log.warning("AWS IOT cannot publish - disconnected");
    return false;
  }

  bool success = client.publish(topic, message);
  if (!success)
  {
    lwmqtt_err_t error = client.lastError();
    Log.error("AWS IOT publish failed: %d", error);
  }
  return success;
}

void AwsIot::setMessageHander(MqttMessageHandler handler)
{
  messageHandler = handler;
  client.onMessage(handler);
}

bool AwsIot::connect()
{
  startTime = millis();
  if (isConnected())
  {
    return true;
  }

  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.begin(AWS_IOT_ENDPOINT, 8883, net);

  // Create a message handler
  client.onMessage(messageHandler);

  char willTopic[200];
  sprintf(willTopic, "bbqmonitor/connection/%s/updates", THINGNAME);
  client.setWill(willTopic, DISCONNECTION_MESSAGE);
  client.setTimeout(2000);
  client.setKeepAlive(30);

  Log.notice("Connecting to AWS IOT");

  while (!client.connect(THINGNAME))
  {
    delay(100);
  }

  if (!isConnected())
  {
    Log.error("AWS IoT Timeout!");
    return false;
  }

  Log.notice("AWS IoT Connected!");

  subscribe();
  publishToShadow("connection", "update", CONNECTION_MESSAGE);
  return true;
}

bool AwsIot::isConnected()
{
  return client.connected();
}

void AwsIot::subscribeToShadow(const char *shadowName, const char *method)
{
  char topic[200];
  sprintf(topic, AWS_SHADOW_TOPIC, THINGNAME, shadowName, method);
  Log.trace("IoT subscribing to: %s", topic);
  client.subscribe(topic);
}

void AwsIot::subscribe()
{
  subscribeToShadow("setpoint", "get/accepted");
  subscribeToShadow("setpoint", "update/accepted");
  subscribeToShadow("pid", "get/accepted");
  subscribeToShadow("pid", "update/accepted");
}

void AwsIot::publishToShadow(const char *shadowName, const char *method, const char *message)
{
  char topic[200];
  sprintf(topic, AWS_SHADOW_TOPIC, THINGNAME, shadowName, method);
  publish(topic, message);
}

void AwsIot::check()
{
  if (isConnected())
  {
    client.loop();
  }
  else
  {
    if (millis() > startTime + waitTime)
    {
      Log.warning("AwsIot disconnected. Reconnecting.");
      Log.error("AWS IOT last error was: return code %d, error %d", client.returnCode(), client.lastError());
      client.disconnect();
      bool success = connect();
      if (!success)
      {
        Log.error("AWS IOT reconnected failed: %d", client.lastError());
      }
    }
  }
}
