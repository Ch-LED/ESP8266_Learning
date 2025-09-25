
# pragma once

# include <Arduino.h>
# include <ArduinoJson.h>
# include "EventBus/EventBus.h"
# include "CommandRegistry/CommandRegistry.h"

class ModuleManager;

class Module {
public:
    Module();

    virtual void begin() = 0;
    virtual void update() = 0;
    virtual bool handleCommand(const String& command, void* data) = 0;
    virtual const String& getName() const = 0;
    virtual void setupEventSubscriptions() = 0;

protected:
    virtual void buildResponse(JsonObject* responseObj, const char* status, const char* message, const JsonObject& extraData);
    virtual void buildResponse(JsonObject* responseObj, const char* status, const char* message);
    virtual bool decodeData(void* data, JsonObject& dataObj, JsonObject& responseObj);
};