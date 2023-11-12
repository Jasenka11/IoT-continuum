import paho.mqtt.client as mqtt
import logging
import time
import json
import serial
import struct

ser = serial.Serial('/dev/ttyACM0')
print(ser.name)
#ser.reset_input_buffer()
#ser.reset_output_buffer()
ser.flush()
listOfPakets = []
dictOfPakets = {}
keys = ["ID","num","D1", "D2"]
keystemphum = ["temp", "humid"]
def writeNewMsgToList():
    data = ser.read_until(b"\r\n")
    print(data)
    print(len(data)) 
    print(type(data))
#    datanew = ser.read_until(b":")
#    print(datanew)
    if isinstance(data, bytes):
#        print("bytes")
#        data = data.decode()
#        print(data)
        if len(data) == 6:
            packet = struct.unpack("BBBB", data[:-2])
    #        dictOfPakets = dict(zip(keys,packet))
        #for floats
        if len(data) == 7:
            packet = struct.unpack("BBBB", data[:-3])
            temp_val1 = packet[0]
            temp_val2 = packet[1]
            hum_val1 = packet[2]
            hum_val2 = packet[3]
            tempval = str(temp_val1) + "." + str(temp_val2)
            print(tempval) 
            print(type(tempval))
            humval = str(hum_val1) + "." + str(hum_val2)
            packet = (tempval, humval)
            print(packet) 
        if len(data) == 4:
            packet = struct.unpack("BB", data[:-2])
            print(packet)
        if len(data) == 13:
            packet = struct.unpack("BBBBBBBBBBBBB" , data[:-2])
        if len(data) == 8:
            packet = struct.unpack("BBBBBB" , data[:-2])
    #        dictOfPakets = dict(zip(keystemphum,packet))
        if len(data) == 11:
            packet = struct.unpack("BBBBBBBBB", data[:-2])
        if len(data) == 12:
            packet = struct.unpack("BBBBBBBBBB", data[:-2])
        dictOfPakets = dict(zip(keystemphum,packet))
        listOfPakets.append(dictOfPakets)
        print(listOfPakets[0])

    else:   
        encoding = 'utf-8'
        dataNew =  str(data, encoding)
        if "." in data:
            encoding = 'utf-8'
            dataNew = str(data, encoding)
            splitted = dataNew.split(":")
            print(splitted)
            temp = splitted[0]
            humi = splitted[1]
            print(humi)
            hum = humi.replace('\r\n','')
            listfordic = [temp,hum]
            print(listfordic)
#        newstring = "'temp'"+':'+temp + ',' + "'humid'"+':'+hum
 #       print(newstring)

            dictOfPakets = dict(zip(keystemphum,listfordic))
            listOfPakets.append(dictOfPakets)
            print(listOfPakets[0])

 

def on_connect(client, userdata, flags, rc):
    logging.info("MQTT - connected to broker, code " + str(rc))


def on_message(client, userdata, msg):
    global message
    message = msg
    topic = msg.topic
    payload = str(msg.payload.decode("utf-8"))
    logging.debug("MQTT - msg received [" + topic + "]: " + payload)


def on_subscribe(client, userdata, mid, granted_qos):
    logging.info("MQTT - subscribed: " + str(mid) + " " + str(granted_qos))


def on_publish(client, userdata, mid):
    logging.debug("MQTT - published:  " + str(mid))


def on_log(client, userdata, level, string):
    logging.debug(string)


def connect(ip, port=1883):
    mqttc = mqtt.Client()
    mqttc.on_connect = on_connect
    mqttc.on_message = on_message
    mqttc.on_publish = on_publish
    mqttc.on_subscribe = on_subscribe
    # mqttc.on_log = on_log
    try:
        mqttc.connect(ip, port, 60)
    except:
        logging.error(
            "MQTT - [" + ip + ":" + str(port) + "] connection failed!, exiting..."
        )
        exit(1)
    return mqttc


if __name__ == "__main__":
    logging.basicConfig(
        format="%(asctime)s | %(name)s | %(levelname)s | %(message)s",
        level=logging.DEBUG,
    )

    mqtt_broker_ip = "localhost"
    mqtt_broker_port = 1883
    mqtt_topic = "HALLO"
    mqttc = connect(mqtt_broker_ip, mqtt_broker_port)

    while True:
         lengthOfList= len(listOfPakets)
         writeNewMsgToList()
#         print(listOfPakets)
         mqttc.publish(mqtt_topic, payload=json.dumps(listOfPakets[0]))
         listOfPakets.pop(lengthOfList)
#         print(listOfPakets)

#         time.sleep(5)
   # mqttc.loop_forever()
