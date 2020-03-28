#define FASTLED_ALLOW_INTERRUPTS 0
#define FASTLED_ESP8266_RAW_PIN_ORDER

#include <spiffs.h>
#include <wifimanager.h>
#include <FastLED.h>
#include <artnet.h>
#include <torch.h>
#include <effects.h>
#include <EasyButton.h>

// Local fastLed Port
const uint8_t fastLedPin = D8; // Equals 0x0f

// Blue LED Port
const uint8_t ledPin = D4; // Equals 0x02

// Flash button on NodeMCU
const uint8_t flashButtonPin = D3; // Equals 0x00

// set Flash button for EasyButton
EasyButton flashButton(flashButtonPin);

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
}

// Main program
// ============

void setup()
{
  // start serial port
  Serial.begin(115200);
  delay(2000);
  Serial.print("Serial Starting...\n");

  // set LED port
  pinMode(ledPin, OUTPUT);

  // LED on while connecting to WLAN
  digitalWrite(ledPin, 0);

  /* Find out which board we use */
  // If D5 == true, we have the WEMOS
  // If D5 == false, we have the nodeMCU
  // Therefore we activate the pullup on D5 and set a jumper from D5 to GND on the NodeMCU board
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

  // Start WiFiManager
  wifiManagerStart();

  // LED off after connecting to WLAN
  digitalWrite(ledPin, 1);

  // Add the callback function to be called when the button is pressed for at least the given time.
  flashButton.onPressedFor(4000, onPressedForDuration);

  // set up UDP
  Udp.begin(ART_NET_PORT);


  // start FastLED port
  FastLED.addLeds<WS2812, fastLedPin, GRB>(leds, numLeds);

  // disable FastLED dither to prevent flickering
  FastLED.setDither(0);

  // setup Torch
  resetEnergy();

  //send message that setup is completed
  Serial.println("\nLeaving setup,\nEntering Main loop...");
}

void loop()
{
  // Serial.println("Loop");
  /* get Art-Net Data */
  recieveUdp();

  // prepare Torch animation
  EVERY_N_MILLISECONDS_I(animationTimer, 20)
  {
    animationTimer.setPeriod(255 - artnetTorchParams.speed);

    switch(artnetTorchParams.effect)
    {
      case 0 ... 19:
        // Serial.println("Effect: No Effect");
        FastLED.clear();
        break;

      case 20 ... 39:
        // Serial.println("Effect: Static color");
        setAll(artnetTorchParams.colorRGB);
        break;

      case 40 ... 59:
        // This is all Torch Stuff
        // param1 = fadeheight
        // Serial.println("Effect: Torch");
        injectRandom();
        // These functions must be called in this sequence, other makes problems
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
        // Serial.println("Effect: MeteorRainRainbow");
        meteorRainRainbow(artnetTorchParams.colorRGB, artnetTorchParams.param1);
        break;

      case 120 ... 139:
        // Serial.println("Effect: SparkleUp");
        sparkleUp(artnetTorchParams.colorRGB, artnetTorchParams.param1);
        break;

      case 140 ... 159:
        // Serial.println("Effect: Confetti");
        confetti(artnetTorchParams.colorRGB, artnetTorchParams.param1);
        break;

      case 160 ... 179:
        // Serial.println("Effect: Static Rainbow");
        staticRainbow(artnetTorchParams.param1);
        break;

      case 180 ... 199:
        // Serial.println("Effect: Juggle");
        juggle(artnetTorchParams.colorRGB, artnetTorchParams.param1);
        break;

      default:
        // Serial.println("Effect: Not Used");
        setAll(CRGB(20, 0, 0));
        break;
    }
  }

    EVERY_N_MILLISECONDS(10)
  {
    /* Read Flash button */
    flashButton.read();

    /* Add sparkle */
    if(artnetTorchParams.param2)
    {
      sparkle(CRGB(255,255,255).nscale8(artnetTorchParams.param2));
    }

    FastLED.setBrightness(artnetTorchParams.brightness);
    FastLED.show();
  }

  EVERY_N_MILLISECONDS(2000)
  {
    /* Debug Stuff... */
    // WiFi.printDiag(Serial);
    // Serial.printf("*Memory [Free heap] %d\n", ESP.getFreeHeap());
    // Serial.printf("*Memory [Free stack] %d\n", ESP.getFreeContStack());

    /* Blink LED */
    digitalWrite(ledPin, 0);  // LED on
    delay(2);  // is this ms or seconds? It is ms.
    digitalWrite(ledPin, 1);  // LED off
  }
}
