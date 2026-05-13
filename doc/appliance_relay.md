# Appliance Relay Management Implementation Plan

## Objective
Implement an appliance relay management feature controlled via MQTT. The appliance will remain off if the battery voltage falls below a dedicated threshold. The relay state will be maintained during the ESP32's deep sleep mode using RTC GPIO hold capabilities.

## Key Files & Context
- `lib/BatteryMonitor/battery_config.h`: Will house new constants for the appliance relay GPIO pin (must be RTC-capable) and the low-voltage cutoff threshold.
- `lib/ConfigManager/config_manager.h`: Needs to store the intended relay state (ON/OFF) in NVS to ensure state recovery after deep sleep wakeup.
- `lib/NetworkManager/network_manager.h` & `network_manager.cpp`: Will handle subscribing to the MQTT command topic (`battery/monitor/appliance/set`), publishing the current state (`.../appliance/state`), and publishing the Home Assistant Discovery configuration for a Switch entity.
- `src/main.cpp`: Will orchestrate reading the battery voltage, evaluating it against the cutoff threshold, applying the intended state to the GPIO pin, and enabling `rtc_gpio_hold_en` before entering deep sleep.

## Implementation Steps

### 1. Configuration Additions
In `lib/BatteryMonitor/battery_config.h`:
- Define `APPLIANCE_RELAY_PIN` (e.g., GPIO 26 or 27 - must be an RTC GPIO).
- Define `APPLIANCE_CUTOFF_VOLTAGE` within the respective battery type namespaces (e.g., 12.2V for Lead-Acid, 12.8V for LiFePO4).

### 2. State Persistence
In `lib/ConfigManager/config_manager.h`:
- Add a boolean variable `applianceTargetState` (default `false`).
- Update `begin()`, `saveConfig()`, and `resetToDefaults()` to persist this variable in NVS under the key `"appl_state"`.

### 3. MQTT and Home Assistant Integration
In `lib/NetworkManager/network_manager.h` & `network_manager.cpp`:
- **Subscription:** Subscribe to the appliance command topic (`.../appliance/set`).
- **Callback:** In `mqttCallback`, intercept commands sent to the appliance topic (e.g., "ON", "OFF", "1", "0", "true", "false"). Update `config.applianceTargetState` and call `config.saveConfig()`.
- **Publish State:** In `publishReading()`, evaluate the *actual* appliance state (which may be OFF despite the target state if the battery is too low) and publish it to `.../appliance/state`.
- **HA Discovery:** In `publishHomeAssistantDiscovery()`, add a JSON configuration payload for a Home Assistant Switch entity targeting the appliance topics.

### 4. Hardware Control & Deep Sleep Retention
In `src/main.cpp`:
- Include `driver/rtc_io.h` and `driver/gpio.h`.
- Disable GPIO hold on boot using `gpio_hold_dis()` / `rtc_gpio_hold_dis()` so the pin can be reconfigured.
- Initialize `APPLIANCE_RELAY_PIN` as an `OUTPUT`.
- After calling `monitor.readBattery()`, evaluate the rule:
  `bool actualState = config.applianceTargetState && (reading.voltage >= Config::Voltage::APPLIANCE_CUTOFF_VOLTAGE);`
- Write `actualState` to the `APPLIANCE_RELAY_PIN`.
- In `enterDeepSleep()`, right before `esp_deep_sleep_start()`, enable GPIO hold for the relay pin using `rtc_gpio_hold_en(APPLIANCE_RELAY_PIN)` to maintain the relay state while the processor sleeps.

## Verification & Testing
- **MQTT Command:** Send an "ON" command via MQTT and verify the intended state is saved and applied.
- **Deep Sleep:** Trigger deep sleep and verify (using a multimeter on the assigned pin) that the pin retains its HIGH or LOW state during sleep.
- **Voltage Cutoff:** Simulate a low battery voltage below `APPLIANCE_CUTOFF_VOLTAGE` and verify the relay turns OFF, regardless of the MQTT-commanded target state.
- **HA Discovery:** Verify a new Switch entity appears in Home Assistant and correctly controls the target state.
\ No newline at end of file