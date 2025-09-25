
# include "CommandRegistry.h"

CommandRegistry commandRegistry;

std::map<String, CommandRegistry::CommandHandler> CommandRegistry::commandHandlers;

void CommandRegistry::registerCommand(const char* command, CommandHandler handler) {
    if (commandHandlers.find(command) == commandHandlers.end()) {
        commandHandlers[command] = handler;
        Serial.printf("[CommandRegistry] Registered command: %s\n", command);
    } else {
        Serial.printf("[CommandRegistry] Command already registered: %s\n", command);
    }
}

bool CommandRegistry::executeCommand(const char* command, void* data) {
    auto it = commandHandlers.find(command);
    if (it != commandHandlers.end()) {
        it->second(data);
        return true;
    } else {
        Serial.printf("[CommandRegistry] Unknown command: %s\n", command);
        return false;
    }
}