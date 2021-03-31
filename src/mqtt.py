
CACERT_PATH = "certs/aws-root-ca.pem"
KEY_PATH = "certs/private.pem.key"
CERT_PATH = "certs/certificate.pem.crt"
import config as local_config
from lib.mqtt_as.mqtt_as import MQTTClient

DEBUG = True

def connect_mqtt(config):
    with open(KEY_PATH, "r") as f:
        key = f.read()

    # with open(CACERT_PATH, "r") as f:
    #     key2 = f.read()

    with open(CERT_PATH, "r") as f:
        cert = f.read()

    config['server'] = local_config.ENDPOINT
    config['ssl'] = True
    config['ssl_params'] = {"key": key, "cert": cert, "server_side": False}
    config['client_id'] = local_config.CLIENT_ID
    config['ssid'] = local_config.WIFI_SSID
    config['wifi_pw'] = local_config.WIFI_PASSWORD

    MQTTClient.DEBUG = DEBUG
    return MQTTClient(config)