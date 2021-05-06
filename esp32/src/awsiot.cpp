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

static MqttMessage mqttMessages[128];
static etk::RingBuffer<MqttMessage, true> iotBuffer(mqttMessages, 128);

void AwsIot::buffer(const char *topic, const char *message)
{
  // Allocate memory for the message and topic
  char *_topic = new char[strlen(topic) + 1];
  strcpy(_topic, topic);
  char *_message = new char[strlen(message) + 1];
  strcpy(_message, message);

  // Add the message to the buffer
  MqttMessage mqtt;
  mqtt.message = _message;
  mqtt.topic = _topic;
  if (iotBuffer.is_full())
  {
    Log.warning("IoT buffer is full, old messages overwritten.");
  }
  iotBuffer.put(mqtt);
}

bool AwsIot::publish(const char *topic, const char *message)
{
  Log.trace("IoT publishing to: %s - %s", topic, message);

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
  subscribeToShadow("controlstate", "get/accepted");
  subscribeToShadow("controlstate", "update/accepted");
  subscribeToShadow("pid", "get/accepted");
  subscribeToShadow("pid", "update/accepted");
  subscribeToShadow("damper", "get/accepted");
  subscribeToShadow("damper", "update/accepted");
}

void AwsIot::publishToShadow(const char *shadowName, const char *method, const char *message)
{
  char topic[200];
  sprintf(topic, AWS_SHADOW_TOPIC, THINGNAME, shadowName, method);
  buffer(topic, message);
}

void AwsIot::check()
{
  if (isConnected())
  {
    client.loop();
    while (iotBuffer.available())
    {
      MqttMessage message = iotBuffer.get();
      bool success = publish(message.topic, message.message);
      if (!success)
      {
        Log.error("Publishing message failed");
      }
      delete[] message.message;
      delete[] message.topic;
    }
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
