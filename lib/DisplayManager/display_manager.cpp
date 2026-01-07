/*
 * Display Manager Library Implementation
 */

#include "display_manager.h"

DisplayManager::DisplayManager() 
    : display(U8G2_R0, /* reset=*/ U8X8_PIN_NONE)
    , initialized(false)
    , lastUpdate(0) {
}

void DisplayManager::begin(uint8_t sda, uint8_t scl) {
    // Initialize I2C with custom pins
    Wire.begin(sda, scl);
    
    // Initialize display
    if (!display.begin()) {
        Serial.println("Failed to initialize SH1106 display!");
        initialized = false;
        return;
    }
    
    display.setFont(u8g2_font_6x10_tr);
    display.clear();
    initialized = true;
    
    Serial.println("SH1106 Display initialized");
}

void DisplayManager::update(const BatteryReading& reading, bool wifiConnected, int8_t rssi) {
    if (!initialized) return;
    
    // Throttle updates
    unsigned long now = millis();
    if (now - lastUpdate < DisplayConfig::UPDATE_INTERVAL) return;
    lastUpdate = now;
    
    display.clearBuffer();
    
    // Title bar
    display.setFont(u8g2_font_5x7_tr);
    display.drawStr(0, 7, "Battery Monitor");
    display.drawHLine(0, 9, 128);
    
    // Battery section
    display.setFont(u8g2_font_9x15_tr);
    
    // Voltage - large font
    char voltageStr[16];
    snprintf(voltageStr, sizeof(voltageStr), "%.2fV", reading.voltage);
    display.drawStr(5, 28, voltageStr);
    
    // Battery icon
    drawBatteryIcon(100, 15, reading.percentage);
    
    // Percentage bar
    display.setFont(u8g2_font_6x10_tr);
    char percentStr[8];
    snprintf(percentStr, sizeof(percentStr), "%.0f%%", reading.percentage);
    display.drawStr(5, 42, percentStr);
    
    // Status
    display.drawStr(45, 42, BatteryMonitor::statusToString(reading.status));
    
    // Progress bar for percentage
    display.drawFrame(5, 46, 118, 8);
    int barWidth = (int)((reading.percentage / 100.0) * 114);
    if (barWidth > 0) {
        display.drawBox(7, 48, barWidth, 4);
    }
    
    // WiFi section
    display.drawHLine(0, 56, 128);
    display.setFont(u8g2_font_5x7_tr);
    
    if (wifiConnected) {
        char wifiStr[32];
        snprintf(wifiStr, sizeof(wifiStr), "WiFi: %ddBm", rssi);
        display.drawStr(5, 63, wifiStr);
        drawWiFiIcon(100, 57, rssi);
    } else {
        display.drawStr(5, 63, "WiFi: Disconnected");
    }
    
    display.sendBuffer();
}

void DisplayManager::showBatteryInfo(const BatteryReading& reading) {
    if (!initialized) return;
    
    display.clearBuffer();
    
    display.setFont(u8g2_font_9x15_tr);
    display.drawStr(15, 15, "Battery Info");
    
    char buffer[32];
    display.setFont(u8g2_font_7x13_tr);
    
    snprintf(buffer, sizeof(buffer), "Voltage: %.2fV", reading.voltage);
    display.drawStr(5, 32, buffer);
    
    snprintf(buffer, sizeof(buffer), "Level: %.1f%%", reading.percentage);
    display.drawStr(5, 46, buffer);
    
    snprintf(buffer, sizeof(buffer), "Status: %s", BatteryMonitor::statusToString(reading.status));
    display.drawStr(5, 60, buffer);
    
    display.sendBuffer();
}

void DisplayManager::showWiFiInfo(bool connected, int8_t rssi) {
    if (!initialized) return;
    
    display.clearBuffer();
    
    display.setFont(u8g2_font_9x15_tr);
    display.drawStr(20, 15, "WiFi Status");
    
    display.setFont(u8g2_font_7x13_tr);
    
    if (connected) {
        display.drawStr(5, 35, "Connected");
        
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "Signal: %ddBm", rssi);
        display.drawStr(5, 50, buffer);
        
        // Draw signal bars
        drawWiFiIcon(50, 55, rssi);
    } else {
        display.drawStr(5, 40, "Disconnected");
    }
    
    display.sendBuffer();
}

void DisplayManager::showBootScreen(int bootCount) {
    if (!initialized) return;
    
    display.clearBuffer();
    
    display.setFont(u8g2_font_9x15_tr);
    display.drawStr(10, 20, "Battery");
    display.drawStr(10, 36, "Monitor");
    
    display.setFont(u8g2_font_6x10_tr);
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "Boot: %d", bootCount);
    display.drawStr(10, 55, buffer);
    
    display.sendBuffer();
}

void DisplayManager::showOTAScreen(const char* message) {
    if (!initialized) return;
    
    display.clearBuffer();
    
    display.setFont(u8g2_font_9x15_tr);
    display.drawStr(20, 20, "OTA Update");
    
    display.setFont(u8g2_font_6x10_tr);
    display.drawStr(5, 40, message);
    
    display.sendBuffer();
}

void DisplayManager::showSleepScreen(int seconds) {
    if (!initialized) return;
    
    display.clearBuffer();
    
    display.setFont(u8g2_font_9x15_tr);
    display.drawStr(5, 25, "Deep Sleep");
    
    display.setFont(u8g2_font_7x13_tr);
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "Wake in: %ds", seconds);
    display.drawStr(5, 45, buffer);
    
    display.sendBuffer();
}

void DisplayManager::clear() {
    if (!initialized) return;
    display.clear();
}

void DisplayManager::drawBatteryIcon(uint8_t x, uint8_t y, float percentage) {
    // Battery outline
    display.drawFrame(x, y, 20, 10);
    display.drawBox(x + 20, y + 3, 2, 4); // Terminal
    
    // Fill based on percentage
    int fillWidth = (int)((percentage / 100.0) * 16);
    if (fillWidth > 0) {
        display.drawBox(x + 2, y + 2, fillWidth, 6);
    }
}

void DisplayManager::drawWiFiIcon(uint8_t x, uint8_t y, int8_t rssi) {
    int8_t bars = getWiFiSignalBars(rssi);
    
    // Draw signal bars (4 bars max)
    for (int i = 0; i < 4; i++) {
        if (i < bars) {
            int height = (i + 1) * 2;
            display.drawBox(x + (i * 4), y + 8 - height, 3, height);
        } else {
            int height = (i + 1) * 2;
            display.drawFrame(x + (i * 4), y + 8 - height, 3, height);
        }
    }
}

const char* DisplayManager::getBatteryStatusIcon(BatteryStatus status) {
    switch (status) {
        case BatteryStatus::FULL:     return "█";
        case BatteryStatus::GOOD:     return "▓";
        case BatteryStatus::LOW_BATTERY: return "▒";
        case BatteryStatus::CRITICAL: return "░";
        case BatteryStatus::DEAD:     return "○";
        default:                      return "?";
    }
}

int8_t DisplayManager::getWiFiSignalBars(int8_t rssi) {
    // Convert RSSI to signal bars (1-4)
    if (rssi >= -50) return 4;      // Excellent
    if (rssi >= -60) return 3;      // Good
    if (rssi >= -70) return 2;      // Fair
    if (rssi >= -80) return 1;      // Weak
    return 0;                        // Very weak
}
