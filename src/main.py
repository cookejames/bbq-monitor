import uasyncio as asyncio
import re
from lib.mqtt_as.config import config
import config as local_config
from mqtt import connect_mqtt

IOT_TEMPERATURE_SHADOW = "$aws/things/{}/shadow/name/{}".format(
    local_config.IOT_TEMPERATURE_THING_NAME, local_config.IOT_TEMPERATURE_THING_SHADOW_NAME)
LATEST_SHADOW_REGEX = re.compile(r"^\$aws\/things\/(.+)\/shadow\/name\/(.+)\/get\/accepted$")
TEMPERATURE_UPDATE_REGEX = re.compile(r"^\$aws\/things\/(.+)\/shadow\/name\/(.+)\/update\/accepted$")

async def callback_connection(client):
    print('Connection callback')

    print('Subscribing to current shadow')
    topic = IOT_TEMPERATURE_SHADOW + "/get/accepted"
    await client.subscribe("{}".format(topic))

    print('Requesting current shadow')
    await client.publish(IOT_TEMPERATURE_SHADOW + "/get", "")

    print('Subscribing to temperature updates')
    topic = IOT_TEMPERATURE_SHADOW + "/update/accepted"
    await client.subscribe("{}".format(topic))

def callback_message_received(topic, msg, retained):
    topic = topic.decode('utf-8')
    msg = msg.decode('utf-8')

    if LATEST_SHADOW_REGEX.match(topic):
        print("Received the latest thing shadow")
    if TEMPERATURE_UPDATE_REGEX.match(topic):
        print("Received the latest sensor reading")
    print(msg)

async def run(client):
    await client.connect()
    while True:
        await asyncio.sleep(5)

def main():
    config['subs_cb'] = callback_message_received
    config['connect_coro'] = callback_connection

    client = connect_mqtt(config)

    try:
        asyncio.run(run(client))
    finally:
        print('Closing...')
        client.close()  # Prevent LmacRxBlk:1 errors

if __name__ == "__main__":
    main()
