import serial
import logging
import json
import mqtt_client
import time

# used Serial port of the RPI
ser = serial.Serial('/dev/ttyACM0')
ser.flush()
dict_keys = ["id","temperature","humidity","size of message","rxNumber", "RSSI", "SNR"]

def temp_hum_extraction(payload,n):
    sensor  = [str(payload)[i:i+n]for i in range(0,len(str(payload)),n)]
    if n == 2:
        temp = int(sensor[0])
        humid = int(sensor[1])
        return temp, humid
    elif n == 3:
        temp = int(sensor[0]) / 10
        if len(sensor) >= 2:
            humid = int(sensor[1]) / 10
        else:
            humid = "-"
        return temp, humid 

def on_lora_msg(topic,ip_mosquitto,USE_MQTT):
    # cleaning received data
    data = ser.read_until(b"\r\n")
    datastring = data.decode("utf-8")
    datalist = datastring.split()
    datalist.insert(0,"LoRa_Data")
    datalist.pop(2)
    datalist.pop(3)
    datalist.pop(4)
    datalist.pop(5)
    # extract temperture data based on message length 
    sensorval = int(datalist[1])
    if int(datalist[2]) == 2:
        temp, humid = temp_hum_extraction(sensorval,2)
    # keh case:
    elif int(datalist[2]) == 3:
        temp, humid = temp_hum_extraction(sensorval,3)
        datalist.pop(0)
        datalist.insert(0, "keh_data")
    elif int(datalist[2]) == 4:
        temp, humid = temp_hum_extraction(sensorval,3) 
   
    else:
        print("received unexpected message")
        return 
    datalist.pop(1)
    datalist.insert(1,str(temp))
    datalist.insert(2, str(humid))
    resultdict = {}
    # write data to json object
    for key in dict_keys:
        for value in datalist:
            resultdict[key] = value
            datalist.remove(value)
            break
    json_object = json.dumps(resultdict, indent=4)
    json_string = json.loads(json_object)
    # publish msg to mosquitto broke if mqtt is used
    if USE_MQTT:
        mqttc = mqtt_client.connect(ip_mosquitto)
        mqttc.publish(topic,str(json_string))
    else:
        print("received lora msg: ", json_string)
    # sleep is necessary otherwise some msg are lost in the sensor controller part
    time.sleep(000.1) 
    return json_object


if __name__ == "__main__":
    logging.basicConfig(
        format="%(asctime)s | %(name)s | %(levelname)s | %(message)s",
        level=logging.DEBUG,
    )
    ip_mosquitto = "IP RPI"
    topic = "keh_lora_topic"
    USE_MQTT = True
       
    while True:
        on_lora_msg(topic,ip_mosquitto,USE_MQTT)
            

