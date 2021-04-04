import time
import re
import ujson
import uasyncio as asyncio
from lib.mqtt_as import config as mqttConfig
import config
from mqtt import connect_mqtt
from fan import Fan
from temperature import Temperature
from servo import Servo
import iot

IOT_SHADOW = "$aws/things/{}/shadow/name/{}"

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
last_debug_publish_time = 0


async def update_shadow(thing, shadow, message, method="/update"):
    topic = IOT_SHADOW.format(thing, shadow) + method
    await client.publish(topic, message)
    print("Published update", topic, message)


async def callback_connection(c):
    print("Connection callback")
    state = {"state": {"reported": {"connection": "Connected"}}}
    await update_shadow(config.IOT_DEVICE_THING_NAME, "connection", ujson.dumps(state))

    print("Subscribing to current shadows")
    await c.subscribe(
        IOT_SHADOW.format(
            config.IOT_TEMPERATURE_THING_NAME, config.IOT_TEMPERATURE_THING_SHADOW_NAME
        )
        + "/get/accepted"
    )
    await c.subscribe(
        IOT_SHADOW.format(
            config.IOT_DEVICE_THING_NAME, config.IOT_DEVICE_THING_SHADOW_NAME
        )
        + "/get/accepted"
    )

    print("Requesting current shadows")
    await update_shadow(
        config.IOT_TEMPERATURE_THING_NAME,
        config.IOT_TEMPERATURE_THING_SHADOW_NAME,
        "",
        method="/get",
    )
    await update_shadow(
        config.IOT_DEVICE_THING_NAME,
        config.IOT_DEVICE_THING_SHADOW_NAME,
        "",
        method="/get",
    )

    print("Subscribing to temperature updates")
    await c.subscribe(
        IOT_SHADOW.format(
            config.IOT_TEMPERATURE_THING_NAME, config.IOT_TEMPERATURE_THING_SHADOW_NAME
        )
        + "/update/accepted"
    )
    await c.subscribe(
        IOT_SHADOW.format(
            config.IOT_DEVICE_THING_NAME, config.IOT_DEVICE_THING_SHADOW_NAME
        )
        + "/update/accepted"
    )


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

    if (
        iot.has_property_changed(msg, "fanSpeed") is True
        or iot.is_in_sync(msg, "fanSpeed", fan.duty()) is False
    ):
        if "fanSpeed" in msg["state"]["desired"].keys():
            speed = msg["state"]["desired"]["fanSpeed"]
            print("Updating fan speed to:", speed)
            fan.set_duty(speed)

        state["state"]["reported"]["fanSpeed"] = fan.duty()
        state_changed = True

    if (
        iot.has_property_changed(msg, "servoAngle") is True
        or iot.is_in_sync(msg, "servoAngle", servo.angle()) is False
    ):
        if "servoAngle" in msg["state"]["desired"].keys():
            angle = msg["state"]["desired"]["servoAngle"]
            print("Updating servo angle to:", angle)
            servo.set_angle(angle)

        state["state"]["reported"]["servoAngle"] = servo.angle()
        state_changed = True

    if (
        iot.has_property_changed(msg, "sensor") is True
        or iot.is_in_sync(msg, "sensor", temperature.measuring_sensor) is False
    ):
        if "sensor" in msg["state"]["desired"].keys():
            temperature.measuring_sensor = msg["state"]["desired"]["sensor"]
            print("Changing measuring sensor to:", temperature.measuring_sensor)
        state["state"]["reported"]["sensor"] = temperature.measuring_sensor
        state_changed = True

    if (
        iot.has_property_changed(msg, "value") is True
        or iot.is_in_sync(msg, "value", temperature.setpoint) is False
    ):
        if "value" in msg["state"]["desired"].keys():
            temperature.setpoint = msg["state"]["desired"]["value"]
            print("Changing setpoint to:", temperature.setpoint)
        state["state"]["reported"]["value"] = temperature.setpoint
        state_changed = True

    if state_changed:
        asyncio.create_task(
            update_shadow(
                config.IOT_DEVICE_THING_NAME,
                config.IOT_DEVICE_THING_SHADOW_NAME,
                ujson.dumps(state),
            )
        )


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
        result = temperature.update()
        global last_debug_publish_time
        if config.IOT_PUBLISH_DEBUG and result is not None and time.time() - last_debug_publish_time > config.IOT_DEBUG_PERIOD :
            print("Debug publishing to ", config.IOT_DEBUG_TOPIC)
            temp, setpoint, fan_speed, servo_angle = result
            message = {
                "pid": {
                    "temperature": temp,
                    "setpoint": setpoint,
                    "fanSpeed": fan_speed,
                    "servoAngle": servo_angle,
                }
            }
            await client.publish(config.IOT_DEBUG_TOPIC, ujson.dumps(message))
            last_debug_publish_time = time.time()
        await asyncio.sleep(1)


def main():
    global client, fan, temperature, servo
    mqttConfig["will"] = (
        "bbqmonitor/connection/{}/updates".format(config.IOT_DEVICE_THING_NAME),
        ujson.dumps({"state": {"reported": {"connection": "Disconnected"}}}),
    )
    mqttConfig["subs_cb"] = callback_message_received
    mqttConfig["connect_coro"] = callback_connection

    client = connect_mqtt(mqttConfig)
    fan = Fan()
    servo = Servo()
    temperature = Temperature(fan, servo)

    try:
        asyncio.run(run())
    finally:
        print("Closing...")
        fan.stop()
        servo.stop()
        client.close()  # Prevent LmacRxBlk:1 errors


if __name__ == "__main__":
    main()
