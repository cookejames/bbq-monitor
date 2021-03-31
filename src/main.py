import re
import ujson
import uasyncio as asyncio
from lib.mqtt_as import config as mqttconfig
import config
from mqtt import connect_mqtt
from fan import Fan
from temperature import Temperature
from servo import Servo
import iot

IOT_TEMPERATURE_SHADOW = "$aws/things/{}/shadow/name/{}".format(
    config.IOT_TEMPERATURE_THING_NAME,
    config.IOT_TEMPERATURE_THING_SHADOW_NAME,
)
IOT_DEVICE_SHADOW = "$aws/things/{}/shadow/name/{}".format(
    config.IOT_DEVICE_THING_NAME, config.IOT_DEVICE_THING_SHADOW_NAME
)
LATEST_SHADOW_REGEX = re.compile(
    r"^\$aws\/things\/(.+)\/shadow\/name\/(.+)\/get\/accepted$"
)
LATEST_UPDATE_REGEX = re.compile(
    r"^\$aws\/things\/(.+)\/shadow\/name\/(.+)\/update\/accepted$"
)

client = None
fan = None
temperature = None
servo = None


async def publish(topic, message):
    await client.publish(topic, message)
    print("Published update", topic, message)


async def callback_connection(c):
    print("Connection callback")

    print("Subscribing to current shadows")
    await c.subscribe("{}".format(IOT_TEMPERATURE_SHADOW + "/get/accepted"))
    await c.subscribe("{}".format(IOT_DEVICE_SHADOW + "/get/accepted"))

    print("Requesting current shadows")
    await c.publish(IOT_DEVICE_SHADOW + "/get", "")
    await c.publish(IOT_TEMPERATURE_SHADOW + "/get", "")

    print("Subscribing to temperature updates")
    await c.subscribe("{}".format(IOT_TEMPERATURE_SHADOW + "/update/accepted"))
    await c.subscribe("{}".format(IOT_DEVICE_SHADOW + "/update/accepted"))


def process_temperature_message(msg):
    if "state" not in msg.keys() or "reported" not in msg["state"].keys():
        print("Message is malformed:", msg)
        return

    if temperature.measuring_sensor not in msg["state"]["reported"].keys():
        print(
            "Measuring sensor {} not found in report: ".format(
                temperature.measuring_sensor
            ),
            msg,
        )
        return

    if (
        iot.has_property_changed(msg, "temperature.measuring_sensor") is True
        or iot.is_in_sync(
            msg,
            msg["state"]["reported"][temperature.measuring_sensor],
            temperature.temperature,
        )
        is False
    ):
        temperature.temperature = msg["state"]["reported"][temperature.measuring_sensor]
        print("Updated monitoring temperature to {}C".format(temperature.temperature))


def process_device_message(msg):
    # We only act if there is a desired state, everything else is a report
    if "desired" not in msg["state"].keys():
        return

    state_changed = False
    state = {"state": {"reported": {}}}

    #TODO remove manual control
    if (
        iot.has_property_changed(msg, "fanSpeed") is True
        or iot.is_in_sync(msg, "fanSpeed", fan.duty()) is False
    ):
        speed = msg["state"]["desired"]["fanSpeed"]
        print("Updating fan speed to:", speed)
        fan.set_duty(speed)

        state["state"]["reported"]["fanSpeed"] = speed
        state_changed = True
    #Manual control ends here

    if (
        iot.has_property_changed(msg, "sensor") is True
        or iot.is_in_sync(msg, "sensor", temperature.measuring_sensor) is False
    ):
        temperature.measuring_sensor = msg["state"]["desired"]["sensor"]
        print("Changing measuring sensor to:", temperature.measuring_sensor)
        state["state"]["reported"]["sensor"] = temperature.measuring_sensor
        state_changed = True

    if (
        iot.has_property_changed(msg, "temperature") is True
        or iot.is_in_sync(msg, "temperature", temperature.setpoint) is False
    ):
        temperature.setpoint = msg["state"]["desired"]["temperature"]
        print("Changing setpoint to:", temperature.setpoint)
        state["state"]["reported"]["temperature"] = temperature.setpoint
        state_changed = True

    if state_changed:
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
            thing == config.IOT_TEMPERATURE_THING_NAME
        ) and shadow == config.IOT_TEMPERATURE_THING_SHADOW_NAME:
            print("Processing temperature message")
            process_temperature_message(obj)
        if (
            thing == config.IOT_DEVICE_THING_NAME
        ) and shadow == config.IOT_DEVICE_THING_SHADOW_NAME:
            print("Processing device message")
            process_device_message(obj)


async def run():
    await client.connect()
    while True:
        temperature.update()
        await asyncio.sleep(1)


def main():
    global client, fan, temperature, servo
    mqttconfig["subs_cb"] = callback_message_received
    mqttconfig["connect_coro"] = callback_connection

    client = connect_mqtt(mqttconfig)
    fan = Fan()
    servo = Servo()
    temperature = Temperature(fan, servo)

    try:
        asyncio.run(run())
    finally:
        print("Closing...")
        fan.stop()
        client.close()  # Prevent LmacRxBlk:1 errors


if __name__ == "__main__":
    main()
