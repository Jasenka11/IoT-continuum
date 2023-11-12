#include "LoRaWan_APP.h"
#include "Arduino.h"
#include <Wire.h>
#include "PubSubClient.h"
#include "LoRaInterface.h"
#include "LoRa.h"
#include "heltec.h"
#include <ArduinoJson.h>
DynamicJsonDocument doc(255);


#define RF_FREQUENCY                                868000000 // Hz

#define TX_OUTPUT_POWER                             18        // dBm

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
#define BUFFER_SIZE                                 30 // Define the payload size here

char txpacket[BUFFER_SIZE];
char rxpacket[BUFFER_SIZE];

static RadioEvents_t RadioEvents;
int16_t txNumber;
int16_t rssi,rxSize;
bool lora_idle = true;
static bool _receivedFlag = false;
static String _payloadBuffer = "";

#include "WiFi.h"

// WiFi Credentials
static const char *_ssid = "netw1";
static const char *_password = "password1";

static const char *_topic = "test";

WiFiClient _wifiClient;
PubSubClient _mqttClient(_wifiClient);

#define MQTT_SERVER   IPAddress(192, 168, 1, 5)
#define WIFI_SSID     "netwX"
#define WIFI_PASSWORD "passwordX"


static boolean connectWifi(const char *ssid, const char *password) {
  boolean success = true;
  int i = 0;
  WiFi.disconnect(true);
  delay(500);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoConnect(true);
  WiFi.begin(ssid, password);
  //displayString(0, 0, "Connecting to WiFi...");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if (i > 10) {
      success = false;
      break;
    }
    i++;
  }
  return success;
}

bool isWiFiConnected() {
  return WiFi.status() == WL_CONNECTED;
}



// connect to MQTT broker
void connectToMQTTServer(IPAddress addr, uint16_t port) {
  _mqttClient.setServer(addr, port);
  connectMQTT();
}

// connect to MQTT server using host and port
//
void connectToMQTTServer(const char *host, uint16_t port) {
  _mqttClient.setServer(host, port);
  connectMQTT();
}
uint16_t _chipID = 44;
void connectMQTT() {
  // Wait until we're connected
  while (!_mqttClient.connected()) {
  
    String clientId = "LoRa-Gateway-Heltec";
    clientId += String((uint32_t)(_chipID>>32));
    clientId += String((uint32_t)_chipID);
    Serial.printf("MQTT connecting as client %s...\n", clientId.c_str());
 
    if (_mqttClient.connect(clientId.c_str())) {
      Serial.println("MQTT connected");
  
      _mqttClient.publish(_topic, String(clientId+" connected").c_str());
    } else {
      Serial.printf("MQTT failed, state %s, retrying...\n", _mqttClient.state());
    
      delay(2500);
    }
  }
}


void updateMQTT() {
  if (!_mqttClient.connected()) {
    connectMQTT();
  }
  _mqttClient.loop();
}

void publishMQTT(const char *topic, const char *msg) {
    _mqttClient.publish(topic, msg);
}

void publishMQTT(const char *msg) {
    _mqttClient.publish(_topic, msg);
}



// LoRa Settings


#define USE_SPREAD_FACTOR 
static int _spreadFactor = MIN_SPREAD_FACTOR;

#ifdef ENA_TRANSMIT
#define SEND_INTERVAL (1000)
static long _lastSendTime = 0;
static uint32_t _txCounter = 0;
#endif


void configureLoRa() {
  Serial.println("configureLoRa()");
#ifdef ENA_TRANSMIT
  LoRa.setTxPowerMax(MAX_TX_POWER);
#endif
#ifdef USE_SPREAD_FACTOR  
 LoRa.setSpreadingFactor(_spreadFactor);
#endif
  LoRa.onReceive(onReceive);
  LoRa.receive();
}

// LoRa receiver

int paRssi() {
  return LoRa.packetRssi();
}

/
static void onReceive(int packetSize)
{
 
  digitalWrite(LED_BUILTIN, HIGH);
  _payloadBuffer = "";
  while (LoRa.available())
  {
    _payloadBuffer += (char) LoRa.read();
  }
  _receivedFlag = true;
  digitalWrite(LED_BUILTIN, LOW);
}


// LoRa transmitter

#ifdef ENA_TRANSMIT

int sendTestPacket() {
  Serial.println("Sending packet "+String(_txCounter));
  LoRa.beginPacket();
  LoRa.print("hello ");
  LoRa.print(_txCounter++);
  LoRa.endPacket();
}

void sendIfReady() {
  if(millis() - _lastSendTime > SEND_INTERVAL)
  {
    digitalWrite(LED_BUILTIN, HIGH);
    sendTestPacket();
    _lastSendTime = millis();

    clearDisplay();
    displayString(0, 0, "sent packet ");
    displayString(8, 0, (const char *)String(_txCounter-1).c_str());
    displaySpreadFactor(_spreadFactor);
    
    digitalWrite(LED_BUILTIN, LOW);
  }
}

#endif

String *checkRxBuffer() {
  Serial.println("in checkRXBuffer");
  static String payload;
  int leng = payload.length();
  Serial.println( "length payload: ");
  Serial.println(leng);

  
  //char str[leng] = payload;
  
//  int leng = payload.length();
  //String(payload).getBytes(_payloadBuffer, leng + 1);
  Serial.println("payload: ");
//  Serial.println(byte(payload));
  if (_receivedFlag && _payloadBuffer.length() > 0) {
    _receivedFlag = false;
    // ensure length does not exceed maximum
    int len = min((int)_payloadBuffer.length(), MAX_LORA_PAYLOAD-1);
    // copy String from rx buffer to local static variable for return to caller
    payload = _payloadBuffer;
    return &payload;
  } else {
    return NULL;
  }
}

static void checkAndForwardPackets() {

  String *rxPacketString = checkRxBuffer();
  Serial.println(" forwarding content to mqtt");

}
void initWiFi(const char *ssid, const char *password) {
  if (connectWifi(ssid, password)) {
    _ssid = ssid;
    _password = password;
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Connection failed.");
  }
  delay(1000);
}

void checkWiFiStatus() {
  if (!isWiFiConnected() && _ssid && _password) {
    connectWifi(_ssid, _password);
    //displayConnectionStatus();
    delay(1000);
  }
}



void setup() {
    Serial.begin(115200);
    Mcu.begin();
    
    txNumber=0;
    rssi=0;
  
    RadioEvents.RxDone = OnRxDone;
    Radio.Init( &RadioEvents );
    Radio.SetChannel( RF_FREQUENCY );
    Radio.SetRxConfig( MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                               LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                               LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                               0, true, 0, 0, LORA_IQ_INVERSION_ON, true );



  // Initialise wifi connection
  initWiFi(WIFI_SSID, WIFI_PASSWORD);
  
  if (isWiFiConnected()) {
    connectToMQTTServer(MQTT_SERVER, 1883);
    Serial.println("connected to wifi");
  }
  Serial.println("setup() done");

}


void loop()
{
  if(lora_idle)
  {
    lora_idle = false;
    Serial.println("into RX mode");
    Radio.Rx(0);
  }
  Radio.IrqProcess( );
  updateMQTT();
}

void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{
    rssi=rssi;
    rxSize=size;
    memcpy(rxpacket, payload, size );
    rxpacket[size]='\0';
    Radio.Sleep( );

    byte temp = byte(rxpacket[4]);
   //Serial.println(temp);
    byte humid = byte(rxpacket[5]);

    String strTemp = "";
    strTemp += String(temp);
    doc["temperature"] = strTemp;
    serializeJson(doc);
    

    
    const char *msg1 = strTemp.c_str();
    const char *msg2 = strHum.c_str();
    const char *msg3 = together.c_str();
    delay(1000);
    publishMQTT(msg1);
    delay(1000);
    delay(1000);
    publishMQTT(msg3);
    Serial.print("rx packet: msg: ");

    lora_idle = true;
   
}
