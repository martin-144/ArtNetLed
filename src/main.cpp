#define FASTLED_ALLOW_INTERRUPTS 0
#define FASTLED_ESP8266_RAW_PIN_ORDER

#include <FS.h>
#include <FastLED.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>         // https://github.com/bblanchon/ArduinoJson
#include <WiFiUdp.h>
#include <artnet.h>
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

// Flash button
const int flashButtonPin = 0x00;

IPAddress broadcastIp(255 ,255 ,255 ,255);

// WiFiManager related Stuff
String WiFiUniverse;
String WiFiDmxChannel;

// flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}


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

      brightness = artnetPacket[ART_DMX_START];
      artnetLevelsRaw = artnetPacket[ART_DMX_START+1];
      artnetLevels = artnetLevelsRaw * levels / 255;

      // Dim Torch when <= artnet_levels
      if (artnetLevels <= dimmingLevel)
      {
      // Serial.printf("levels = %d, dimming_level = %d\n", levels, dimming_level);
      // Serial.printf("Dimming Level = %d\n", map(artnet_levels_raw, 0, ledsPerLevel * dimmingLevel, 0, 255));
      brightness = map(artnetLevelsRaw, 0, 60, 0, brightness);
      }

      Serial.printf("Brightness: %d, Torch Level: %d, Torch Level Raw: %d\n", brightness, artnetLevels, artnetLevelsRaw);
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

  // set Flash button Port
  pinMode(flashButtonPin, INPUT);

  // set LED port
  pinMode(ledPin, OUTPUT);

  // LED on while connecting to WLAN
  digitalWrite(ledPin, 0);

  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        // size_t size = configFile.size();

        Serial.printf("Size: %d\n", configFile.size());
        // Allocate a buffer to store contents of the file.
        // std::unique_ptr<char[]> buf(new char[size]);

        // configFile.readBytes(buf.get(), size);
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, configFile);
        if (!error)
        {
          WiFiDmxChannel = doc["dmxchannel"].as<String>();
          WiFiUniverse = doc["universe"].as<String>();
          // strcpy(WiFiDmxChannel, doc["dmxchannel"]);
          Serial.println("Parsed json:\n");
          Serial.printf("Universe: %s\n", WiFiUniverse.c_str());
          Serial.printf("DMX Channel: %s\n", WiFiDmxChannel.c_str());
          // strcpy(output, configFile["value"]);
        }
        else
        {
          Serial.println("Failed to load json config:\n");
          Serial.println(error.c_str());
          return;
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  //end read

  // WiFiManagler local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  // reset saved settings
  // wifiManager.resetSettings();

  // flag for saving data
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  // wifiManager.setCustomHeadElement("<style>html{filter: invert(100%); -webkit-filter: invert(100%);}</style>");
  // wifiManager.setCustomHeadElement("<form action=\"/artnet\" method=\"get\"><button>Configure ArtNet</button></form>");
  // wifiManager.setCustomHeadElement("<style>body{background: #f7f5f5;}button{transition: 0.3s;opacity: 0.8;cursor: pointer;border:0;border-radius:1rem;background-color:#1dca79;color:#fff;line-height:2.4rem;font-size:1.2rem;width:100%;}button:hover {opacity: 1}button[type=\"submit\"]{margin-top: 15px;margin-bottom: 10px;font-weight: bold;text-transform: capitalize;}input{height: 30px;font-family:verdana;margin-top: 5px;background-color: rgb(253, 253, 253);border: 0px;-webkit-box-shadow: 2px 2px 5px 0px rgba(0,0,0,0.75);-moz-box-shadow: 2px 2px 5px 0px rgba(0,0,0,0.75);box-shadow: 2px 2px 5px 0px rgba(0,0,0,0.75);}div{color: #14a762;}div a{text-decoration: none;color: #14a762;}div[style*=\"text-align:left;\"]{color: black;}, div[class*=\"c\"]{border: 0px;}a[href*=\"wifi\"]{border: 2px solid #1dca79;text-decoration: none;color: #1dca79;padding: 10px 30px 10px 30px;font-family: verdana;font-weight: bolder;transition: 0.3s;border-radius: 5rem;}a[href*=\"wifi\"]:hover{background: #1dca79;color: white;}</style>");

  WiFiManagerParameter custom_text("<p><h3>ArtNet Parameters</h3></p>");
  wifiManager.addParameter(&custom_text);

  // Adding an additional config on the WIFI manager webpage for the API Key and Channel ID
  WiFiManagerParameter customUniverse("universe", "Universe", WiFiUniverse.c_str(), 3);
  WiFiManagerParameter customDmxChannel("dmxchannel", "DMX Channel", WiFiDmxChannel.c_str(), 3);
  wifiManager.addParameter(&customUniverse);
  wifiManager.addParameter(&customDmxChannel);

    // set custom ip for portal
  // wifiManager.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

  // fetches ssid and pass from eeprom and tries to connect
  // if it does not connect it starts an access point with the specified name, here  "AutoConnectAP"
  // and goes into a blocking loop awaiting configuration
  // wifiManager.autoConnect("AutoConnectAP");
  wifiManager.startConfigPortal("OnDemandAP");

  WiFiUniverse = customUniverse.getValue();
  WiFiDmxChannel = customDmxChannel.getValue();

  if(shouldSaveConfig == true)
  {
    DynamicJsonDocument doc(1024);
    doc["universe"] = WiFiUniverse.c_str();
    doc["dmxchannel"] = WiFiDmxChannel.c_str();

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile)
    {
      Serial.println("Failed to open config file for writing\n");
    }
    else
    {
      Serial.println("Writing to config file:\n");
      serializeJson(doc, Serial);
      serializeJson(doc, configFile);
    }

    configFile.close();
  }

  // LED off after connecting to WLAN
  digitalWrite(ledPin, 1);

  // start FastLED port
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, numLeds);

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
