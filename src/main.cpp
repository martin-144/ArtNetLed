#define FASTLED_ALLOW_INTERRUPTS 0
#define FASTLED_ESP8266_RAW_PIN_ORDER

#include <FastLED.h>
#include <ESP8266WiFi.h>
#include <artnet.h>
#include <WiFiUdp.h>
#include <torch.h>
#include <wifi.h>
#include <string>

struct art_poll_reply_s ArtPollReply;

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
  byte header[ART_DMX_START]; // Buffer for Artnet Header

  int packetSize = Udp.parsePacket();

  //test if a packet has been recieved
  if (packetSize)
  {
    digitalWrite(ledPin, 0);  // Light Led when receiving UDP data
    // Print received byte
    // Serial.printf("Received %d bytes from %s, port %d\n", packetSize, Udp.remoteIP().toString().c_str(), Udp.remotePort()); // For Debug

    // Read Artnet Header
    int len = Udp.read(header, ART_DMX_START);

    // Test for empty packet, if empty return
    if(len < 1)
      return;

    if (header[9] == 0x20) // ArtNet OpPoll received
    {
      Serial.printf("ArtNet OpPoll received from ");
      Serial.print(Udp.remoteIP());
      Serial.printf(" Port ");
      Serial.println(Udp.remotePort());

      IPAddress local_ip = WiFi.localIP();

      node_ip_address[0] = local_ip[0];
      node_ip_address[1] = local_ip[1];
      node_ip_address[2] = local_ip[2];
      node_ip_address[3] = local_ip[3];

      sprintf((char *)id, "Art-Net");
      memcpy(ArtPollReply.id, id, sizeof(ArtPollReply.id));
      memcpy(ArtPollReply.ip, node_ip_address, sizeof(ArtPollReply.ip));

      ArtPollReply.opCode = ART_POLL_REPLY;
      ArtPollReply.port =  ART_NET_PORT;

      memset(ArtPollReply.goodinput, 0x08, numports);
      memset(ArtPollReply.goodoutput, 0x80, numports);
      memset(ArtPollReply.porttypes, 0x00, 1);
      memset(ArtPollReply.porttypes, 0xc0, 1);

      uint8_t shortname [18] = {0};
      uint8_t longname [64] = {0};
      sprintf((char *)shortname, "ArtNet Torch");
      sprintf((char *)longname, "Art-Net -> Arduino Bridge");
      memcpy(ArtPollReply.shortname, shortname, sizeof(shortname));
      memcpy(ArtPollReply.longname, longname, sizeof(longname));

      ArtPollReply.estaman[0] = 0;
      ArtPollReply.estaman[1] = 0;
      ArtPollReply.verH       = 1;
      ArtPollReply.ver        = 0;
      ArtPollReply.subH       = 0;
      ArtPollReply.sub        = 0;
      ArtPollReply.oemH       = 0;
      ArtPollReply.oem        = 0xFF;
      ArtPollReply.ubea       = 0;
      ArtPollReply.status     = 0xd0;
      ArtPollReply.swvideo    = 0;
      ArtPollReply.swmacro    = 0;
      ArtPollReply.swremote   = 0;
      ArtPollReply.style      = 0;

      ArtPollReply.numports[1] = (numports >> 8) & 0xff;
      ArtPollReply.numports[0] = numports & 0xff;
      ArtPollReply.status2   = 0x08;

      ArtPollReply.bindip[0] = node_ip_address[0];
      ArtPollReply.bindip[1] = node_ip_address[1];
      ArtPollReply.bindip[2] = node_ip_address[2];
      ArtPollReply.bindip[3] = node_ip_address[3];

      uint8_t swin[4]  = {0x01,0x02,0x03,0x04};
      uint8_t swout[4] = {0x01,0x02,0x03,0x04};

      for(uint8_t i = 0; i < numports; i++)
      {
          ArtPollReply.swout[i] = swout[i];
          ArtPollReply.swin[i] = swin[i];
      }

      sprintf((char *)ArtPollReply.nodereport, "%d DMX output universes active.", numports);

      Serial.println("Sending ArtNet OpPollReply Packet...");

      Udp.beginPacket(broadcast, ART_NET_PORT); //send ArtNet OpPollReply
      Udp.write((uint8_t *)&ArtPollReply, sizeof(ArtPollReply));
      Udp.endPacket();
    }

    // test for Art-Net DMX packet
	  // (packet[14] & packet[15] are the low and high bytes for universe
    if(header[9] == 0x50)
    {
      Serial.printf("ArtNet data received, Universe %d\n", header[14]);
      byte packet[32]; // Buffer for Artnet Packet

      Udp.read(packet, 32);
      // Serial.printf("%s\n", packet); // For Debug
      int dmx = 0; //ART_DMX_START + dmx_channel - 1;

      brightness = packet[dmx];
      artnet_levels = packet[dmx+1] * levels / 256;

      Serial.printf("Brightness: %d, Level: %d\n", brightness, artnet_levels);

      // Torch completely off when artnet_levels < 1
      if (artnet_levels < 1)
         brightness = 0;

      digitalWrite(ledPin, 1);  // Unlight LED
    }
  }
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

  Serial.print("Connected to: ");
  Serial.println(WiFi.SSID());
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // LED off after connecting to WLAN
  digitalWrite(ledPin, 1);

  // set up UDP receiver
  Udp.begin(ART_NET_PORT);

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
