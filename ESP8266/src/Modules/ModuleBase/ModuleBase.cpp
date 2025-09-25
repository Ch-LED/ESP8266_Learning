
# include "ModuleBase.h"
# include "ModuleManager/ModuleManager.h"

Module::Module() {
    ModuleManager::registerModule(this);
}

void Module::buildResponse(JsonObject* responseObj, const char* status, const char* message, const JsonObject& extraData) {
    (*responseObj)["content"] = message;
    JsonObject data = (*responseObj).createNestedObject("data");
    data["status"] = status;

    // Copy extra data if provided
    for (JsonPair kv : extraData) {
        data[kv.key()] = kv.value();
    }
}

void Module::buildResponse(JsonObject* responseObj, const char* status, const char* message) {
    StaticJsonDocument<1> emptyDoc;
    JsonObject emptyObj = emptyDoc.to<JsonObject>();
    buildResponse(responseObj, status, message, emptyObj);
}

bool Module::decodeData(void* data, JsonObject& dataObj, JsonObject& responseObj) {
    if (data == nullptr) {
        const char* name = getName().c_str();
        Serial.printf("[%s] Invalid data (nullptr).\n", name);
        return false;
    }

    JsonObject* mainObj = static_cast<JsonObject*>(data);

    if (mainObj->containsKey("data") && mainObj->containsKey("response")) {
        dataObj = (*mainObj)["data"].as<JsonObject>();
        responseObj = (*mainObj)["response"].as<JsonObject>();
        return true;
    } else {
        const char* name = getName().c_str();
        Serial.printf("[%s] Failed to extract data or response objects.\n", name);
        return false;
    }
}