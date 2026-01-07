# Agents & AI Assistant Context

This document serves as a guide for AI agents and developers working on the `batterymonitor` project. It outlines best practices, workflows, and minimum requirements for the embedded system.

## Best Practices

### Code Quality & Structure
- **Modular Design**: Keep the `src/main.cpp` (or `main` file) minimal. Move logic into libraries within the `lib/` directory or separate components in `src/`.
- **Configuration**: Do not hardcode credentials (WiFi, MQTT). Use build flags (`-D`) in `platformio.ini` or a separate `secrets.h` file added to `.gitignore`.
- **Formatting**: Adhere to the project's `.clang-format` or standard C++ style guides.
- **Documentation**: Comment complex logic. Maintain `README.md` for high-level overview.

### PlatformIO Specifics
- **Dependency Management**: Define libraries in `platformio.ini` using `lib_deps`. Pin versions to ensure reproducibility.
- **Environments**: Use separate environments in `platformio.ini` for different boards or build types (e.g., `debug`, `release`, `ota`).

## Git Branching Strategy

We use a simplified Feature Branch Workflow:

- **`main`**: The stable, production-ready branch. Code here must compile and pass all tests.
- **`feature/<feature-name>`**: Created from `main` for new features. Merged back to `main` via Pull Request (PR).
- **`fix/<bug-name>`**: Created from `main` to address bugs.
- **`chore/<task-name>`**: For maintenance, documentation, or dependency updates.

**Rules:**
1.  Never push directly to `main`.
2.  All PRs require a successful build check.
3.  Commit messages should be imperative and descriptive (e.g., "Add MQTT reconnection logic" not "added mqtt").

## Basic Minimum for Embedded Project

For this project (and similar IoT nodes), the following features are mandatory foundation blocks:

### 1. Over-The-Air (OTA) Updates
- **Mechanism**: Must support updating firmware without physical access.
- **Safety**: Implement rollback or confirmation mechanisms if possible to prevent bricking remote devices.
- **Security**: Use password protection or signed binaries if exposed.

### 2. MQTT Publishing & Connectivity
- **State Reporting**: Periodically publish sensor data/state to topic `prefix/device_id/state`.
- **Availability**: Implement LWT (Last Will and Testament) on topic `prefix/device_id/status` (payload: `online`/`offline`) to detect disconnects.
- **Resilience**: Auto-reconnect logic for both WiFi and MQTT with exponential backoff.

### 3. Home Assistant (HA) Discovery
- **Auto-Configuration**: The device must self-register with Home Assistant via MQTT Discovery.
- **Topic Structure**: Publish config payloads to `homeassistant/<component>/<node_id>/<object_id>/config`.
- **Payload**: Include `name`, `state_topic`, `unique_id`, `device` (manufacturer, model), and `availability_topic`.
- **Benefit**: No manual YAML configuration required in Home Assistant.
