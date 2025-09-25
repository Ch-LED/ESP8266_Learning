
# pragma once

# include "ModuleBase/ModuleBase.h"

class BasicCommandsModule : public Module {
public:
    void begin() override;
    void update() override;
    bool handleCommand(const String& command, void* data) override;
    const String& getName() const override;
    void setupEventSubscriptions() override;
private:
    // LED control state
    bool breathEnabled = false;
    int brightness = 0;
    int fadeAmount = 5;
    unsigned long breathInterval = 30; // milliseconds
    unsigned long lastUpdateTime = 0;

    // Safe reboot state
    bool safeReboot = false;
    unsigned long rebootTime = 0;

    // Command handlers
    bool handleSafeReboot(void* data);
    void updateSafeReboot();
    bool handleLedStateChanged(void* data);
    void updateBreathEffect();
};

extern BasicCommandsModule basicCommandsModule;