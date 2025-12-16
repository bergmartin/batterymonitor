#include "command_handler.h"

CommandHandler::CommandHandler(ConfigManager& cfg) : config(cfg) {}

void CommandHandler::checkCommands() {
    if (Serial.available() > 0) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        
        // Parse command and arguments
        String cmd = command;
        String arg = "";
        int spaceIndex = command.indexOf(' ');
        if (spaceIndex > 0) {
            cmd = command.substring(0, spaceIndex);
            arg = command.substring(spaceIndex + 1);
            arg.trim();
        }
        cmd.toLowerCase();
        
        if (cmd == "reset" && (arg == "nvs" || arg == "")) {
            handleReset();
        }
        else if (cmd == "show" || cmd == "config") {
            config.printConfig();
        }
        else if (cmd == "save") {
            config.saveConfig();
            Serial.println("✓ Configuration saved to NVS");
        }
        else if (cmd == "set") {
            handleSet(arg);
        }
        else if (cmd == "nosleep" || cmd == "stay" || cmd == "awake") {
            handleNoSleep();
        }
        else if (cmd == "sleep") {
            handleSleep();
        }
        else if (cmd == "reboot" || cmd == "restart") {
            handleReboot();
        }
        else if (cmd == "otaver") {
            handleOTAVersion(arg);
        }
        else if (cmd == "help") {
            showHelp();
        }
        else if (command.length() > 0) {
            Serial.print("✗ Unknown command: ");
            Serial.println(command);
            Serial.println("Type 'help' for available commands");
        }
    }
}

void CommandHandler::handleReset() {
    Serial.println("\n╔═══════════════════════════════╗");
    Serial.println("║   Clearing NVS Storage        ║");
    Serial.println("╚═══════════════════════════════╝");
    config.clear();
    Serial.println("NVS cleared. Rebooting...");
    delay(1000);
    ESP.restart();
}

void CommandHandler::handleSet(const String& arg) {
    int secondSpace = arg.indexOf(' ');
    if (secondSpace > 0) {
        String key = arg.substring(0, secondSpace);
        String value = arg.substring(secondSpace + 1);
        key.trim();
        key.toLowerCase();
        value.trim();
        
        bool validKey = true;
        
        if (key == "wifi_ssid" || key == "ssid") {
            config.wifiSSID = value;
            Serial.print("✓ WiFi SSID set to: ");
            Serial.println(value);
        }
        else if (key == "wifi_password" || key == "wifi_pass" || key == "password") {
            config.wifiPassword = value;
            Serial.println("✓ WiFi password set (hidden)");
        }
        else if (key == "mqtt_server" || key == "server") {
            config.mqttServer = value;
            Serial.print("✓ MQTT server set to: ");
            Serial.println(value);
        }
        else if (key == "mqtt_port" || key == "port") {
            config.mqttPort = value.toInt();
            Serial.print("✓ MQTT port set to: ");
            Serial.println(config.mqttPort);
        }
        else if (key == "mqtt_user" || key == "user") {
            config.mqttUser = value;
            Serial.print("✓ MQTT user set to: ");
            Serial.println(value);
        }
        else if (key == "mqtt_password" || key == "mqtt_pass") {
            config.mqttPassword = value;
            Serial.println("✓ MQTT password set (hidden)");
        }
        else if (key == "mqtt_client_id" || key == "client_id" || key == "id") {
            config.mqttClientID = value;
            Serial.print("✓ MQTT client ID set to: ");
            Serial.println(value);
        }
        else if (key == "deep_sleep") {
            handleDeepSleepSet(value, validKey);
        }
        else if (key == "ota_version" || key == "ota_target" || key == "otaver") {
            config.otaTargetVersion = value;
            Serial.print("✓ OTA target version set to: ");
            Serial.println(value);
        }
        else {
            validKey = false;
            Serial.print("✗ Unknown key: ");
            Serial.println(key);
            Serial.println("Type 'help' for valid keys");
        }
        
        if (validKey) {
            Serial.println("Remember to type 'save' to persist changes!");
        }
    } else {
        Serial.println("✗ Usage: set <key> <value>");
        Serial.println("Example: set wifi_ssid MyNetwork");
    }
}

void CommandHandler::handleDeepSleepSet(String value, bool& validKey) {
    value.toLowerCase();
    if (value == "true" || value == "1" || value == "on" || value == "enable") {
        config.deepSleepEnabled = true;
        Serial.println("✓ Deep sleep enabled");
    } else if (value == "false" || value == "0" || value == "off" || value == "disable") {
        config.deepSleepEnabled = false;
        Serial.println("✓ Deep sleep disabled");
    } else {
        validKey = false;
        Serial.print("✗ Invalid value: ");
        Serial.println(value);
        Serial.println("Use: true/false, on/off, enable/disable, or 1/0");
    }
}

void CommandHandler::handleNoSleep() {
    config.deepSleepEnabled = false;
    config.saveConfig();
    Serial.println("✓ Deep sleep disabled and saved");
    Serial.println("Device will stay awake for debugging");
}

void CommandHandler::handleSleep() {
    config.deepSleepEnabled = true;
    config.saveConfig();
    Serial.println("✓ Deep sleep enabled and saved");
    Serial.println("Device will enter deep sleep after next reading");
}

void CommandHandler::handleReboot() {
    Serial.println("Rebooting...");
    delay(500);
    ESP.restart();
}

void CommandHandler::handleOTAVersion(const String& version) {
    if (version.length() > 0) {
        config.otaTargetVersion = version;
        config.saveConfig();
        Serial.print("✓ OTA target version set to: ");
        Serial.println(version);
        Serial.println("✓ Configuration saved");
        Serial.println("Device will check for this version on next wake");
    } else {
        Serial.println("Current OTA target version: " + 
                      (config.otaTargetVersion.length() > 0 ? config.otaTargetVersion : "(not set)"));
        Serial.println("Usage: otaver <version>");
        Serial.println("Example: otaver 1.0.2");
    }
}

void CommandHandler::showHelp() {
    Serial.println("\n╔═══════════════════════════════════════════════════════╗");
    Serial.println("║   Battery Monitor - Serial Commands                   ║");
    Serial.println("╚═══════════════════════════════════════════════════════╝");
    Serial.println("\nConfiguration Commands:");
    Serial.println("  show              - Display current configuration");
    Serial.println("  set <key> <value> - Change a configuration value");
    Serial.println("  save              - Save configuration to NVS");
    Serial.println("  reset nvs         - Clear NVS and reboot");
    Serial.println("\nConfiguration Keys:");
    Serial.println("  wifi_ssid         - WiFi network name");
    Serial.println("  wifi_password     - WiFi password");
    Serial.println("  mqtt_server       - MQTT broker address");
    Serial.println("  mqtt_port         - MQTT broker port");
    Serial.println("  mqtt_user         - MQTT username");
    Serial.println("  mqtt_password     - MQTT password");
    Serial.println("  mqtt_client_id    - MQTT client identifier");
    Serial.println("  deep_sleep        - Enable/disable deep sleep (true/false)");
    Serial.println("  ota_version       - Target OTA version (e.g., 1.0.1)");
    Serial.println("\nSystem Commands:");
    Serial.println("  nosleep           - Disable deep sleep (stay awake)");
    Serial.println("  sleep             - Enable deep sleep");
    Serial.println("  otaver <version>  - Set target OTA version (shortcut)");
    Serial.println("  reboot            - Restart the device");
    Serial.println("  help              - Show this help message");
    Serial.println("\nExamples:");
    Serial.println("  set wifi_ssid MyHomeNetwork");
    Serial.println("  set mqtt_server 192.168.1.100");
    Serial.println("  set deep_sleep false");
    Serial.println("  otaver 1.0.2");
    Serial.println("  save");
}
