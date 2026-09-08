#include "_config.h"
#include "_osc.h"

int oscPort = 9004; 
unsigned long messagesCount = 0;

AsyncUDP oscUdp;

struct ArduinoStringHash
{
    size_t operator()(const String& s) const
    {
        size_t hash = 5381;

        for (size_t i = 0; i < s.length(); ++i)
            hash = ((hash << 5) + hash) ^ s[i];

        return hash;
    }
};

static std::unordered_map<
    String,
    OscCallback,
    ArduinoStringHash
> oscCallbacks;


static bool restartOscUdp()
{
    oscUdp.close();

    bool success = oscUdp.listen(oscPort);

    if (success)
        Serial.println("OSC: AsyncUDP redémarré");
    else
        Serial.println("OSC: échec du redémarrage AsyncUDP");

    return success;
}

void oscSubscribe(const String& address, OscCallback callback)
{
    oscCallbacks[address] = std::move(callback);
}

static void processOscPacket(AsyncUDPPacket& packet)
{
    OscDecoder decoder;

    if (!decoder.init(packet.data(), packet.length()))
        return;

    OscMessage* message;

    while ((message = decoder.decode()) != nullptr)
    {
        if (!message->available())
            continue;
        messagesCount++;
        message->remoteIP(packet.remoteIP());
        message->remotePort(packet.remotePort());

        auto it = oscCallbacks.find(message->address());

        if (it != oscCallbacks.end())
            it->second(*message);
    }
}

void sendConfig(String k, String remoteIp) {
  if (configTypes[k] == "b") { 
    OscWiFi.getClient().send(remoteIp, oscPort, "/"+chipName+""+String(chipId)+"/config/"+k, config[k].as<bool>()); 
  }
  else if (configTypes[k] == "f") { 
    OscWiFi.getClient().send(remoteIp, oscPort, "/"+chipName+""+String(chipId)+"/config/"+k, config[k].as<float>()); 
    if (!configOptions[k]["rangeMin"].isNull()) {
      OscWiFi.getClient().send(remoteIp, oscPort, "/"+chipName+""+String(chipId)+"/config/"+k, config[k].as<float>(), "range",configOptions[k]["rangeMin"].as<float>(), configOptions[k]["rangeMax"].as<float>());
    }
  }
  else if (configTypes[k] == "i") { 
    OscWiFi.getClient().send(remoteIp, oscPort, "/"+chipName+""+String(chipId)+"/config/"+k, config[k].as<int>()); 
    if (!configOptions[k]["rangeMin"].isNull()) {
      OscWiFi.getClient().send(remoteIp, oscPort, "/"+chipName+""+String(chipId)+"/config/"+k, config[k].as<int>(), "range",configOptions[k]["rangeMin"].as<int>(), configOptions[k]["rangeMax"].as<int>());
    }
  }
  else if (configTypes[k] == "s") { 
    OscWiFi.getClient().send(remoteIp, oscPort, "/"+chipName+""+String(chipId)+"/config/"+k, config[k].as<String>()); 
  }
  else if (configTypes[k] == "e") { 
    OscWiFi.getClient().send(remoteIp, oscPort, "/"+chipName+""+String(chipId)+"/config/"+k, config[k].as<String>()); 
    OscWiFi.getClient().send(remoteIp, oscPort, "/"+chipName+""+String(chipId)+"/config/"+k, config[k].as<String>(), "options",configOptions[k]["options"].as<String>());
  }

}

void sendInfo(String k, String remoteIp) {
  if (!info[k].isNull() && info[k].is<bool>()) { OscWiFi.getClient().send(remoteIp, oscPort, "/"+chipName+""+String(chipId)+"/info/"+k, info[k].as<bool>()); }
  else if (!info[k].isNull() && info[k].is<float>()) { OscWiFi.getClient().send(remoteIp, oscPort, "/"+chipName+""+String(chipId)+"/info/"+k, info[k].as<float>()); }
  else if (!info[k].isNull() && info[k].is<int>()) { OscWiFi.getClient().send(remoteIp, oscPort, "/"+chipName+""+String(chipId)+"/info/"+k, info[k].as<int>()); }
  else if (!info[k].isNull() && info[k].is<String>()) { OscWiFi.getClient().send(remoteIp, oscPort, "/"+chipName+""+String(chipId)+"/info/"+k, info[k].as<String>()); }
}

void subscribeAll() {
  for (JsonPair kv : config.as<JsonObject>()) {
    String k = kv.key().c_str();
    oscSubscribe("/config/"+k, [k](const OscMessage& m){
      if (m.size()>0) {
        if (m.isBool(0)) {writeConfig(k, m.getArgAsBool(0));}
        else if (m.isFloat(0)) {writeConfig(k, m.getArgAsFloat(0));}
        else if (m.isInt32(0)) {writeConfig(k, m.getArgAsInt32(0));}
        else if (m.isStr(0)) {writeConfig(k, m.getArgAsString(0));}
      } else {
        sendConfig(k, m.remoteIP());
      }
    } );
  }

  oscSubscribe("/config", [](const OscMessage& m){
      for (JsonPair kv : config.as<JsonObject>()) {
        String k = kv.key().c_str();
        sendConfig(k, m.remoteIP());
      }
  });

  for (JsonPair kv : trigger.as<JsonObject>()) {
    String k = kv.key().c_str();
    oscSubscribe("/trigger/"+k, [k](const OscMessage& m){
      triggerTriggered(k);
    } );
  }
  oscSubscribe("/trigger", [](const OscMessage& m){
    for (JsonPair kv : trigger.as<JsonObject>()) {
      String k = kv.key().c_str();
      //OscWiFi.getClient().send(m.remoteIP(), oscPort, "/"+chipName+""+String(chipId)+"/trigger/"+k); 
    }
  });

  for (JsonPair kv : info.as<JsonObject>()) {
    String k = kv.key().c_str();
    oscSubscribe("/info/"+k, [k](const OscMessage& m){
      sendInfo(k, m.remoteIP());
    } );
  }
  oscSubscribe("/info", [](const OscMessage& m){
      for (JsonPair kv : info.as<JsonObject>()) {
        String k = kv.key().c_str();
        sendInfo(k, m.remoteIP());
      }
  });


}

void setupOSC()
{
    subscribeAll();
    oscUdp.onPacket([](AsyncUDPPacket packet)
    {
        processOscPacket(packet);
    });

    if (oscUdp.listen(oscPort)) Serial.println("OSC: AsyncUDP listening");
    else Serial.println("OSC: impossible de démarrer AsyncUDP");
}
