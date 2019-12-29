#ifndef ARDUINO_ARTNET_H
#define ARDUINO_ARTNET_H

#include <inttypes.h>
#include <ESP8266WiFi.h>

// UDP specific
#define ART_NET_PORT 6454
// Opcodes
#define ART_POLL 0x2000
#define ART_POLL_REPLY 0x2100
#define ART_DMX 0x5000
#define ART_SYNC 0x5200
// Buffers
#define MAX_BUFFER_ARTNET 530
// Packet
#define ART_NET_ID "Art-Net\0"
#define ART_DMX_START 17

extern WiFiUDP Udp;
extern uint8_t brightness;
extern uint16_t dmxChannel;
extern const int ledPin;
extern const uint8_t levels;
extern const uint8_t dimmingLevel;

uint8_t artnetPacket[MAX_BUFFER_ARTNET];
uint16_t packetSize;
IPAddress broadcast = {255, 255, 255, 255};
IPAddress remoteIP;
uint16_t opcode;
uint8_t universe;
uint16_t dmxChannel = 1;
uint16_t dmxLength;
uint16_t dmxDataLength;
uint8_t sequence;
uint8_t node_ip_address[4];
uint8_t id[8];
uint8_t numports = 1;
uint8_t artnetLevels;
uint8_t artnetLevelsRaw;

struct art_poll_reply_s {
  uint8_t  id[8];
  uint16_t opCode;
  uint8_t  ip[4];
  uint16_t port;
  uint8_t  verH;
  uint8_t  ver;
  uint8_t  subH;
  uint8_t  sub;
  uint8_t  oemH;
  uint8_t  oem;
  uint8_t  ubea;
  uint8_t  status;
  uint8_t  estaman[2];
  uint8_t  shortname[18];
  uint8_t  longname[64];
  uint8_t  nodereport[64];
  uint8_t  numports[2];
  uint8_t  porttypes[4]; //max of 4 ports per node
  uint8_t  goodinput[4] = {0};
  uint8_t  goodoutput[4] = {0};
  uint8_t  swin[4] = {0};
  uint8_t  swout[4] = {0};
  uint8_t  swvideo;
  uint8_t  swmacro;
  uint8_t  swremote;
  uint8_t  sp1;
  uint8_t  sp2;
  uint8_t  sp3;
  uint8_t  style;
  uint8_t  mac[6];
  uint8_t  bindip[4];
  uint8_t  bindindex;
  uint8_t  status2;
  uint8_t  filler[26];
} __attribute__((packed));

struct art_poll_reply_s ArtPollReply;

void recieveUdp()
/*
Linux command to test:
echo -n "Test-Command" | nc -u -w0 192.168.178.31 6454
*/
{

  int packetSize = Udp.parsePacket();

  //test if a packet has been recieved
  if (packetSize)
  {
    digitalWrite(ledPin, 0);  // Light Led when receiving UDP data
    // Print received byte
    // Serial.printf("Received %d bytes from %s, port %d\n", packetSize, Udp.remoteIP().toString().c_str(), Udp.remotePort()); // For Debug

    // Read Artnet Header
    int len = Udp.read(artnetPacket, MAX_BUFFER_ARTNET);

    // Test for empty packet, if empty return
    if(len < 1)
      return;

    // Check that packetID is "Art-Net" else ignore
    for (byte i = 0 ; i < 8 ; i++)
    {
      if (artnetPacket[i] != ART_NET_ID[i])
        return;
    }

    opcode = artnetPacket[8] | artnetPacket[9] << 8;
    universe = artnetPacket[14] | artnetPacket[15] << 8;
    dmxLength = artnetPacket[17] | artnetPacket[16] << 8;

    if (opcode == ART_POLL) // ArtNet OpPoll received
    {
      Serial.printf("ArtNet OpPoll received from ");
      Serial.print(Udp.remoteIP());
      Serial.printf(":");
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

    if(opcode == ART_DMX) // Test for Art-Net DMX packet
    {
      Serial.printf("ArtNet data received, Universe %d, DMX length %d\n", universe, dmxLength);

      brightness = artnetPacket[ART_DMX_START + dmxChannel];
      artnetLevelsRaw = artnetPacket[ART_DMX_START + dmxChannel + 1];
      artnetLevels = artnetLevelsRaw * levels / 255;

      // Dim Torch when <= artnet_levels
      if (artnetLevels <= dimmingLevel)
      {
      // Serial.printf("levels = %d, dimming_level = %d\n", levels, dimming_level);
      // Serial.printf("Dimming Level = %d\n", map(artnet_levels_raw, 0, ledsPerLevel * dimmingLevel, 0, 255));
      // brightness = map(artnetLevelsRaw, 0, 60, 0, brightness);
      brightness = map(artnetLevelsRaw, 0, 60, 0, brightness); // why is it 60 ?
      }

      Serial.printf("Brightness: %d, Torch Level: %d, Torch Level Raw: %d\n", brightness, artnetLevels, artnetLevelsRaw);
      digitalWrite(ledPin, 1);  // Unlight LED
    }
  }
}

#endif // ARDUINO_ARTNET_H
