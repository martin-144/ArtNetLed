#define FASTLED_ALLOW_INTERRUPTS 0
#define FASTLED_ESP8266_RAW_PIN_ORDER
#include <FastLED.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <torch.h>
#include <string>

// Wifi connection
const char* wifi_ssid[2] = { "KB-Light", "MCWLAN2" };
const char* wifi_pass[2] = { "kolping1963", "brechtelsbauerwpakey" };

// UDP settings
const uint16_t udp_port= 6454;
WiFiUDP Udp;

// ArtNet Settings
uint16_t artnet_levels;
uint8_t  dmx_channel = 1;

// Local fastLed Ports
#define LED_PIN D8
CRGB leds[numLeds];

// Blue LED blink interval
const int interval = 500;
const int ledPin = 0x02;


void RecieveUdp()
/*
Linux command to test:
echo -n "Test-Command" | nc -u -w0 192.168.178.31 6454
*/
{
  byte packet[18 + (numLeds * 3)];
  int packetSize = Udp.parsePacket();

  //test if a packet has been recieved
  if (packetSize)
  {
    digitalWrite(ledPin, 0);  // Light Led when receiving UDP data
    // Print received byte
    // Serial.printf("Received %d bytes from %s, port %d\n", packetSize, Udp.remoteIP().toString().c_str(), Udp.remotePort()); // For Debug

    // Read-in packet and get length
    int len = Udp.read(packet, 18 + (numLeds * 3));
    // Serial.printf("%s\n", packet); // For Debug

    //discard unread bytes
    Udp.flush();

    //test for empty packet
    if(len < 1)
      return;

    digitalWrite(ledPin, 1);  // Unlight LED

    //test for Art-Net DMX packet
	  //(packet[14] & packet[15] are the low and high bytes for universe
    if(packet[9] == 0x50)
    {
      // Udp.beginPacketMulticast(WiFi.localIP(), multicastAddress, multicastPort);
      int dmx = 18 + dmx_channel - 1;

      /*
      for(int n = 0; n < numLeds; n++)
      {
        leds[n] = CRGB(packet[dmx++], packet[dmx++], packet[dmx++]);
      }
      */

      brightness = packet[dmx];
      artnet_levels = packet[dmx+1] * levels / 256;

      // Torch completely off when artnet_levels < 1
      if (artnet_levels < 1)
         brightness = 0;


      // Serial.write(artnet_levels);

      //push led data - moved to loop() after ReceiveUDP()
      //FastLED.show();
    }
  }
}

void calcNextEnergy()
{
  int i = 0;
  for (int y=0; y<levels; y++) {
    for (int x=0; x<ledsPerLevel; x++) {
      uint8_t e = currentEnergy[i];
      uint8_t m = energyMode[i];
      switch (m) {
        case torch_spark: {
          // loose transfer up energy as long as the is any
          reduce(&e, spark_tfr, 0);
          // cell above is temp spark, sucking up energy from this cell until empty
          if (y<artnet_levels-1) {
            energyMode[i+ledsPerLevel] = torch_spark_temp;
          }
          // 20191206 Martin has introduced this for dynamic level control
          else if (y>artnet_levels-1) {
            energyMode[i+ledsPerLevel] = torch_passive;
          }
          break;
        }
        case torch_spark_temp: {
          // just getting some energy from below
          uint8_t e2 = currentEnergy[i-ledsPerLevel];
          if (e2<spark_tfr) {
            // cell below is exhausted, becomes passive
            energyMode[i-ledsPerLevel] = torch_passive;
            // gobble up rest of energy
            increase(&e, e2, 255);
            // loose some overall energy
            e = ((int)e*spark_cap)>>8;
            // this cell becomes active spark
            energyMode[i] = torch_spark;
          }
          else {
            increase(&e, spark_tfr, 255);
          }
          break;
        }
        case torch_passive: {
          e = ((int)e*heat_cap)>>8;
          increase(&e, ((((int)currentEnergy[i-1]+(int)currentEnergy[i+1])*side_rad)>>9) + (((int)currentEnergy[i-ledsPerLevel]*up_rad)>>8), 255);
        }
        default:
          break;
      }
      nextEnergy[i++] = e;
    }
  }
}


const uint8_t energymap[32] = {0, 64, 96, 112, 128, 144, 152, 160, 168, 176, 184, 184, 192, 200, 200, 208, 208, 216, 216, 224, 224, 224, 232, 232, 232, 240, 240, 240, 240, 248, 248, 248};

void calcNextColors()
{
  for (int i=0; i<numLeds; i++) {
      uint16_t e = nextEnergy[i];
      currentEnergy[i] = e;
  //    leds.setColorDimmed(i, 255, 170, 0, e);const __FlashStringHelper *
      if (e>250)
        setColorDimmed(leds, i, 170, 170, e, brightness);
      else {
        if (e>0) {
          // energy to brightness is non-linear
          uint8_t eb = energymap[e>>3];
          uint8_t r = red_bias;
          uint8_t g = green_bias;
          uint8_t b = blue_bias;
          increase(&r, (eb*red_energy)>>8, 255);
          increase(&g, (eb*green_energy)>>8, 255);
          increase(&b, (eb*blue_energy)>>8, 255);
          setColorDimmed(leds , i, r, g, b, brightness);
        }
        else {
          // background, no energy
          setColorDimmed(leds, i, red_bg, green_bg, blue_bg, brightness);
        }
      }
  }
}


void injectRandom()
{
  // random flame energy at bottom row
  for (int i=0; i<ledsPerLevel; i++) {
    currentEnergy[i] = torch_random(flame_min, flame_max);
    energyMode[i] = torch_nop;
  }
  // random sparks at second row
  for (int i=ledsPerLevel; i<2*ledsPerLevel; i++) {
    if (energyMode[i]!=torch_spark && torch_random(100, 0)<random_spark_probability) {
      currentEnergy[i] = torch_random(spark_min, spark_max);
      energyMode[i] = torch_spark;
    }
  }
}

// Utilities
// =========

void resetEnergy()
{
  for (int i=0; i<numLeds; i++) {
    currentEnergy[i] = 0;
    nextEnergy[i] = 0;
    energyMode[i] = torch_passive;
  }
}
// WiFi.begin("MCWLAN2", "brechtelsbauerwpakey");

uint8_t torch_random(uint8_t aMinOrMax, uint8_t aMax)
{
  if (aMax==0)
  {
    aMax = aMinOrMax;
    aMinOrMax = 0;
  }
  uint32_t r = aMinOrMax;
  aMax = aMax - aMinOrMax + 1;
  r += rand() % aMax;
  return r;
}


void reduce(uint8_t *aByte, uint8_t aAmount, uint8_t aMin)
{
  int r = *aByte-aAmount;
  if (r<aMin)
    *aByte = aMin;
  else
    *aByte = (uint8_t)r;
}


void increase(uint8_t *aByte, uint8_t aAmount, uint8_t aMax)
{
  int r = *aByte+aAmount;
  if (r>aMax)
    *aByte = aMax;
  else
   *aByte = (uint8_t)r;
}

void setColor(struct CRGB *leds, uint16_t aLedNumber, uint8_t aRed, uint8_t aGreen, uint8_t aBlue)
{
  if (aLedNumber>=numLeds) return; // invalid LED number
  // linear brightness is stored with 5bit precision only
  leds[aLedNumber].red = aRed;
  leds[aLedNumber].green = aGreen;
  leds[aLedNumber].blue = aBlue;
}

void setColorDimmed(struct CRGB *leds, uint16_t aLedNumber, uint8_t aRed, uint8_t aGreen, uint8_t aBlue, uint8_t aBrightness)
{
  setColor(leds, aLedNumber, (aRed*aBrightness)>>8, (aGreen*aBrightness)>>8, (aBlue*aBrightness)>>8);
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

  // connect to WiFi network
  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);


  while (WiFi.status() != WL_CONNECTED)
  {
    for (uint8_t i=0; i < sizeof(wifi_ssid)/sizeof(wifi_ssid)[0]; i++)
    {
    // connect to WiFi network
    Serial.printf("Trying to connect to %s, Password %s\n", wifi_ssid[i], wifi_pass[i]);
    WiFi.begin(wifi_ssid[i], wifi_pass[i]);
    delay(4000);
    }
 }

  Serial.println();
  Serial.print("Connected to: ");
  Serial.println(WiFi.SSID());
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // LED off after connecting to WLAN
  digitalWrite(ledPin, 1);

  // set up UDP receiver
  Udp.begin(udp_port);

  // start LED port
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, numLeds);

  // setup Torch
  resetEnergy();

  //send message that setup is completed
  Serial.println("\nLeaving setup,\nEntering Main loop...");
}

void loop()
{

 // get Art-Net
 RecieveUdp();

 // torch animation + text display + cheerlight background
 EVERY_N_MILLISECONDS(cycle_wait)
 {
 injectRandom();
 calcNextEnergy();
 calcNextColors();
 }

 // display on WS2812
 FastLED.show();

 EVERY_N_MILLISECONDS(2000)
 {
    Serial.println(".");
    digitalWrite(ledPin, 0);  // LED on
    delay(2);
    digitalWrite(ledPin, 1);  // LED off
 }
}
