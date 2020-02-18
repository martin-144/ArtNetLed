#ifndef SPIFFS_H
#define SPIFFS_H

#include <FS.h>
#include <ArduinoJson.h>         // https://github.com/bblanchon/ArduinoJson
#include <artnet.h>

void spiffsRead()
{
  Serial.println("*SF: Mounting SPIFFS file system");

  if (SPIFFS.begin())
  {
    Serial.println("*SF: Mounted file system");
    if (SPIFFS.exists("/config.json"))
    {
      //file exists, reading and loading
      Serial.println("*SF: Reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile)
      {
        Serial.printf("*SF: Opened config file, size: %d bytes\n", configFile.size());
        configFile.print(Serial);

        DynamicJsonDocument doc(512);
        DeserializationError error = deserializeJson(doc, configFile);
        if (!error)
        {
          artnet.dmxChannel = int(doc["dmxchannel"]);
          artnet.listenUniverse = int(doc["universe"]);
          Serial.printf("*SF: Parsed Json: ");
          serializeJson(doc, Serial);
          Serial.println();
        }
        else
        {
          Serial.println("*SF: Failed to load json config:\n");
          Serial.println(error.c_str());
          return;
        }
      }
      configFile.close();
      Serial.printf("*SF: Done.\n");
    }
    else
    {
      Serial.println("*SF: Config file does not exist: ");
    }
  }
  else
  {
    Serial.println("*SF: Failed to mount SPIFFS FS");
  }
//end read
}

void spiffsWrite()
{
    Serial.println("*SF: Should save params");
    DynamicJsonDocument doc(1024);
    doc["universe"] = String(artnet.listenUniverse);
    doc["dmxchannel"] = String(artnet.dmxChannel);

    Serial.println("*SF: SPIFFS FS trying to save params");

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile)
    {
      Serial.println("*SF: Failed to open config file for writing\n");
    }
    else
    {
      Serial.printf("*SF: Writing to config file: ");
      serializeJson(doc, Serial);
      Serial.println();
      serializeJson(doc, configFile);
    }
    configFile.close();
    Serial.printf("*SF: Done.\n");
}

#endif // SPIFFS_H
