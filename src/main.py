
import re
from time import sleep
import config
import mqtt

IOT_TEMPERATURE_SHADOW = "$aws/things/{}/shadow/name/{}".format(
    config.IOT_TEMPERATURE_THING_NAME, config.IOT_TEMPERATURE_THING_SHADOW_NAME)
LATEST_SHADOW_REGEX = re.compile(r"^\$aws\/things\/(.+)\/shadow\/name\/(.+)\/get\/accepted$")
TEMPERATURE_UPDATE_REGEX = re.compile(r"^\$aws\/things\/(.+)\/shadow\/name\/(.+)\/update\/accepted$")

client = None

def sub_cb(topic, msg):
    topic = topic.decode('utf-8')
    msg = msg.decode('utf-8')
    if LATEST_SHADOW_REGEX.match(topic):
        print("Fetched the latest thing shadow")
    if TEMPERATURE_UPDATE_REGEX.match(topic):
        print("Fetched the latest sensor reading")
    print(msg)


def initialise_mqtt():
    global client
    client = mqtt.connect_mqtt(config.ENDPOINT, config.CLIENT_ID)
    client.set_callback(sub_cb)

    # Get the latest shadow
    topic = IOT_TEMPERATURE_SHADOW + "/get/accepted"
    client.subscribe(b"{}".format(topic))
    client.publish(IOT_TEMPERATURE_SHADOW + "/get", b"")

    # Subscribe to updates
    topic = IOT_TEMPERATURE_SHADOW + "/update/accepted"
    client.subscribe(b"{}".format(topic))


def main():
    global client
    initialise_mqtt()

    while True:
        # Non-blocking wait for message
        client.check_msg()
        # Then need to sleep to avoid 100% CPU usage (in a real
        # app other useful actions would be performed instead)
        sleep(1)

    client.disconnect()


if __name__ == "__main__":
    main()
