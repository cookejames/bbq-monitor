#include <awsiot.h>
#include <secrets.h>
#include <ArduinoLog.h>

#define AWS_SHADOW_TOPIC "$aws/things/%s/shadow/name/%s/%s"
#define CONNECTION_MESSAGE "{\"state\": {\"reported\": {\"connection\": \"Connected\"}}}"
#define DISCONNECTION_MESSAGE "{\"state\": {\"reported\": {\"connection\": \"Disconnected\"}}}"

static WiFiClientSecure net = WiFiClientSecure();
static MQTTClient client = MQTTClient(4000);
static MqttMessageHandler messageHandler = [](String &topic, String &payload) { return; };

bool AwsIot::publish(const char *topic, const char *message)
{
  return client.publish(topic, message);
}

void AwsIot::setMessageHander(MqttMessageHandler handler)
{
  messageHandler = handler;
  client.onMessage(handler);
}

void AwsIot::connect()
{
  if (isConnected())
  {
    return;
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

  Log.notice("Connecting to AWS IOT");

  while (!client.connect(THINGNAME))
  {
    delay(100);
  }

  if (!isConnected())
  {
    Log.error("AWS IoT Timeout!");
    return;
  }

  Log.notice("AWS IoT Connected!");

  subscribe();
  publishToShadow("connection", "update", CONNECTION_MESSAGE);
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
}

void AwsIot::publishToShadow(const char *shadowName, const char *method, const char *message)
{
  char topic[200];
  sprintf(topic, AWS_SHADOW_TOPIC, THINGNAME, shadowName, method);
  Log.trace("IoT publishing to: %s - %s", topic, message);
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
    Log.warning("AwsIot disconnected.");
  }
}
