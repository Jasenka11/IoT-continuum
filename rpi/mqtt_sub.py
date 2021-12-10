import paho.mqtt.client as mqtt  # Paho MQTT client


# callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
    print("MQTT - connected to broker with result code " + str(rc))


# callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
    global message
    message = str(msg.payload.decode("utf-8"))
    print("MQTT subscriber - message received [" + msg.topic + "]: " + message)


def on_subscribe(mqttc, obj, mid, granted_qos):
    print("MQTT - subscribed: " + str(mid) + " " + str(granted_qos))


def on_log(mqttc, obj, level, string):
    print(string)


def connect_to_mqtt_broker(ip, port):
    mqttc = mqtt.Client()
    mqttc.on_connect = on_connect
    mqttc.on_message = on_message
    mqttc.on_subscribe = on_subscribe
    # mqttc.on_log = on_log # Uncomment to enable debug messages
    mqttc.connect(ip, port, 60)  # connect to the broker
    return mqttc


if __name__ == "__main__":
    # parameters
    mqtt_broker_ip = "localhost"
    mqtt_broker_port = 1883
    mqtt_topic = "testMQTT"

    mqttc = connect_to_mqtt_broker(mqtt_broker_ip, mqtt_broker_port)
    mqttc.subscribe(mqtt_topic)  # subscribe to the topic

    mqttc.loop_forever()
