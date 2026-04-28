#pragma once
#include "controller.hpp"
#include "time_provider.hpp"
#include "display.hpp"
#include "http_client.hpp"
#include "persistent_storage.hpp"
#include "wifi.hpp"
#include <vector>

/**
 * Different platforms have different ways of identifying the buttons used
 * for the action controller. For instance, the Arduino input shield has
 * colored buttons and so when rendering control hints in the UI, we can use
 * those colors. On the other hand, if we are running on an emulator,
 * we might want to use the letters that correspond to the user input keys.
 * On top of that, MicroBox 2 doesn't have colored buttons, so we need to
 * refer to them using directions (e.g. arrow up icon). Each platform
 * is responsible for definint the flavour of the buttons that it relies on.
 * Depending on this selection, the UI hints and help text will be adjusted
 * accordingly.
 */
enum class ActionButtonKind {
        Colors = 0,
        Directions = 1,
        Letters = 2,
};

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
        ActionButtonKind action_button_kind = ActionButtonKind::Colors;
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
