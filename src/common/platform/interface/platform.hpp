#pragma once
#include "controller.hpp"
#include "delay.hpp"
#include "display.hpp"
#include "http_client.hpp"
#include "persistent_storage.hpp"
#include "wifi.hpp"
#include <vector>

/**
 * Structure encapsulating all interfaces that a given implementation of the
 * game console platform needs to provide so that we can run our games on it.
 */
struct Platform {
        Display *display;
        std::vector<DirectionalController*> *directional_controllers;
        std::vector<ActionController*> *action_controllers;
        DelayProvider *delay_provider;
        PersistentStorage *persistent_storage;
        WifiProvider *wifi_provider;
        HttpClient *client;
};
