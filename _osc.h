#pragma once

#include <ArduinoOSCWiFi.h>
#include <AsyncUDP.h>

#include <functional>
#include <unordered_map>

using OscCallback = std::function<void(const OscMessage&)>;

void setupOSC();
void loopOSC();

void oscSubscribe(const String& address, OscCallback callback);

extern int oscPort;
extern unsigned long messagesCount;