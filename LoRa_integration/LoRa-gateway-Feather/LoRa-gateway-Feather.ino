#include <SPI.h>
#include <RH_RF95.h>
#include <RHSPIDriver.h>
#include <RHGenericDriver.h>
#define RFM95_CS A1
#define RFM95_RST 10
#define RFM95_INT 12

// Receiving frequency 870.0 -> no duty cyle as long as tx power is under 5mW
#define RF95_FREQ 870.0

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

void setup()
{
  // reset LoRa Radio
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  //while (!Serial);
  Serial.begin(9600);
  delay(1500);

  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);
  // ensuring that LoRa radio is initialized
  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    while (1);
  }
    // Set radio frequency to one defined on top
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }
  
  // setting the LoRa specific parameters

  rf95.setSignalBandwidth(500000);
  rf95.setSpreadingFactor(7);
  rf95.setCodingRate4(5);
  

  // about 20mW which fits to the EU wide harmonised national radio interfaces from 25 MHz to 1 000 MHz for 868MHz which is 25mW
  rf95.setTxPower(13);

  //for debugging: to check if Registers are set correctly;
  //Serial.println("registers in setup");
  //rf95.printRegisters();
  //Serial.println(rf95.spiRead(0x1E));

  delay(1000);
}

void loop()
{
  // buffer and message length initialized
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);
  //  //waiting for a message:

  if (rf95.waitAvailableTimeout(1000))
  {
    // Should be a message for us now
    if (rf95.recv(buf, &len))
    {
      // printing message, size of message, Rssi and Snr of last received message
      Serial.print((char*)buf);
      Serial.print(" size_of_message ");
      Serial.print(len);
      Serial.print(" RSSI ");
      Serial.print(rf95.lastRssi(), DEC);
      Serial.print(" SNR ");
      Serial.println(rf95.lastSNR());
    }
    else
    {
      Serial.println("recv failed");
    }
  }
  delay(1000);
}
