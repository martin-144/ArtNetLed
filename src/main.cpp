#define FASTLED_ALLOW_INTERRUPTS 0
#define FASTLED_ESP8266_RAW_PIN_ORDER

#include <FastLED.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <torch.h>
#include <wifi.h>
#include <string>


// ARTNET CODES
struct artnet_t {
  const uint8_t data = 0x50;
  const uint8_t poll = 0x20;
  const uint8_t poll_reply = 0x21;
  const uint8_t header_size = 17;
} artnet;


/* replaced by above
const uint8_t artnet_data = 0x50;
const uint8_t artnet_poll = 0x20;
const uint8_t artnet_poll_reply = 0x21;
const uint8_t artnet_header = 17;
*/

// UDP settings
// const uint16_t udp_port = 6454;
WiFiUDP Udp;



// Local fastLed Ports
#define LED_PIN D8

// Blue LED blink interval
const int interval = 500;
const int ledPin = 0x02;

IPAddress broadcastIp(255 ,255 ,255 ,255);

void recieveUdp()
/*
Linux command to test:
echo -n "Test-Command" | nc -u -w0 192.168.178.31 6454
*/
{
  byte packet[18 + (numLeds * 3)];
  byte header[artnet.header_size];

  int packetSize = Udp.parsePacket();

  //test if a packet has been recieved
  if (packetSize)
  {
    digitalWrite(ledPin, 0);  // Light Led when receiving UDP data
    // Print received byte
    // Serial.printf("Received %d bytes from %s, port %d\n", packetSize, Udp.remoteIP().toString().c_str(), Udp.remotePort()); // For Debug

    // Read Artnet Header
    int len = Udp.read(header, artnet.header_size + 1);

    // Test for empty packet, if empty return
    if(len < 1)
      return;

    Serial.printf("%s\n", header); // For Debug

    if (header[9] == 0x20) // ArtNet OpPoll received
    {
      Serial.printf("ArtNet OpPoll received\n");
      Serial.println(Udp.remoteIP());
      Serial.println(Udp.remotePort());

      Udp.beginPacket(broadcastIP, Udp.remotePort());
      // Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
      Udp.print("Art-Net"); // + ... to be continued
      Udp.endPacket();
    }

    if (header[9] == 0x50) // ArtNet Data received
    {
      Serial.printf("ArtNet Net data received, Universe %d\n", header[15]);
    }

    /*
    Serial.printf("%s\n", header); // For Debug
    Serial.printf("0x%x, ", header[8]);
    Serial.printf("0x%x, ", header[9]);IPAddress broadcastIp(255,255,255,255);
    Serial.printf("0x%x\n", hUdp.remoteIP()eader[15]);
    */

    // Read-in packet and get length
    Udp.read(packet, 8);
    Serial.printf("%x\n", packet); // For Debug

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

// Main program
// ============

void setup()
{
  // start serial port
  Serial.begin(115200);
  delay(2000);IPAddress broadcastIp(255,255,255,255);
  Serial.print("Serial Starting...\n");

  // set LED port
  pinMode(ledPin, OUTPUT);

  // LED on while connecting to WLAN
  digitalWrite(ledPin, 0);

  // connect to WiFi network
  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);
  // WiFi.config(ip, gateway, subnet);

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

  delay(1000);
  Serial.print("Connected to: ");
  Serial.println(WiFi.SSID());
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // LED off after connecting to WLAN
  digitalWrite(ledPin, 1);

  // set up UDP receiver
  Udp.begin(6454);

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
 recieveUdp();

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
    // Serial.println(".");
    digitalWrite(ledPin, 0);  // LED on
    delay(2);
    digitalWrite(ledPin, 1);  // LED off
 }
}
