/*
 * Display Manager Library
 * 
 * Manages SH1106 OLED display for battery monitoring
 * Displays battery info and WiFi signal strength
 */

#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include "battery_monitor.h"

// Display configuration
namespace DisplayConfig {
    const uint8_t I2C_SDA = 21;      // Default SDA pin
    const uint8_t I2C_SCL = 22;      // Default SCL pin
    const uint8_t I2C_ADDRESS = 0x3C; // Default SH1106 I2C address
    const unsigned long UPDATE_INTERVAL = 1000; // Update display every 1 second
}

class DisplayManager {
public:
    DisplayManager();
    
    // Initialize display with default or custom I2C pins
    void begin(uint8_t sda = DisplayConfig::I2C_SDA, 
               uint8_t scl = DisplayConfig::I2C_SCL);
    
    // Update display with battery and network info
    void update(const BatteryReading& reading, bool wifiConnected, int8_t rssi);
    
    // Display individual screens
    void showBootScreen(int bootCount);
    void showOTAScreen(const char* message);
    void showOTAProgress(unsigned int progress, unsigned int total);
    void showOTAComplete();
    void showOTAError(const char* error);
    void showSleepScreen(time_t wakeupTime, const BatteryReading& reading);
    
    // Clear display
    void clear();
    
    // Check if display is ready
    bool isReady() const { return initialized; }
    
private:
    U8G2_SH1106_128X64_NONAME_F_HW_I2C display;
    bool initialized;
    unsigned long lastUpdate;
    
    // Helper functions
    void drawBatteryIcon(uint8_t x, uint8_t y, float percentage);
    void drawWiFiIcon(uint8_t x, uint8_t y, int8_t rssi);
    int8_t getWiFiSignalBars(int8_t rssi);
};

#endif // DISPLAY_MANAGER_H
