#include "LoRaWan_APP.h"
#include "Arduino.h"
#include <ArduinoJson.h>
#include "WiFi.h"
#include "PubSubClient.h"


/////////////////////// Lora settings:
#define RF_FREQUENCY                                870000000 // Hz

#define TX_OUTPUT_POWER                             14        // dBm

#define LORA_BANDWIDTH                              0         // [0: 125 kHz,
//  1: 250 kHz,
//  2: 500 kHz,
//  3: Reserved]
#define LORA_SPREADING_FACTOR                       7         // [SF7..SF12]
#define LORA_CODINGRATE                             1         // [1: 4/5,
//  2: 4/6,
//  3: 4/7,
//  4: 4/8]
#define LORA_PREAMBLE_LENGTH                        8         // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT                         0         // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON                        false


#define RX_TIMEOUT_VALUE                            1000
#define BUFFER_SIZE                                 250 // Define the payload size here
// transmitted packet initialization
char txpacket[BUFFER_SIZE];
// received packet initialization
char rxpacket[BUFFER_SIZE];
char rxpacketlong[BUFFER_SIZE];

// RadioEvents are RxDone,TxDone,CadDone functions
static RadioEvents_t RadioEvents;
// function is called when cad is done
void onCadDone( bool channelActivityDetected);
// to check if channel is free
bool channelfree = false;
// CadSymbols are necessary to performe CAD (see Application Note: SX126x CAD Performance Evaluation from semtech)

uint8_t CadSymbols = 2;
// counter of packets
int16_t txNumber;
// rssi and size of rxpacket
int16_t rssi, rxSize;
// to deterimine if there is a lora transmission
bool lora_idle = true;
//for extracting information from lora paket
byte temp;
byte humid;

///// for writing lora messages to json array:
String JSONStorage[100];
//int msgcounter = 0;
DynamicJsonDocument doc(1024);

////// Wifi settings:
const char* ssid     = "netw111";
const char* password = "password1";

int globalMsgCounter;

///// MQTT settings:
// RPI IP adresse
const char* mqtt_server = "192.168.1.5";

WiFiClient Client;
PubSubClient client(Client);
const int num_subscribe_topics = 1;
String subscribe_topics[num_subscribe_topics] = {"LoRa"};
char* LoRa_topic = "LoRa";
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];
// string to safe queued mqtt msges
String queuedMQTTMsg = "";

//int value = 0;
// client id name
const char* client_id = "lora_data";
// theshold RSSI value to store lora messages
int thresholdRSSI = -90;

/////for showing more output in Serial Monitor
bool information = true;
bool clientbusy = false;
bool reqMsg = false;

//String to store current LoRa Msg
String latestLoRaMsg = "";


void setup() {
  Serial.begin(115200);
  Mcu.begin();

  txNumber = 0;
  rssi = 0;

  RadioEvents.RxDone = OnRxDone;
  RadioEvents.TxDone = OnTxDone;
  RadioEvents.TxTimeout = OnTxTimeout;
  RadioEvents.CadDone = OnCadDone;

  Radio.Init( &RadioEvents );
  Radio.SetChannel( RF_FREQUENCY );
  Radio.SetRxConfig( MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                     LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                     LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                     0, true, 0, 0, LORA_IQ_INVERSION_ON, true );

  wifiSetup();
  randomSeed(analogRead(0));
  globalMsgCounter = 0;
  Serial.println("initial globalMsgCounter; ");
  Serial.println(globalMsgCounter);
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop()
{
  //connecting mqtt client
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (lora_idle)
  {
    lora_idle = false;
    Serial.println("into RX mode");
    Radio.Rx(0);
  }
  Radio.IrqProcess( );
  //delay(1000);
}

void wifiSetup() {
  delay(10);
  // connecting wifi:
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

}
void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{
  delay(100);
  rssi = rssi;
  rxSize = size;
  Serial.println("newMsg arrived");
  Serial.println("message size:");
  Serial.println(rxSize);
  Serial.println("rssi of msg: ");
  Serial.println(rssi);
  memcpy(rxpacket, payload, size );
  Serial.println("printing paylpad in rxdone");
  Serial.println(rxpacket);
  rxpacket[size] = '\0';
  int counter = globalmessagecounter();
  // to prevent array from exeeding its size -> leads to rebooting of device
  if (counter == 100) {
    Serial.println("size exceeded write array over now" );
    globalMsgCounter = 0;
  }

  buildJSONStorage(payload, size, globalMsgCounter, false);
  latestLoRaMsg = updateLatestLoRaMsg(payload, size, globalMsgCounter);
  // publish needs a const char
  const char* mqttmsg = latestLoRaMsg.c_str();
  if (rssi > thresholdRSSI) {
    client.publish(LoRa_topic, mqttmsg);
    if (queuedMQTTMsg.length() > 0) {

      const char* queuedMQTTMsges = queuedMQTTMsg.c_str();
      Serial.println("publishing queued msges");
      client.publish(LoRa_topic, queuedMQTTMsges);
      queuedMQTTMsg = "";
    }
  }
  else {
    queuedMQTTMsg.concat(mqttmsg);
    Serial.println("queuedMQTTMsg: ");
    Serial.println(queuedMQTTMsg);
  }

  Radio.Sleep( );
  lora_idle = true;
  if (information) {
    Serial.println("message size:");
    Serial.println(rxSize);
    Serial.println("latest Lora msg: ");
    Serial.println(latestLoRaMsg);
    //Serial.println("requested Lora Msg: ");
   //Serial.println(requestedLoraMsg);
    Serial.println("JSONSTorage: ");
    for (int i = 0; i <= globalMsgCounter ; i++) {
      Serial.println(JSONStorage[i]);
    }
    Serial.println("msgcounter: ");
    Serial.println(globalMsgCounter);
  }
}
int globalmessagecounter() {
  globalMsgCounter += 1;
  return globalMsgCounter;
}

String buildLoRaMsg(uint8_t *payload, uint16_t size, int byteposition1 , int byteposition2) {
  rxSize = size;
  Serial.println("message size:");
  Serial.println(rxSize);
  memcpy(rxpacket, payload, size );
  rxpacket[size] = '\0';


  byte position1 = byte(rxpacket[byteposition1]);
  byte position2 = byte(rxpacket[byteposition2]);
  String decimalNum = "";
  String strPos1 = "";
  String strPos2 = "";
  strPos1 += String(position1);
  strPos2 += String(position2);
  decimalNum = strPos1 + "." + strPos2;
  if (byteposition2 == 0) {
    decimalNum = strPos1;
  }
  if (information) {
    Serial.println("decimal Num: ");
    Serial.println(decimalNum);
    Serial.println(position1);
    Serial.println(position2);
  }
  return decimalNum;
}
String buildJSONStorage(uint8_t *payload, uint16_t size, int msgCounter, bool retransmission) {
  String strTemp = "";
  String strHum = "";
  
  if (!retransmission) {
    if (size == 6) {
      strTemp = buildLoRaMsg(payload, size, 4, 0);
      strHum = buildLoRaMsg(payload, size, 5, 0);

    }
    if (size == 8) {
      strTemp = buildLoRaMsg(payload, size, 4, 5);
      strHum = buildLoRaMsg(payload, size, 6, 7);
    }
    if (size == 7) {
      strTemp = buildLoRaMsg(payload, size, 4, 5);
      strHum = buildLoRaMsg(payload, size, 6, 0);
      Serial.println("value fo strHum");
      if (strHum = 75){
        strHum = "keh";     
      }
    }
    else {
      Serial.println("long message expected");
      memcpy(rxpacketlong, payload, size );
      rxpacketlong[size] = '\0';
      Serial.println("rxpacket long");
      Serial.println(rxpacketlong);


      // function to extract temp and hum out of sentence
    }
  }
  else {
    strTemp = buildLoRaMsg(payload, size, 7, 8);
    strHum = buildLoRaMsg(payload, size, 9, 10);
  }
  delay(1000);

  String together = "";
  doc["id"] = client_id;
  if (retransmission) {
    doc["paketnumber"] = "retransmission" + String(txNumber);
  }
  else {
    doc["paketnumber"] = txNumber;

  }
  doc["temperature"] = strTemp;
  doc["identifier"] = strHum;
  serializeJson(doc, together);

  const char *msg1 = strTemp.c_str();
  const char *msg2 = strHum.c_str();
  const char *msg3 = together.c_str();

  if (information) {
    Serial.print("rx packet: msg: ");
    Serial.println(together);
  }
  JSONStorage[msgCounter] = together;
  txNumber += 1;
  return JSONStorage[msgCounter];
  txNumber += 1;
}

// to update the String "latestLoraMsg" when a new LoRa message arrives
String updateLatestLoRaMsg(uint8_t *payload, uint16_t size, int msgCounter) {
  String latestLoRaMsg = JSONStorage[msgCounter];
  return latestLoRaMsg;
}

///////////////////beginning MQTT functions
// is called when mqtt msg arrives
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}
// to reconnect wifi
void reconnect() {
  Serial.println("in reconnect()");
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "HeltecWiFiLoRaV3-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(LoRa_topic, "mqtt init");
      // ... and resubscribe
      //client.subscribe(LoRa_topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      if (client.state() == -2) {
        wifiSetup();
      }
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
// is called when message has been send
void OnTxDone( void )
{
  Serial.println("TX done......");
  lora_idle = true;
  channelfree = false;

}
// is called when timeout occurs in state transmission
void OnTxTimeout( void )
{
  Radio.Sleep( );
  Serial.println("TX Timeout......");
  lora_idle = true;
}
//is called when cad detection is finished (after timeout?)
void OnCadDone(bool channelActivityDetected) {
  delay(100);
  if (channelActivityDetected == true) {
    channelfree = false;
  }
  else {
    channelfree = true;
  }

}
