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
#define ART_IP_PROG 0xf800
#define ART_ADDRESS 0x6000


// Buffers
#define MAX_BUFFER_ARTNET 530

// Packet
#define ART_NET_ID "Art-Net\0"
#define ART_DMX_START 17

WiFiUDP Udp;

extern const uint8_t ledPin;

struct artnet_dmx_params_s {
  // IPAddress broadcast = {255, 255, 255, 255};
  IPAddress unicast;
  uint16_t opcode;
  uint16_t universe;
  uint16_t listenUniverse;
  uint16_t dmxChannel;
  uint16_t numports;
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
  uint8_t  ver[2];
  uint8_t  sub[2];
  uint8_t  oem[2];
  uint8_t  ubea;
  uint8_t  status1;
  uint8_t  estaman[2];
  uint8_t  shortname[18];
  uint8_t  longname[64];
  uint8_t  nodereport[64];
  uint8_t  numports[2]; // Don't change it here! Here is Number of Bytes.
  uint8_t  porttypes[4]; //max of 4 ports per node
  uint8_t  goodinput[4];
  uint8_t  goodoutput[4];
  uint8_t  swin[4];
  uint8_t  swout[4];
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

void sendArtDmxReply(void);
void receiveArtDmx(void);

void recieveUdp()
/*
Linux command to test:
echo -n "Test-Command" | nc -u -w0 192.168.178.31 6454
*/
{
// Serial.println("receiveUDP");
int packetSize = Udp.parsePacket();

    // Test if a packet has been recieved
    if(packetSize)
    {
    // Print received byte
    // if(packetSize != 530)
    // Serial.printf("Received %d bytes from %s, port %d\n", packetSize, Udp.remoteIP().toString().c_str(), Udp.remotePort()); // For Debug

    // Read Artnet Header
    Udp.read(artnet.packet, MAX_BUFFER_ARTNET);

    // Check that packetID is "Art-Net" else return
    for (byte i = 0 ; i < 8 ; i++)
    {
      if (artnet.packet[i] != ART_NET_ID[i])
      return;
    }

    artnet.opcode = artnet.packet[8] | artnet.packet[9] << 8;

    digitalWrite(ledPin, 0);  // Light Led when receiving Artnet data

    switch(artnet.opcode)
    {
    case ART_POLL: // ArtNet OpPoll received
      sendArtDmxReply();
      Serial.print("*ArtNet [OpPoll] received from ");
      Serial.print(Udp.remoteIP());
      Serial.print(":");
      Serial.print(Udp.remotePort());
      Serial.print(", sent [OpPollReply] packet to ");
      Serial.print(artnet.unicast);
      Serial.print(":");
      Serial.println(Udp.remotePort());
      break;

    case ART_DMX: // Artnet OpDmx packet received
      static int num;
      EVERY_N_SECONDS(1)
      {
        Serial.printf("*ArtNet [%d OpDmx] packets received from " , num);
        Serial.print(Udp.remoteIP());
        Serial.print(":");
        Serial.print(Udp.remotePort());
        Serial.printf(", Universe %d. ", artnet.universe);
        Serial.printf("Listening on Universe %d, DMX Channel %d.\n", artnet.listenUniverse, artnet.dmxChannel);
        num = 0;
      }
      receiveArtDmx();
      num++;
      break;

    case ART_IP_PROG: // Artnet OpIpProg packet received
      Serial.printf("*ArtNet [OpIpProg] packet received, OpCode %#06x, ", artnet.opcode);
      Serial.printf("NOT IMPLEMENTED.\n");
      break;

    case ART_ADDRESS: // Artnet OpAdress packet received
      Serial.printf("*ArtNet [OpAddress] packet received, OpCode %#06x, ", artnet.opcode);
      Serial.printf("NOT IMPLEMENTED.\n");
      break;

    default: // Unknown Artnet Packed received
      Serial.printf("*ArtNet [Unknown] packet received, OpCode %#06x, ", artnet.opcode);
      Serial.printf("NOT IMPLEMENTED of course.\n");
      break;

    }
    digitalWrite(ledPin, 1);  // Unlight LED
  }
}

void receiveArtDmx(void)
{
  artnet.universe = artnet.packet[14] | artnet.packet[15] << 8;
  // artnet.dmxLength = artnet.packet[17] | artnet.packet[16] << 8;

  if(artnet.universe != artnet.listenUniverse)
  {
   return;
  }

  /* Artnet Channel mapping:
     1: brightness;
     2,3,4: colorRGB;
     5: effect;
     6: speed;
     7: param1;
     8: param2;
  */

  artnetTorchParams.brightness = artnet.packet[ART_DMX_START + artnet.dmxChannel];
  artnetTorchParams.colorRGB = CRGB (artnet.packet[ART_DMX_START + artnet.dmxChannel + 1],
                                     artnet.packet[ART_DMX_START + artnet.dmxChannel + 2],
                                     artnet.packet[ART_DMX_START + artnet.dmxChannel + 3]);
  artnetTorchParams.effect = artnet.packet[ART_DMX_START + artnet.dmxChannel + 4];
  artnetTorchParams.speed = artnet.packet[ART_DMX_START + artnet.dmxChannel + 5];
  artnetTorchParams.param1 = artnet.packet[ART_DMX_START + artnet.dmxChannel + 6];
  artnetTorchParams.param2 = artnet.packet[ART_DMX_START + artnet.dmxChannel + 7];
}

void sendArtDmxReply(void) {
  IPAddress local_ip = WiFi.localIP();

  artnet.node_ip_address[0] = local_ip[0];
  artnet.node_ip_address[1] = local_ip[1];
  artnet.node_ip_address[2] = local_ip[2];
  artnet.node_ip_address[3] = local_ip[3];

  artnet.unicast = Udp.remoteIP();

  sprintf((char *)artnet.id, "Art-Net");
  memcpy(artPollReply.id, artnet.id, sizeof(artPollReply.id));
  memcpy(artPollReply.ip, artnet.node_ip_address, sizeof(artPollReply.ip));

  // Serial.println(artnet.id);
  // Serial.println(artnet.node_ip_address);

  artPollReply.opCode = ART_POLL_REPLY;
  artPollReply.port = ART_NET_PORT;

  // memset(artPollReply.goodinput, 0x00, 4);
  // memset(artPollReply.goodoutput, 0x80, 4);
  // memset(artPollReply.porttypes, 0b01000101, 4);

  memset(artPollReply.goodinput, 0x80, 4);
  memset(artPollReply.goodoutput, 0x80, 4);
  memset(artPollReply.porttypes, 0x80, 4);

  uint8_t shortname[18] = {0};
  uint8_t longname[64] = {0};
  sprintf((char *)shortname, "ArtNet Torch");
  sprintf((char *)longname, "ArtNet Torch");
  memcpy(artPollReply.shortname, shortname, sizeof(shortname));
  memcpy(artPollReply.longname, longname, sizeof(longname));

  artPollReply.estaman[1] = 'K';
  artPollReply.estaman[0] = 'B';
  artPollReply.ver[1]       = 1;
  artPollReply.ver[0]       = 0;
  artPollReply.sub[1]       = 0;
  artPollReply.sub[0]       = 0;
  artPollReply.oem[1]       = 0;
  artPollReply.oem[0]       = 0;
  artPollReply.ubea         = 0;
  artPollReply.status1      = 0xd0;
  artPollReply.swvideo      = 0;
  artPollReply.swmacro      = 0;
  artPollReply.swremote     = 0;
  artPollReply.style        = 0;

  /*
  artPollReply.numports[0]  = (artnet.numports >> 8) & 0xff;
  artPollReply.numports[1]  = artnet.numports & 0xff;
  */

  artPollReply.numports[0]  = 0;
  artPollReply.numports[1]  = 1;

  artPollReply.status2      = 0x08;

  artPollReply.bindip[0] = artnet.node_ip_address[0];
  artPollReply.bindip[1] = artnet.node_ip_address[1];
  artPollReply.bindip[2] = artnet.node_ip_address[2];
  artPollReply.bindip[3] = artnet.node_ip_address[3];

  uint8_t swin[4]  = {0}; // Each Hex number is the Universe the input will listen on
  uint8_t swout[4] = {0}; // Each Hex number is the Universe the output will listen on
  swout[0] = artnet.listenUniverse; // We set it here for channel 1
  swin[0] = artnet.listenUniverse; // We set it here for channel 1

  for(uint8_t i = 0; i < 4; i++)
  {
    artPollReply.swout[i] = swout[i];
    artPollReply.swin[i] = swin[i];
  }

  sprintf((char *)artPollReply.nodereport, "%d DMX output universes active.", artnet.numports);

  // Udp.read(artnet.packet, MAX_BUFFER_ARTNET);
  // Udp.flush();

  WiFiUDP UdpTx;
  UdpTx.begin(ART_NET_PORT);
  UdpTx.beginPacket(artnet.unicast, ART_NET_PORT); //send ArtNet OpPollReply
  UdpTx.write((uint8_t *)&artPollReply, sizeof(artPollReply));
  UdpTx.endPacket();
  UdpTx.stop();
}

#endif // ARDUINO_ARTNET_H
