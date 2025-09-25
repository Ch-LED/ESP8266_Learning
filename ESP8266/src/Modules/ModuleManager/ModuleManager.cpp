
# include "ModuleManager.h"
# include "EventBus/EventBus.h"
# include "CommandRegistry/CommandRegistry.h"

std::vector<Module*>& ModuleManager::getModules() {
    static std::vector<Module*> modules;
    return modules;
}

void ModuleManager::registerModule(Module* module) {
    getModules().push_back(module);
}

void ModuleManager::initAllModules() {
    for (auto& module : getModules()) {
        Serial.printf("Initializing module: %s\n", module->getName().c_str());
        module->begin();
        module->setupEventSubscriptions();
    }
}

void ModuleManager::updateAllModules() {
    for (auto& module : getModules()) {
        module->update();
    }
}

bool ModuleManager::handleCommand(const char* command, void* data) {
    if (getModules().empty() || command == nullptr || strlen(command) == 0 || data == nullptr) {
        return false;
    }
    /* Debugging data content
    String dataStr;
    JsonObject obj = *static_cast<JsonObject*>(data);
    serializeJson(obj, dataStr);
    Serial.printf("[ModuleManager] Data received: %s\n", dataStr.c_str());
    */

    if (strcmp(command,"set_led") == 0) {
        if (CommandRegistry::executeCommand(command, data)) {
            Serial.printf("[ModuleManager] Command executed: %s\n", command);
            return true;
        } else {
            Serial.printf("[ModuleManager] Command execution failed: %s\n", command);
            return false;
        }
        return true;
    }
    else {
        Serial.printf("[ModuleManager] Unknown command: %s\n", command);
        return false;
    }
    return false;
}

size_t ModuleManager::getModuleCount() {
    return getModules().size();
}
