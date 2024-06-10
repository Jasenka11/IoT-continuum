import mqtt_client
import amqp_client
import json
#import argparse

# rabbit
ip_rabbit = "SERVER_IP"
port_rabbit = 60040
user_rabbit = "user"
pass_rabbit = "password"
exchange_rabbit = "amq.topic"
r_key_rabbit = "keh_key"
queue_rabbit = "keh_queue"
# put 0 for off 
rssiThreshold = -50
# mosquitto
ip_mosquitto = "RaspberryPi_IP"
topic_mosquitto_lora = "LoRa"
queuedLoraMsg = []

# connect to rabbitmq
amqp_ch = amqp_client.connect_to_broker(ip_rabbit, port=port_rabbit, user=user_rabbit, passw=pass_rabbit)
amqp_client.create_queue(amqp_ch, exchange_rabbit, r_key_rabbit, queue_rabbit)

# connect to mosquitto and subscribe to topics
mqtt_client.message = None
mqttc = mqtt_client.connect(ip_mosquitto)
mqttc.subscribe(topic_mosquitto_lora)

# processing incoming LoRa messages based on a threshold rssi value
def forwarding(rssiThreshold, rssiVal, payload, queuedLoraMsg):
        # if rssi value of new message is bigger then the threshold, publish that message immedialtely
        if  rssiVal >= rssiThreshold:
             print("sensor-controller - AMQP publisher - Forwarding data from Mosquitto to Rabbitmq: " + payload)
             amqp_ch.basic_publish(exchange_rabbit, r_key_rabbit, payload)
             # if there are queued messages, publish all elements of list individually
             if queuedLoraMsg:
                  for el in queuedLoraMsg:
                      amqp_ch.basic_publish(exchange_rabbit, r_key_rabbit, el)
                      print("sensor-controller - AMQP publisher - Forwarding data from Mosquitto to Rabbitmq: " + el)
             #empting list with queued lora messages
             queuedLoraMsg.clear() 

        # if there is no threshold, then publish all incomining messages immediately 
        elif rssiThreshold == 0:
             print("sensor-controller - AMQP publisher - Forwarding data from Mosquitto to Rabbitmq: " + payload)
             amqp_ch.basic_publish(exchange_rabbit, r_key_rabbit, payload)
        # if the rssi value is smaller then the threshold, add message to queue 
        elif rssiVal < rssiThreshold:
             queuedLoraMsg.append(payload)
             print("adding message to queue of length", len(queuedLoraMsg))  

# keep checking if new data arrived
mqttc.loop_start()
while True:
   # print("hallo")
    if mqtt_client.message is not None:
        # retrieve topic and payload
        topic = mqtt_client.message.topic
        payload = str(mqtt_client.message.payload.decode("utf-8"))
        # if the topic of the mqtt message is "lora" then extract rssi value from payload and call forwarding function
        if topic == topic_mosquitto_lora:
             encodeJson = json.loads(payload)
             rssiVal = encodeJson["rssi"]
             forwarding(rssiThreshold,rssiVal,payload, queuedLoraMsg)
        # reset message
        mqtt_client.message = None

