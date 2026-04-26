#pragma once
#include "controller.hpp"
#include "time_provider.hpp"
#include "display.hpp"
#include "http_client.hpp"
#include "persistent_storage.hpp"
#include "wifi.hpp"
#include <vector>

/**
 * Bundles all features that are not available on every platform. The code
 * then relies on that to conditionally enable certain features.
 *
 * For instance, if our platform doesn't have wifi support, we shouldn't
 * make wifi settings avaialble to the users.
 */
struct PlatformCapabilities {
        bool has_wifi = true;
        bool can_sleep = true;
};

/**
 * A platform structure contains all components required to run our games
 * on that platform. The idea is that different target platforms (e.g. emulator
 * / ESP32 target device) will define their own implementations of the required
 * components of the platform.
 *
 * For example, a PC emulator might implement a display using SFML and have
 * directional/action controllers that work based on keyboard input.
 */
struct Platform {
        Display *display;
        std::vector<DirectionalController *> directional_controllers;
        std::vector<ActionController *> action_controllers;
        TimeProvider *time_provider;
        PersistentStorage *persistent_storage;
        WifiProvider *wifi_provider;
        HttpClient *client;
        PlatformCapabilities capabilities;
};

