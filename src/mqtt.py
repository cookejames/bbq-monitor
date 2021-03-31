from umqtt.simple import MQTTClient

CACERT_PATH = "certs/aws-root-ca.pem"
KEY_PATH = "certs/private.pem.key"
CERT_PATH = "certs/certificate.pem.crt"


def connect_mqtt(endpoint, client_id):
    with open(KEY_PATH, "r") as f:
        key = f.read()

    # with open(CACERT_PATH, "r") as f:
    #     key2 = f.read()

    with open(CERT_PATH, "r") as f:
        cert = f.read()

    client = MQTTClient(
        client_id=client_id,
        server=endpoint,
        port=8883,
        keepalive=1200,
        ssl=True,
        ssl_params={"key": key, "cert": cert, "server_side": False},
    )
    print("Connecting to MQTT")
    client.connect()
    print("MQTT Connected")
    return client
