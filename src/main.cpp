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
#include <string>

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
void saveConfigCallback ()
{
  Serial.println("Should save config");
  shouldSaveConfig = true;
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

  Serial.println("Mounting SPIFFS file system...");

  if (SPIFFS.begin())
  {
    Serial.println("Mounted file system");
    if (SPIFFS.exists("/config.json"))
    {
      //file exists, reading and loading
      Serial.println("Reading config file...");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile)
      {
        Serial.printf("Opened config file, size: %d bytes\n", configFile.size());
        configFile.print(Serial);

        DynamicJsonDocument doc(512);
        DeserializationError error = deserializeJson(doc, configFile);
        if (!error)
        {
          WiFiDmxChannel = doc["dmxchannel"].as<String>();
          WiFiUniverse = doc["universe"].as<String>();
          Serial.printf("Parsed Json: ");
          serializeJson(doc, Serial);
          Serial.println();
        }
        else
        {
          Serial.println("Failed to load json config:\n");
          Serial.println(error.c_str());
          return;
        }
      }
      configFile.close();
    }
  }
  else
  {
    Serial.println("failed to mount SPIFFS FS");
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

  // Adding an additional config on the WIFI manager webpage for the universe and DMX channel
  WiFiManagerParameter customText("<p><h3>ArtNet Parameters</h3></p>");
  WiFiManagerParameter customUniverse("universe", "Universe", WiFiUniverse.c_str(), 3, "type='number'");
  WiFiManagerParameter customDmxChannel("dmxchannel", "DMX Channel", WiFiDmxChannel.c_str(), 3, "type='number'");
  wifiManager.addParameter(&customText);
  wifiManager.addParameter(&customUniverse);
  wifiManager.addParameter(&customDmxChannel);

  // set custom ip for portal
  // wifiManager.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

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
      Serial.printf("Writing to config file:\n");
      serializeJson(doc, Serial);
      serializeJson(doc, configFile);
    }
    configFile.close();
  }

  dmxChannel = WiFiDmxChannel.toInt();

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
