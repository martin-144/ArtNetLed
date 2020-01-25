#ifndef WIFIMANAGER_H
#define WIFIMANAGER_H

#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>         // https://github.com/bblanchon/ArduinoJson
#include <DNSServer.h>
#include <artnet.h>

String getParam();
void saveParamsCallback();
void wifiManagerStart();

// WiFiManagler local intialization. Once its business is done, there is no need to keep it around
extern WiFiManager wifiManager;

String getParam(String name)
{
  //read parameter from server, for customhmtl input
  String value;
  if(wifiManager.server->hasArg(name)) {
    value = wifiManager.server->arg(name);
  }
  return value;
}

//callback notifying us of the need to save config
void saveParamsCallback()
{
  Serial.println("*WM: Should save params");
  artnet.universe = getParam("universe").toInt();
  artnet.dmxChannel = getParam("dmxchannel").toInt();
  spiffsWrite();
}

void wifiManagerStart()
{
  WiFi.mode(WIFI_STA);

  // Read configuration from SPIFFS FS
  spiffsRead();

  // Callback for saving data
  wifiManager.setSaveParamsCallback(saveParamsCallback);


  // wifiManager.setCustomHeadElement("<style>html{filter: invert(100%); -webkit-filter: invert(100%);}</style>");
  // wifiManager.setCustomHeadElement("<form action=\"/artnet\" method=\"get\"><button>Configure ArtNet</button></form>");
  // wifiManager.setCustomHeadElement("<style>body{background: #f7f5f5;}button{transition: 0.3s;opacity: 0.8;cursor: pointer;border:0;border-radius:1rem;background-color:#1dca79;color:#fff;line-height:2.4rem;font-size:1.2rem;width:100%;}button:hover {opacity: 1}button[type=\"submit\"]{margin-top: 15px;margin-bottom: 10px;font-weight: bold;text-transform: capitalize;}input{height: 30px;font-family:verdana;margin-top: 5px;background-color: rgb(253, 253, 253);border: 0px;-webkit-box-shadow: 2px 2px 5px 0px rgba(0,0,0,0.75);-moz-box-shadow: 2px 2px 5px 0px rgba(0,0,0,0.75);box-shadow: 2px 2px 5px 0px rgba(0,0,0,0.75);}div{color: #14a762;}div a{text-decoration: none;color: #14a762;}div[style*=\"text-align:left;\"]{color: black;}, div[class*=\"c\"]{border: 0px;}a[href*=\"wifi\"]{border: 2px solid #1dca79;text-decoration: none;color: #1dca79;padding: 10px 30px 10px 30px;font-family: verdana;font-weight: bolder;transition: 0.3s;border-radius: 5rem;}a[href*=\"wifi\"]:hover{background: #1dca79;color: white;}</style>");
  // wifiManager.setCustomHeadElement("<script>if (window.location.href.indexOf(\"/wifi\")<0) {window.location=\"/wifi\"};</script>");

  // Adding an additional config on the WIFI manager webpage for the universe and DMX channel
  WiFiManagerParameter customText("<p><h3>ArtNet Parameters</h3></p>");
  WiFiManagerParameter customUniverse("universe", "Universe", String(artnet.universe).c_str() , 3, "type='number'");
  WiFiManagerParameter customDmxChannel("dmxchannel", "DMX Channel", String(artnet.dmxChannel).c_str() , 3, "type='number'");
  wifiManager.addParameter(&customText);
  wifiManager.addParameter(&customUniverse);
  wifiManager.addParameter(&customDmxChannel);

  /*
  ---------------------------------------------------------------------
  Code below will only work with the development version of WiFiManager
  ---------------------------------------------------------------------
  */

  std::vector<const char*> menu = {"wifi","wifinoscan","param","info","close","sep","exit","sep","restart","erase"};
  wifiManager.setMenu(menu); // custom menu, pass vector

  // set custom ip for portal
  // wifiManager.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

  wifiManager.setShowStaticFields(true);

  /*
  ---------------------------------------------------------------------
  Code above will only work with the development version of WiFiManager
  ---------------------------------------------------------------------
  */

  // wifiManager.setConfigPortalTimeout(180);
  wifiManager.autoConnect("AutoConnectAP");
  // wifiManager.startConfigPortal("OnDemandAP");
}

#endif // WIFIMANAGER_H
