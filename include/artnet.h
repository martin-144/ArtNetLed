#ifndef ARDUINO_ARTNET_H
#define ARDUINO_ARTNET_H

#include <inttypes.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <torch.h>

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
extern const int ledPin;
extern const uint8_t levels;
extern const uint8_t dimmingLevel;

struct artnet_dmx_params_s {
  IPAddress broadcast = {255, 255, 255, 255};
  uint16_t opcode;
  uint16_t universe;
  uint16_t listenUniverse;
  uint16_t dmxLength;
  uint16_t dmxChannel = 1;
  uint16_t dmxDataLength;
  uint16_t numports = 1;
  uint8_t packet[MAX_BUFFER_ARTNET];
  uint8_t node_ip_address[4];
  uint8_t id[8];
};
struct artnet_dmx_params_s artnet;

struct artnet_torch_params_s {
  uint8_t brightness;
  CRGB colorRGB;
  uint8_t effect;
  uint8_t speed;
  uint8_t param1;
  uint8_t param2;
};
struct artnet_torch_params_s artnetTorchParams;

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
struct art_poll_reply_s artPollReply;

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
    int len = Udp.read(artnet.packet, MAX_BUFFER_ARTNET);

    // Test for empty packet, if empty return
    if(len < 1)
    return;

    // Check that packetID is "Art-Net" else ignore
    for (byte i = 0 ; i < 8 ; i++)
    {
      if (artnet.packet[i] != ART_NET_ID[i])
      return;
    }

    artnet.opcode = artnet.packet[8] | artnet.packet[9] << 8;
    artnet.universe = artnet.packet[14] | artnet.packet[15] << 8;
    artnet.dmxLength = artnet.packet[17] | artnet.packet[16] << 8;

    if (artnet.opcode == ART_POLL) // ArtNet OpPoll received
    {
      Serial.print("ArtNet OpPoll received from ");
      Serial.print(Udp.remoteIP());
      Serial.print(":");
      Serial.println(Udp.remotePort());

      IPAddress local_ip = WiFi.localIP();

      artnet.node_ip_address[0] = local_ip[0];
      artnet.node_ip_address[1] = local_ip[1];
      artnet.node_ip_address[2] = local_ip[2];
      artnet.node_ip_address[3] = local_ip[3];

      sprintf((char *)artnet.id, "Art-Net");
      memcpy(artPollReply.id, artnet.id, sizeof(artPollReply.id));
      memcpy(artPollReply.ip, artnet.node_ip_address, sizeof(artPollReply.ip));

      artPollReply.opCode = ART_POLL_REPLY;
      artPollReply.port =  ART_NET_PORT;

      memset(artPollReply.goodinput, 0x80, 4);
      memset(artPollReply.goodoutput, 0x80, 4);
      memset(artPollReply.porttypes, 0xc0, 4);

      uint8_t shortname [18] = {0};
      uint8_t longname [64] = {0};
      sprintf((char *)shortname, "ArtNet Torch");
      sprintf((char *)longname, "Art-Net -> Arduino Bridge");
      memcpy(artPollReply.shortname, shortname, sizeof(shortname));
      memcpy(artPollReply.longname, longname, sizeof(longname));

      artPollReply.estaman[1] = 0x00;
      artPollReply.estaman[0] = 0x00;
      artPollReply.verH       = 1;
      artPollReply.ver        = 0;
      artPollReply.subH       = 0;
      artPollReply.sub        = 0;
      artPollReply.oemH       = 0;
      artPollReply.oem        = 0xFF;
      artPollReply.ubea       = 0;
      artPollReply.status     = 0xd0;
      artPollReply.swvideo    = 0;
      artPollReply.swmacro    = 0;
      artPollReply.swremote   = 0;
      artPollReply.style      = 0;
      artPollReply.numports[0] = (artnet.numports >> 8) & 0xff;
      artPollReply.numports[1] = artnet.numports & 0xff;
      artPollReply.status2   = 0x08;

      artPollReply.bindip[0] = artnet.node_ip_address[0];
      artPollReply.bindip[1] = artnet.node_ip_address[1];
      artPollReply.bindip[2] = artnet.node_ip_address[2];
      artPollReply.bindip[3] = artnet.node_ip_address[3];

      uint8_t swin[4]  = {0x01, 0x02, 0x03, 0x03};
      uint8_t swout[4] = {0x01, 0x02, 0x03, 0x04};

      for(uint8_t i = 0; i < 4; i++)
      {
        artPollReply.swout[i] = swout[i];
        artPollReply.swin[i] = swin[i];
      }

      sprintf((char *)artPollReply.nodereport, "%d DMX output universes active.", artnet.numports);

      Serial.println("Sending ArtNet OpPollReply Packet...");

      Udp.beginPacket(artnet.broadcast, ART_NET_PORT); //send ArtNet OpPollReply
      Udp.write((uint8_t *)&artPollReply, sizeof(artPollReply));
      Udp.endPacket();
    }

    else if(artnet.opcode == ART_DMX) // Test for Art-Net DMX packet
    {
      Serial.printf("ArtNet data received, Universe %d, DMX length %d. ", artnet.universe, artnet.dmxLength);
      Serial.printf("Listening on Universe %d, DMX Channel %d.\n", artnet.listenUniverse, artnet.dmxChannel);

      if(artnet.universe != artnet.listenUniverse)
      return;
      /* Artnet Channel mapping:
         1: brightness;
         2,3,4: colorRGB;
         5: effect;
         6: speed;
         7: param1;
         8: param2;
      */

      artnetTorchParams.brightness = artnet.packet[ART_DMX_START + artnet.dmxChannel];
      artnetTorchParams.colorRGB = CRGB(artnet.packet[ART_DMX_START + artnet.dmxChannel + 1],
                                        artnet.packet[ART_DMX_START + artnet.dmxChannel + 2],
                                        artnet.packet[ART_DMX_START + artnet.dmxChannel + 3]);
      artnetTorchParams.effect = artnet.packet[ART_DMX_START + artnet.dmxChannel + 4];
      artnetTorchParams.speed = artnet.packet[ART_DMX_START + artnet.dmxChannel + 5];
      artnetTorchParams.param1 = artnet.packet[ART_DMX_START + artnet.dmxChannel + 6];
      artnetTorchParams.param2 = artnet.packet[ART_DMX_START + artnet.dmxChannel + 7];

      digitalWrite(ledPin, 1);  // Unlight LED
    }
  }
}

#endif // ARDUINO_ARTNET_H
