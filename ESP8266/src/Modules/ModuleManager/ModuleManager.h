
# pragma once

# include <vector>
# include "ModuleBase/ModuleBase.h"

class ModuleManager {
public:
    static void registerModule(Module* module);
    static void initAllModules();
    static void updateAllModules();
    static bool handleCommand(const char* command, void* data);
    static size_t getModuleCount();

private:
    static std::vector<Module*>& getModules();
};