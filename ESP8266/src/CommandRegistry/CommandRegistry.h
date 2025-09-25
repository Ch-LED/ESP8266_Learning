
# pragma once

# include <Arduino.h>
# include <functional>
# include <map>

class CommandRegistry {
public:
    using CommandHandler = std::function<void(void* data)>;

    static void registerCommand(const char* command, CommandHandler handler);
    static bool executeCommand(const char* command, void* data);

private:
    static std::map<String, CommandHandler> commandHandlers;
};