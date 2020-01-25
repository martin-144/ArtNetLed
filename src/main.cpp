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
#include <effects.h>
#include <string>

// UDP settings
// const uint16_t udp_port = 6454;
WiFiUDP Udp;

// WiFiManager
WiFiManager wifiManager;

// Local fastLed Ports
const uint8_t fastLedPin = D8; // Equals 0x0f

// Blue LED blink interval
const int interval = 500;
const int ledPin = D4; // Equals 0x02

// Flash button
const int flashButtonPin = 0x00;

// set Flash button for EasyButton
EasyButton flashButton(flashButtonPin);

IPAddress broadcastIp(255, 255, 255, 255);

// Callback function to be called when the button is pressed.
void onPressedForDuration()
{
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

  // Find out which board we use
  // If D5 == true, we have the WEMOS
  // If D5 == false, we have the nodeMCU

  pinMode(D5, INPUT_PULLUP);

  if(digitalRead(D5) == true)
  {
    // ledsPerLevel = 14;
    // levels = 28;
    Serial.printf("Found WEMOS Board, assuming ledsPerLevel = %d, levels = %d\n", ledsPerLevel, levels);
  }
  else
  {
    // ledsPerLevel = 8;
    // levels = 26;
    Serial.printf("Found NodeMCU Board, assuming ledsPerLevel = %d, levels = %d\n", ledsPerLevel, levels);
  }

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

  // disable FastLED dither to prevent flickering
  // FastLED.setDither(0);

  //send message that setup is completed
  Serial.println("\nLeaving setup,\nEntering Main loop...");

}

void loop()
{
  // get Art-Net
  recieveUdp();

  // prepare Torch animation
  EVERY_N_MILLISECONDS_I(animationTimer, 20)
  {
    animationTimer.setPeriod(255 - artnetTorchParams.speed);

    switch(artnetTorchParams.effect)
    {

      case 0 ... 19:
        Serial.println("Effect: No Effect");
        FastLED.clear();
        break;

      case 20 ... 39:
        // Serial.println("Effect: Static color");
        setAll(artnetTorchParams.colorRGB);
        break;

      case 40 ... 59:
        // This is all Torch Stuff
        // param1 = fadeheight
        // Serial.println("Effect: Fire");
        injectRandom();
        calcNextEnergy(artnetTorchParams.param1);
        calcNextColors(artnetTorchParams.colorRGB);
        break;

      case 60 ... 79:
        // Serial.println("Effect: MeteorRain");
        meteorRain(artnetTorchParams.colorRGB, 10, 48, true);
        break;

      case 80 ... 99:
        // Serial.println("Effect: MeteorRainRows");
        meteorRainRows(artnetTorchParams.colorRGB, artnetTorchParams.param1);
        break;

      case 100 ... 119:
        // Serial.println("Effect: Confetti");
        confetti(artnetTorchParams.colorRGB, artnetTorchParams.param1);
        break;

      case 120 ... 139:
        // Serial.println("Effect: Static Rainbow");
        staticRainbow(artnetTorchParams.param1);
        break;

      case 140 ... 159:
        // Serial.println("Effect: Plasma");
        plasma(artnetTorchParams.param1);
        break;

      default:
        Serial.println("Effect: Not Used");
        setAll(CRGB(20, 0, 0));
        break;
    }
  }

  EVERY_N_MILLISECONDS(10)
  {
    // Add sparkle
    if(artnetTorchParams.param2)
    {
      sparkle(CRGB(255,255,255).nscale8(artnetTorchParams.param2));
    }
  }

  EVERY_N_MILLISECONDS(10)
  {
    FastLED.setBrightness(artnetTorchParams.brightness);
    FastLED.show();
  }

  flashButton.read();

  EVERY_N_MILLISECONDS(2000)
  {
    // Serial.println(".");
    digitalWrite(ledPin, 0);  // LED on
    delay(2);
    digitalWrite(ledPin, 1);  // LED off

    nblendPaletteTowardPalette(currentPalette, targetPalette, 24);   // AWESOME palette blending capability.
    uint8_t baseC = random8();                                       // You can use this as a baseline colour if you want similar hues in the next line.
    targetPalette = CRGBPalette16(CHSV(baseC+random8(32), 192, random8(128,255)), CHSV(baseC+random8(32), 255, random8(128,255)), CHSV(baseC+random8(32), 192, random8(128,255)), CHSV(baseC+random8(32), 255, random8(128,255)));
  }
}
