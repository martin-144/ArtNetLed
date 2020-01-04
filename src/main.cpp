#define FASTLED_ALLOW_INTERRUPTS 0
#define FASTLED_ESP8266_RAW_PIN_ORDER

#include <spiffs.h>
#include <wifimanager.h>
#include <FastLED.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <EasyButton.h>
#include <artnet.h>
#include <torch.h>
#include <string>

// UDP settings
// const uint16_t udp_port = 6454;
WiFiUDP Udp;

// WiFiManager
WiFiManager wifiManager;

// Local fastLed Ports
const uint8_t fastLedPin = D8;

// Blue LED blink interval
const int interval = 500;
const int ledPin = 0x02;

// Flash button
const int flashButtonPin = 0x00;

// set Flash button for EasyButton
EasyButton flashButton(flashButtonPin);

IPAddress broadcastIp(255, 255, 255, 255);

// Callback function to be called when the button is pressed.
void onPressedForDuration() {
  Serial.println("Flash button has been pressed for 4 seconds.");
  Serial.println("Resetting ESP...");

  // LED on while connecting to WLAN
  digitalWrite(ledPin, 0);

  wifiManager.resetSettings();
  ESP.reset();
  ESP.restart();

  // LED off after connecting to WLAN
  digitalWrite(ledPin, 1);
}

// Main program
// ============

void setup()
{
  // start serial port
  Serial.begin(115200);
  delay(2000);
  Serial.print("Serial Starting...\n");

  // pinMode(flashButtonPin, INPUT);

  // Add the callback function to be called when the button is pressed for at least the given time.
  flashButton.onPressedFor(4000, onPressedForDuration);

  // set LED port
  pinMode(ledPin, OUTPUT);

  // LED on while connecting to WLAN
  digitalWrite(ledPin, 0);

  // Start WiFiMnager
  wifiManagerStart();

  // Set DMX channel and universe
  // artnet.dmxChannel_int = artnet.dmxChannel.toInt();
  // artnet.universe_int = artnet.universe.toInt();

  WiFi.mode(WIFI_STA);

  // LED off after connecting to WLAN
  digitalWrite(ledPin, 1);

  // start FastLED port
  FastLED.addLeds<WS2812, fastLedPin, GRB>(leds, numLeds);

  // set up UDP receiver
  Udp.begin(ART_NET_PORT);

  // setup Torch
  resetEnergy();

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

  flashButton.read();

  EVERY_N_MILLISECONDS(2000)
  {
    // Serial.println(".");
    digitalWrite(ledPin, 0);  // LED on
    delay(2);
    digitalWrite(ledPin, 1);  // LED off
  }
}
