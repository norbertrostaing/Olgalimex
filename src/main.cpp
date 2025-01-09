#include <Arduino.h>

#include "_config.h"
#include "_ethernet.h"
#include "_wifi.h"
#include "_dmx.h"
#include "_artnet.h"
#include "_osc.h"
#include "_web.h"

void setup() {
  // generic parameters
  oscPort = 9004;
  chipName = "Olgalimex";

  // config : read and write parameters
  // infos : read only parameters
  Serial.begin(115200);
  addStringConfig("test/stringParam", "test");
  addIntConfig("test/intParam", 42);
  setRange("test/intParam",0,512);
  addFloatConfig("test/floatParam", 3.14156);
  setRange("test/floatParam",-360.0,360.0);
  addBoolConfig("test/boolParam", true);
  addEnumConfig("test/enumParam", "Option A");
  addEnumOption("test/enumParam", "Option B");
  addEnumOption("test/enumParam", "Option C");
  addEnumOption("test/enumParam", "Option D");
  addTrigger("go");
  addTrigger("reset");
  addTrigger("purge");

  initConfig();
  setupEthernet();
  setupWifi();
  setupOSC();
  setupWebServer();
  //setupArtnet();
  //setupDMX();

  OscWiFi.subscribe(oscPort, "/test", [](const OscMessage& m){
    OscWiFi.send(m.remoteIP(), oscPort, "/test/coucou", 0.24, "test");
    Serial.println("test");
  });
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(40);
}


void configUpdatedMain(String key) {
  
}

void triggerTriggered(String str) {
  Serial.println("trigger : "+str);
}

void onArtnetFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data, IPAddress remoteIP)
{
    Serial.println("Artnet received : universe "+String(universe));
}

void onDmxFrame()
{
  Serial.println("DMX received");
}

