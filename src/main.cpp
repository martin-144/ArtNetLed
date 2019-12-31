#define FASTLED_ALLOW_INTERRUPTS 0
#define FASTLED_ESP8266_RAW_PIN_ORDER

#include <spiffs.h>
#include <wifimanager.h>
#include <FastLED.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <artnet.h>
#include <torch.h>
#include <string>

// UDP settings
// const uint16_t udp_port = 6454;
WiFiUDP Udp;

// Local fastLed Ports
#define LED_PIN D8

// Blue LED blink interval
const int interval = 500;
const int ledPin = 0x02;

// Flash button
const int flashButtonPin = 0x00;

IPAddress broadcastIp(255 ,255 ,255 ,255);

// WiFiManager related Stuff
String WiFiUniverse;
String WiFiDmxChannel;

// flag for saving data
bool shouldSaveParams = false;




// Main program
// ============

void setup()
{
  // start serial port
  Serial.begin(115200);
  delay(2000);
  Serial.print("Serial Starting...\n");

  // set Flash button Port
  pinMode(flashButtonPin, INPUT);

  // set LED port
  pinMode(ledPin, OUTPUT);

  // LED on while connecting to WLAN
  digitalWrite(ledPin, 0);

  // Start WiFiMnager
  wifiManagerStart();

  // LED off after connecting to WLAN
  digitalWrite(ledPin, 1);

  // start FastLED port
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, numLeds);

  // set up UDP receiver
  Udp.begin(ART_NET_PORT);

  // setup Torch
  resetEnergy();

  // Set DMX channel and universe
  dmxChannel = WiFiDmxChannel.toInt();
  universe = WiFiUniverse.toInt();

  //send message that setup is completed
  Serial.println("\nLeaving setup,\nEntering Main loop...");
}


void loop()
{
 // get Art-Net
 recieveUdp();

 // prepare Torch animation
 EVERY_N_MILLISECONDS(cycle_wait)
 {
 injectRandom();
 calcNextEnergy();
 calcNextColors();
 }

 // display on WS2812
 FastLED.show();

 if (!digitalRead(flashButtonPin)==HIGH)
 {
    Serial.println("Flash Button Pressed\n");
    WiFiManager wifiManager;
    wifiManager.startConfigPortal("OnDemandAP");
 }

 EVERY_N_MILLISECONDS(2000)
 {
    // Serial.println(".");
    digitalWrite(ledPin, 0);  // LED on
    delay(2);
    digitalWrite(ledPin, 1);  // LED off
 }
}
