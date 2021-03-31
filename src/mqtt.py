import config
from lib.mqtt_as import MQTTClient

CACERT_PATH = "certs/aws-root-ca.pem"
KEY_PATH = "certs/private.pem.key"
CERT_PATH = "certs/certificate.pem.crt"

DEBUG = True

def connect_mqtt(mqttConfig):
    with open(KEY_PATH, "r") as f:
        key = f.read()

    # with open(CACERT_PATH, "r") as f:
    #     key2 = f.read()

    with open(CERT_PATH, "r") as f:
        cert = f.read()

    mqttConfig['server'] = config.IOT_ENDPOINT
    mqttConfig['ssl'] = True
    mqttConfig['ssl_params'] = {"key": key, "cert": cert, "server_side": False}
    mqttConfig['client_id'] = config.IOT_CLIENT_ID
    mqttConfig['ssid'] = config.WIFI_SSID
    mqttConfig['wifi_pw'] = config.WIFI_PASSWORD

    MQTTClient.DEBUG = DEBUG
    return MQTTClient(mqttConfig)