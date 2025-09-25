
# include "BasicCommandsModule.h"

BasicCommandsModule basicCommandsModule;

void BasicCommandsModule::begin() {
    CommandRegistry::registerCommand("set_led", [this](void* data) {
        return handleLedStateChanged(data);
    });
    CommandRegistry::registerCommand("safe_reboot", [this](void* data) {
        return handleSafeReboot(data);
    });
    Serial.println("[BasicCommandsModule] Initialized.");
}

void BasicCommandsModule::update() {
    updateBreathEffect();
    updateSafeReboot();
}

bool BasicCommandsModule::handleCommand(const String& command, void* data) {
    // Command handling logic here
    return false; // Return true if the command was handled
}

const String& BasicCommandsModule::getName() const {
    static String name = "BasicCommandsModule";
    return name;
}

void BasicCommandsModule::setupEventSubscriptions() {
    // Subscribe to events
}

bool BasicCommandsModule::handleSafeReboot(void* data) {
    JsonObject dataObj, responseObj;
    if (!decodeData(data, dataObj, responseObj)) {
        Serial.println("[BasicCommandsModule] Failed to decode event data.");
        return false;
    }
    Serial.println("[BasicCommandsModule] Safe reboot initiated.");
    buildResponse(&responseObj, "ok", "Safe reboot initiated");
    safeReboot = true;
    rebootTime = millis() + 1000; // Reboot after 1 second
    return true;
}

void BasicCommandsModule::updateSafeReboot() {
    if (safeReboot && millis() >= rebootTime) {
        Serial.println("[BasicCommandsModule] Rebooting now...");
        delay(100);
        ESP.restart();
    }
}

bool BasicCommandsModule::handleLedStateChanged(void* data) {
    if (data == nullptr) {
        Serial.println("[BasicCommandsModule] No data provided for LED state change.");
        return false;
    }
    
    JsonObject dataObj, responseObj;
    if (!decodeData(data, dataObj, responseObj)) {
        Serial.println("[BasicCommandsModule] Failed to decode data.");
        return false;
    }

    JsonArray arguments = dataObj["arguments"].as<JsonArray>();
    const char* state = arguments[0].as<const char*>();

    if (state == nullptr) {
        Serial.println("[BasicCommandsModule] Invalid LED state data.");
        return false;
    }
    
    if (strcmp(state, "on") == 0) {
        breathEnabled = false;
        digitalWrite(LED_BUILTIN, LOW);
        buildResponse(&responseObj, "ok", "LED turned on");
    }
    else if (strcmp(state, "off") == 0) {
        breathEnabled = false;
        digitalWrite(LED_BUILTIN, HIGH);
        buildResponse(&responseObj, "ok", "LED turned off");
    }
    else if (strcmp(state, "breath_on") == 0) {
        breathEnabled = true;
        brightness = 0;
        fadeAmount = 5;
        buildResponse(&responseObj, "ok", "Breathing LED enabled");
    }
    else if (strcmp(state, "breath_off") == 0) {
        breathEnabled = false;
        digitalWrite(LED_BUILTIN, HIGH);
        buildResponse(&responseObj, "ok", "Breathing LED disabled");
    }
    else {
        buildResponse(&responseObj, "error", "Unknown LED state");
        Serial.println("[BasicCommandsModule] Unknown LED state.");
        return false;
    }

    Serial.printf("[BasicCommandsModule] LED state changed to: %s\n", state);
    return true;
}

void BasicCommandsModule::updateBreathEffect() {
    if (!breathEnabled) return;

    unsigned long currentTime = millis();
    if (currentTime - lastUpdateTime >= breathInterval) {
        lastUpdateTime = currentTime;

        brightness += fadeAmount;
        if (brightness <= 0 || brightness >= 255) {
            fadeAmount = -fadeAmount;
        }
        analogWrite(LED_BUILTIN, brightness);
    }
}
