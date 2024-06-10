import mqtt_client
import amqp_client
import lora_client
import json

# rabbit
ip_rabbit = "SERVER_IP"
port_rabbit = 60040
user_rabbit = "user"
pass_rabbit = "password"
exchange_rabbit = "amq.topic"
r_key_rabbit = "lora_key"
queue_rabbit = "lora_queue"
# mosquitto
ip_mosquitto = "192.168.1.5"
topic_mosquitto = "keh_lora_topic"
USE_MQTT = True
# connect to rabbitmq
amqp_ch = amqp_client.connect_to_broker(ip_rabbit, port=port_rabbit, user=user_rabbit, passw=pass_rabbit)
amqp_client.create_queue(amqp_ch, exchange_rabbit, r_key_rabbit, queue_rabbit)

# connect to mosquitto and subscribe to topics
mqtt_client.message = None
mqttc = mqtt_client.connect(ip_mosquitto)
mqttc.subscribe(topic_mosquitto)

# keep checking if new data arrived
mqttc.loop_start()
while True:
    loramsg = json.loads(lora_client.on_lora_msg(topic_mosquitto,ip_mosquitto,USE_MQTT))
    # if a lora msg was received using the lora client forward it using the lora client
    if loramsg is not None and not USE_MQTT :
        print("sensor-controller - LoRa - Message received - Forwarding data to Rabbitmq:  ")
        print(str(loramsg))
        amqp_ch.basic_publish(exchange_rabbit, r_key_rabbit, str(loramsg))
        loramsg = None
 
    if mqtt_client.message is not None:
        # retrieve topic and payload
        topic = mqtt_client.message
        payload = str(mqtt_client.message.payload.decode("utf-8"))
        print("sensor-controller - MQTT subscriber - Message received: " + payload)
        # forward payload to amqp server
        print("sensor-controller - AMQP publisher - Forwarding data from Mosquitto to Rabbitmq: " + payload)
        amqp_ch.basic_publish(exchange_rabbit, r_key_rabbit, payload)
        # reset message
        mqtt_client.message = None


