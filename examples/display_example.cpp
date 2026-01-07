/*
 * SH1106 Display Example
 * 
 * Demonstrates basic usage of the DisplayManager library
 * Shows battery info and WiFi status on SH1106 OLED display
 */

#include <Arduino.h>
#include "display_manager.h"
#include "battery_monitor.h"
#include <WiFi.h>

// WiFi credentials (replace with your own)
const char* WIFI_SSID = "YourSSID";
const char* WIFI_PASSWORD = "YourPassword";

DisplayManager display;
BatteryMonitor battery;

void setup() {
    Serial.begin(115200);
    delay(500);
    
    Serial.println("SH1106 Display Example");
    
    // Initialize display with default pins (SDA=21, SCL=22)
    display.begin();
    
    // Or use custom pins:
    // display.begin(/* sda */ 21, /* scl */ 22);
    
    if (!display.isReady()) {
        Serial.println("Display initialization failed!");
        return;
    }
    
    // Show boot screen
    display.showBootScreen(1);
    delay(2000);
    
    // Initialize battery monitor
    battery.begin();
    
    // Connect to WiFi (optional)
    Serial.println("Connecting to WiFi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void loop() {
    // Read battery
    BatteryReading reading = battery.readBattery();
    
    // Get WiFi status
    bool wifiConnected = WiFi.status() == WL_CONNECTED;
    int8_t rssi = wifiConnected ? WiFi.RSSI() : 0;
    
    // Update display with battery and WiFi info
    display.update(reading, wifiConnected, rssi);
    
    // Print to serial for debugging
    Serial.printf("Voltage: %.2fV | Level: %.1f%% | WiFi: %s",
                  reading.voltage, 
                  reading.percentage,
                  wifiConnected ? "Connected" : "Disconnected");
    
    if (wifiConnected) {
        Serial.printf(" (%ddBm)", rssi);
    }
    Serial.println();
    
    delay(2000);
}

/*
 * Display Features Demonstrated:
 * 
 * 1. Battery voltage (large display)
 * 2. Battery percentage with progress bar
 * 3. Battery status (FULL, GOOD, LOW, CRITICAL, DEAD)
 * 4. Battery icon showing charge level
 * 5. WiFi connection status
 * 6. WiFi signal strength (RSSI in dBm)
 * 7. WiFi signal bars visualization
 * 
 * Display Layout:
 * ┌──────────────────────────────┐
 * │ Battery Monitor              │
 * ├──────────────────────────────┤
 * │ 12.45V               [████]  │ <- Voltage + Battery Icon
 * │ 85%    GOOD                  │ <- Percentage + Status
 * │ [███████████████░░]          │ <- Progress Bar
 * ├──────────────────────────────┤
 * │ WiFi: -65dBm         [▐▐▐▐]  │ <- WiFi + Signal Bars
 * └──────────────────────────────┘
 * 
 * Wiring:
 * - VCC -> 3.3V
 * - GND -> GND
 * - SCL -> GPIO 22
 * - SDA -> GPIO 21
 */
