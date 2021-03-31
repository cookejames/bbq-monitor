import re
import ujson
import uasyncio as asyncio
from lib.mqtt_as.config import config
import config as local_config
from mqtt import connect_mqtt
from fan import set_fan_duty
from iot import has_property_changed

IOT_TEMPERATURE_SHADOW = "$aws/things/{}/shadow/name/{}".format(
    local_config.IOT_TEMPERATURE_THING_NAME,
    local_config.IOT_TEMPERATURE_THING_SHADOW_NAME,
)
IOT_DEVICE_SHADOW = "$aws/things/{}/shadow/name/{}".format(
    local_config.IOT_DEVICE_THING_NAME, local_config.IOT_DEVICE_THING_SHADOW_NAME
)
LATEST_SHADOW_REGEX = re.compile(
    r"^\$aws\/things\/(.+)\/shadow\/name\/(.+)\/get\/accepted$"
)
LATEST_UPDATE_REGEX = re.compile(
    r"^\$aws\/things\/(.+)\/shadow\/name\/(.+)\/update\/accepted$"
)

client = None


async def publish(topic, message):
    await client.publish(topic, message)
    print("Published update", topic, message)


async def callback_connection(c):
    print("Connection callback")

    print("Subscribing to current shadows")
    await c.subscribe("{}".format(IOT_TEMPERATURE_SHADOW + "/get/accepted"))
    await c.subscribe("{}".format(IOT_DEVICE_SHADOW + "/get/accepted"))

    print("Requesting current shadows")
    await c.publish(IOT_TEMPERATURE_SHADOW + "/get", "")
    await c.publish(IOT_DEVICE_SHADOW + "/get", "")

    print("Subscribing to temperature updates")
    await c.subscribe("{}".format(IOT_TEMPERATURE_SHADOW + "/update/accepted"))
    await c.subscribe("{}".format(IOT_DEVICE_SHADOW + "/update/accepted"))


def process_temperature(msg):
    print(msg)

def process_device(msg):
    if has_property_changed(msg, "fanSpeed") is False:
        return

    speed = msg["state"]["desired"]["fanSpeed"]
    print("Updating fan speed to:", speed)
    set_fan_duty(speed)

    state = {"state": {"reported": {"fanSpeed": speed}}}
    print("Publishing fan speed update")
    asyncio.create_task(publish(IOT_DEVICE_SHADOW + "/update", ujson.dumps(state)))


def callback_message_received(topic, msg, retained):
    topic = topic.decode("utf-8")
    msg = msg.decode("utf-8")

    if LATEST_SHADOW_REGEX.match(topic):
        match = LATEST_SHADOW_REGEX.match(topic)
    elif LATEST_UPDATE_REGEX.match(topic):
        match = LATEST_UPDATE_REGEX.match(topic)
    else:
        print("Topic is not recognised:", topic)
        return

    thing = match.group(1)
    shadow = match.group(2)
    try:
        obj = ujson.loads(msg)
    except Exception as e:
        print("Error parsing message:", msg)
        print(e)
    else:
        if (
            thing == local_config.IOT_TEMPERATURE_THING_NAME
        ) and shadow == local_config.IOT_TEMPERATURE_THING_SHADOW_NAME:
            print("Processing temperature message")
            process_temperature(obj)
        if (
            thing == local_config.IOT_DEVICE_THING_NAME
        ) and shadow == local_config.IOT_DEVICE_THING_SHADOW_NAME:
            print("Processing device message")
            process_device(obj)


async def run():
    await client.connect()
    while True:
        await asyncio.sleep(5)


def main():
    global client
    config["subs_cb"] = callback_message_received
    config["connect_coro"] = callback_connection

    client = connect_mqtt(config)

    try:
        asyncio.run(run())
    finally:
        print("Closing...")
        client.close()  # Prevent LmacRxBlk:1 errors


if __name__ == "__main__":
    main()
