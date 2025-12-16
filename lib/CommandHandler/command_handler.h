#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include <Arduino.h>
#include "config_manager.h"

class CommandHandler {
private:
    ConfigManager& config;
    
    void handleReset();
    void handleSet(const String& arg);
    void handleDeepSleepSet(String value, bool& validKey);
    void handleNoSleep();
    void handleSleep();
    void handleReboot();
    void handleOTAVersion(const String& version);
    void showHelp();
    
public:
    CommandHandler(ConfigManager& cfg);
    void checkCommands();
};

#endif // COMMAND_HANDLER_H
