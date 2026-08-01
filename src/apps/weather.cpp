#include <chrono>
#include <cstring>
#include <cassert>

#include "../common/logging.hpp"
#include "../lib/ArduinoJson-v7.4.2.h"
#include "../common/maths_utils.hpp"
#include "../menu.hpp"
#include "weather.hpp"
#include "settings.hpp"

#define TAG "random_seed_picker"

#define CONTROL_POLLING_DELAY 10
#define LOOP_DELAY 30

WeatherAppConfiguration DEFAULT_WEATHER_APP_CONFIG = {
    .header = {.magic = CONFIGURATION_MAGIC, .version = 2},
    .curr_config_idx = 0,
    .occupied_config_slots = 3,
    .locations = {"Peniche, PT", "London, UK", "Sosnowa 3, Kamieniec, PL"},
    .action = WeatherAppAction::Fetch,
    .forecast_days = 5};

UserAction random_seed_picker_loop(Platform *p,
                                   UserInterfaceCustomization *customization);

const char *WeatherApp::get_game_name() const { return "Weather App"; }
const char *WeatherApp::get_help_text() const { return "TODO"; }

UserAction
handle_update_location(const Platform &p,
                       const UserInterfaceCustomization &customization,
                       const WeatherAppConfiguration &config);
UserAction handle_fetch(const Platform &p,
                        const UserInterfaceCustomization &customization,
                        const WeatherAppConfiguration &config);
UserAction handle_add_new(const Platform &p,
                          const UserInterfaceCustomization &customization,
                          const WeatherAppConfiguration &config);
UserAction WeatherApp::app_loop(const Platform &p,
                                const UserInterfaceCustomization &customization,
                                const WeatherAppConfiguration &config) const
{
        switch (config.action) {
        case WeatherAppAction::UpdateLocation:
                return handle_update_location(p, customization, config);
        case WeatherAppAction::Fetch:
                return handle_fetch(p, customization, config);
        case WeatherAppAction::AddNew:
                return handle_add_new(p, customization, config);
        }
        return UserAction::PlayAgain;
}

WeatherAppConfiguration *
load_initial_weather_app_config(PersistentStorage *storage);
UserAction
handle_update_location(const Platform &p,
                       const UserInterfaceCustomization &customization,
                       const WeatherAppConfiguration &config)
{
        char *location;
        auto maybe_interrupt = collect_string_input(
            p, customization, "Enter location:", &location);
        if (maybe_interrupt.has_value()) {
                UserAction action = maybe_interrupt.value();
                if (action == UserAction::Exit) {
                        LOG_DEBUG(TAG, "User cancelled modifying the weather "
                                       "query location.");
                }
                return action;
        }

        WeatherAppConfiguration copy = config;
        // We need to ensure that the header version is not outdated
        copy.header.version = DEFAULT_WEATHER_APP_CONFIG.header.version;
        const auto storage = p.persistent_storage;
        sprintf(copy.locations[config.curr_config_idx], "%s", location);
        int storage_offset = get_settings_storage_offset(Game::WeatherApp);

        // Before saving the updated values we need to restore the default
        // action to avoid overwriting it with the 'update location' action that
        // we have received in the current `config`
        auto initial_config = std::unique_ptr<WeatherAppConfiguration>(
            load_initial_weather_app_config(p.persistent_storage));
        copy.action = initial_config->action;

        storage->put(storage_offset, copy);

        return UserAction::PlayAgain;
}

std::vector<std::string> get_month_day_labels(std::chrono::year_month_day start,
                                              int days);
std::chrono::year_month_day from_timestamp_string(std::string timestamp);
void render_weather_data(const Platform &p, const WeatherDatapoint &datapoint,
                         int forecast_days_count, bool refresh_values_only);

int get_hour_from_timestamp(std::string timestamp);
UserAction handle_fetch(const Platform &p,
                        const UserInterfaceCustomization &customization,
                        const WeatherAppConfiguration &config)
{

        GeolocationProvider geolocation{p};
        WeatherProvider weather{p};

        std::string location_description =
            std::string(config.locations[config.curr_config_idx]);
        LOG_DEBUG(TAG, "Fetching weather data for location: %s",
                  location_description.c_str());

        Location location = geolocation.search_location(location_description);
        auto data = weather.get_weather_data(location, config.forecast_days);

        auto [time, temp, rain, precipitation] = data.current;

        p.display->clear(Color::Black);
        render_weather_data(p, data.current, config.forecast_days, false);

        // This is needed for the x-axis day labels on the graph.
        std::chrono::year_month_day today = from_timestamp_string(time);
        std::vector<std::string> labels =
            get_month_day_labels(today, config.forecast_days);

        // We need to determine the index of the current time in the set of
        // hourly datapoints. Note that the weather API returns hourly
        // datapoints starting from the beginning of today (00:00) and so our
        // current datapoint is not equal to the firsts datapoint in the hourly
        // forecast. This index is then used to highlight the current datapoint
        // in the bar graph.
        int curr_hour = get_hour_from_timestamp(time);
        int curr_idx = data.hourly.size();
        for (int i = 0; i < data.hourly.size(); i++) {
                int hour = get_hour_from_timestamp(data.hourly[i].timestamp);
                // we break as soon as we get a match on the hour as
                // `timestamps`
                // are hourly datapoint. For example, if the current datapoint
                // is for 5:45, we want to pick up the 5:00 hourly datapoint and
                // highlight that on the graph.
                if (curr_hour == hour) {
                        curr_idx = i;
                        break;
                }
        }
        int current_time_idx = curr_idx;

        // We need to extract the list of temperatures for the y-values on the
        // graph.
        std::vector<float> temperatures(data.hourly.size());
        std::transform(
            data.hourly.begin(), data.hourly.end(), temperatures.begin(),
            [](WeatherDatapoint datapoint) { return datapoint.temperature; });

        // Note: the Y-start is hand-callibrated. This will be dynamically
        // calculated in the proper solution.
        int y_start =
            p.display->get_font_configuration().font_dimensions.height * 7;
        render_bar_graph(p, customization, y_start, labels, temperatures,
                         curr_idx);

        auto move_datapoint_selection = [&](int prev_idx, int new_idx) {
                render_bar_graph(p, customization, y_start, labels,
                                 temperatures, prev_idx,
                                 customization.accent_color, true);
                render_bar_graph(p, customization, y_start, labels,
                                 temperatures, new_idx, Color::Red, true);

                const auto &datapoint = new_idx == current_time_idx
                                            ? data.current
                                            : data.hourly[new_idx];
                render_weather_data(p, datapoint, config.forecast_days, true);
        };

        bool input_registered_last_iteration = false;
        while (true) {
                if (!p.display->refresh()) {
                        return UserAction::CloseWindow;
                }
                auto maybe_direction =
                    poll_directional_input(p.directional_controllers);
                auto maybe_action = poll_action_input(p.action_controllers);
                if (!maybe_direction.has_value() && !maybe_action.has_value()) {
                        input_registered_last_iteration = false;
                        p.time_provider->delay_ms(CONTROL_POLLING_DELAY);
                        continue;
                }

                if (maybe_direction.has_value()) {
                        Direction dir = maybe_direction.value();
                        if (dir == Direction::LEFT) {
                                int prev = curr_idx;
                                curr_idx = modulo_decrement(curr_idx,
                                                            data.hourly.size());
                                move_datapoint_selection(prev, curr_idx);
                        } else if (dir == Direction::RIGHT) {
                                int prev = curr_idx;
                                curr_idx = modulo_increment(curr_idx,
                                                            data.hourly.size());
                                move_datapoint_selection(prev, curr_idx);
                        }
                }

                if (maybe_action.has_value()) {
                        Action act = maybe_action.value();
                        // If the user holds the button depressed, we still only
                        // act once to avoid double-processing of slow presses
                        // caused by button debounce issues.
                        if (input_registered_last_iteration) {
                                p.time_provider->delay_ms(
                                    CONTROL_POLLING_DELAY);
                                continue;
                        }
                        switch (act) {
                        case CONFIRM_ACTION: {
                                int prev = curr_idx;
                                curr_idx = current_time_idx;
                                move_datapoint_selection(prev, curr_idx);
                        } break;
                        case BACK_ACTION:
                                p.time_provider->delay_ms(LOOP_DELAY);
                                return UserAction::PlayAgain;
                        case FORWARD_ACTION: {
                                int prev = curr_idx;
                                curr_idx = modulo_increment(curr_idx,
                                                            data.hourly.size());
                                move_datapoint_selection(prev, curr_idx);
                                // Wait a bit longer on forward scroll on button
                                // presses. This is needed for higher precision
                                // steering of the highlight selection.
                                p.time_provider->delay_ms(2 * LOOP_DELAY);
                        } break;
                        case HELP_ACTION:
                                const char *message =
                                    "Use the joystick to scroll through hourly "
                                    "datapoints. Press down to go back to the "
                                    "current time. Press left to advance "
                                    "slowly advance by one datapoint. Press "
                                    "right to exit.";
                                render_wrapped_help_text(p, customization,
                                                         message);
                                wait_until_green_pressed(p);
                                p.display->clear(Black);
                                render_weather_data(p, data.current,
                                                    config.forecast_days,
                                                    false);
                                render_bar_graph(p, customization, y_start,
                                                 labels, temperatures,
                                                 curr_idx);
                        }
                }
                input_registered_last_iteration = true;

                // We wait slightly longer after an action is
                // selected.
                p.time_provider->delay_ms(LOOP_DELAY);
        }

        auto maybe_interrupt = wait_until_green_pressed(p);
        // Applicable on the emulator only: if the window is closed
        // while waiting for input we need to propagate the
        // `CloseWindow` action back to the top level so that the SFML
        // window can be closed properly.
        if (maybe_interrupt.has_value()) {
                return maybe_interrupt.value();
        }
        return UserAction::PlayAgain;
}

void render_weather_data(const Platform &p, const WeatherDatapoint &datapoint,
                         int forecast_days_count, bool refresh_values_only)
{
        auto [fw, fh] = p.display->get_font_configuration().font_dimensions;
        auto [time, temp, rain, precipitation] = datapoint;

        const char *time_heading = "Time: ";
        const char *temperature_heading = "Tempearture: ";
        const char *precipitation_heading = "Rain Probability: ";
        const char *forecast_days_heading = "Forecast ";

        char time_buffer[20];
        char temperature_buffer[10];
        char precipitation_buffer[10];
        char forecast_days_buffer[10];

        sprintf(time_buffer, "%s", time.c_str());
        sprintf(temperature_buffer, "%.1f Cel.", temp);
        sprintf(precipitation_buffer, "%.1f %%", precipitation);
        sprintf(forecast_days_buffer, "%d days:", forecast_days_count);

        std::vector<std::pair<const char *, char *>> headings_and_values = {
            {time_heading, time_buffer},
            {temperature_heading, temperature_buffer},
            {precipitation_heading, precipitation_buffer},
            {forecast_days_heading, forecast_days_buffer},
        };

        // Start with a decent margin.
        Point first_line_start{fw, fw};
        Point start = first_line_start;

        for (auto [heading, value] : headings_and_values) {
                auto value_start = start + Point{(int)strlen(heading) * fw, 0};

                if (refresh_values_only) {
                        // We need to erase a bit further in case a numerical
                        // value flips from double digits to a single digit,
                        // which will shorten it. If that happens, the current
                        // strlen(value) not enough and we need to erase
                        // further to reach the end of the previous value.
                        auto value_end =
                            value_start +
                            Point{((int)strlen(value) + 2) * fw, fh};
                        p.display->clear_region(value_start, value_end,
                                                Color::Black);
                } else {
                        p.display->draw_string(start, (char *)heading,
                                               FontSize::Size16, Color::Black,
                                               Color::White);
                }

                p.display->draw_string(value_start, value, FontSize::Size16,
                                       Color::Black, Color::White);

                start = start + Point{0, fh + 5};
        }
}

std::chrono::year_month_day from_timestamp_string(std::string timestamp)
{
        std::istringstream iss{timestamp};
        std::chrono::year_month_day ymd;
        iss >> std::chrono::parse("%FT%R", ymd);
        return ymd;
}

/**
 * This assumes timestamps formatted like: "2026-07-19T21:00".
 */
int get_hour_from_timestamp(std::string timestamp)
{
        return std::stoi(timestamp.substr(11, 2));
}
std::vector<std::string> get_month_day_labels(std::chrono::year_month_day start,
                                              int day_count)
{
        using namespace std::chrono;
        std::vector<std::string> labels;
        sys_days sd{start};
        for (int i = 0; i < day_count; i++) {
                char buffer[6];
                year_month_day curr{sd};
                sprintf(buffer, "%d-%d", unsigned(curr.month()),
                        unsigned(curr.day()));
                labels.push_back(std::string(buffer));
                sd += days{1};
        }
        return labels;
}

UserAction handle_add_new(const Platform &p,
                          const UserInterfaceCustomization &customization,
                          const WeatherAppConfiguration &config)
{
        if (config.occupied_config_slots >= AVAILABLE_CONFIGURATION_SLOTS) {
                render_wrapped_text(
                    p, customization,
                    "You have reached the maximum number of saved "
                    "locations. Please overwrite an existing location "
                    "to add a "
                    "new one.");
                auto maybe_interrupt = wait_until_green_pressed(p);
                if (maybe_interrupt.has_value()) {
                        return maybe_interrupt.value();
                }
                return UserAction::PlayAgain;
        }

        char *location;
        auto maybe_interrupt = collect_string_input(
            p, customization, "Enter location:", &location);
        if (maybe_interrupt.has_value()) {
                UserAction action = maybe_interrupt.value();
                if (action == UserAction::Exit) {
                        LOG_DEBUG(TAG, "User cancelled modifying the weather "
                                       "query location.");
                }
                return action;
        }

        WeatherAppConfiguration copy = config;
        // We need to ensure that the header version is not outdated
        copy.header.version = DEFAULT_WEATHER_APP_CONFIG.header.version;
        const auto storage = p.persistent_storage;
        int new_config_idx = config.occupied_config_slots;
        copy.occupied_config_slots++;
        sprintf(copy.locations[new_config_idx], "%s", location);
        int storage_offset = get_settings_storage_offset(Game::WeatherApp);

        // Before saving the updated values we need to restore the
        // default action to avoid overwriting it with the 'add new'
        // action that we have received in the current `config`
        auto initial_config = std::unique_ptr<WeatherAppConfiguration>(
            load_initial_weather_app_config(p.persistent_storage));
        copy.action = initial_config->action;

        storage->put(storage_offset, copy);

        return UserAction::PlayAgain;
}

WeatherAppConfiguration *
load_initial_weather_app_config(PersistentStorage *storage)
{

        int storage_offset = get_settings_storage_offset(Game::WeatherApp);

        WeatherAppConfiguration config;
        LOG_DEBUG(TAG,
                  "Trying to load initial settings from the persistent "
                  "storage "
                  "at offset %d",
                  storage_offset);
        storage->get(storage_offset, config);

        WeatherAppConfiguration *output = new WeatherAppConfiguration();

        if (!config.header.validate_against(DEFAULT_WEATHER_APP_CONFIG)) {
                LOG_DEBUG(TAG,
                          "The storage does not contain a valid "
                          "weather app configuration, using default values.");
                memcpy(output, &DEFAULT_WEATHER_APP_CONFIG,
                       sizeof(WeatherAppConfiguration));
                storage->put(storage_offset, DEFAULT_WEATHER_APP_CONFIG);

        } else {
                LOG_DEBUG(TAG, "Using configuration from persistent storage.");
                memcpy(output, &config, sizeof(WeatherAppConfiguration));
        }

        return output;
}

Configuration *assemble_weather_app_configuration(
    const Platform &p, const WeatherAppConfiguration &initial_config);
void extract_weather_app_config(WeatherAppConfiguration &initial_config,
                                WeatherAppConfiguration &weather_query_config,
                                const Configuration &config);

std::optional<UserAction>
WeatherApp::collect_config(const Platform &p,
                           const UserInterfaceCustomization &customization,
                           WeatherAppConfiguration &game_config) const
{
        auto initial_config = std::unique_ptr<WeatherAppConfiguration>(
            load_initial_weather_app_config(p.persistent_storage));
        auto config = std::unique_ptr<Configuration>(
            assemble_weather_app_configuration(p, *initial_config));

        auto maybe_interrupt_action =
            collect_configuration(p, *config, customization);
        if (maybe_interrupt_action) {
                return maybe_interrupt_action;
        }

        extract_weather_app_config(*initial_config, game_config, *config);
        return std::nullopt;
}

/**
 * Returns the saved configurations as a vector. This is used to allow
 * for more convenient processing even though the actual representation
 * needs to be a simple array so that we can safely serialize it to raw
 * bytes in EEPROM.
 */
std::vector<const char *> WeatherAppConfiguration::get_saved_locations() const
{
        std::vector<const char *> output;
        for (std::size_t i = 0; i < occupied_config_slots; i++) {
                output.push_back(this->locations[i]);
        }
        return output;
}

Configuration *assemble_weather_app_configuration(
    const Platform &p, const WeatherAppConfiguration &initial_config)
{
        std::vector<const char *> availabe_actions = {
            WeatherAppActionUtils::to_cstr(WeatherAppAction::Fetch),
            WeatherAppActionUtils::to_cstr(WeatherAppAction::UpdateLocation),
            WeatherAppActionUtils::to_cstr(WeatherAppAction::AddNew),
        };

        auto *action = ConfigurationOption::of_strings(
            ">", availabe_actions,
            WeatherAppActionUtils::to_cstr(initial_config.action));

        int occupied_slots = initial_config.occupied_config_slots;
        std::vector<const char *> locations =
            initial_config.get_saved_locations();

        auto *location = ConfigurationOption::of_strings(
            "Where", locations, locations[initial_config.curr_config_idx]);
        auto *forecast_days = ConfigurationOption::of_integers(
            "Days", {1, 2, 3, 4, 5}, initial_config.forecast_days);

        return new Configuration("Weather", {forecast_days, location, action});
}

void extract_weather_app_config(WeatherAppConfiguration &initial_config,
                                WeatherAppConfiguration &weather_query_config,
                                const Configuration &config)
{

        ConfigurationOption *forecast_days = config.options[0];
        // TODO: add support for storing multiple locations
        ConfigurationOption *location = config.options[1];
        ConfigurationOption *action = config.options[2];

        weather_query_config.forecast_days =
            forecast_days->get_curr_int_value();
        int selected_location_idx = location->currently_selected;
        weather_query_config.action =
            WeatherAppActionUtils::from_cstr(action->get_current_str_value())
                .value();
        weather_query_config.curr_config_idx = selected_location_idx;
        weather_query_config.occupied_config_slots =
            initial_config.occupied_config_slots;
        memcpy(weather_query_config.locations, initial_config.locations,
               sizeof(char[AVAILABLE_CONFIGURATION_SLOTS][100]));

        // We need to ensure that the header version is not outdated
        weather_query_config.header.version =
            DEFAULT_WEATHER_APP_CONFIG.header.version;
}

const std::string HOST = "api.open-meteo.com";
const std::string BASE_URL = "https://api.open-meteo.com//v1/forecast?";

const std::string METRICS_TO_QUERY =
    "current=temperature_2m,rain,precipitation_probability&hourly="
    "temperature_"
    "2m,rain,"
    "precipitation_probability&timezone=auto";

std::string assemble_query_url(Location location, int forecast_days);
WeatherData parse_weather_data(std::string response);
WeatherData WeatherProvider::get_weather_data(Location location,
                                              int forecast_days)
{
        ConnectionConfig config{HOST, 443};
        std::string query_url = assemble_query_url(location, forecast_days);
        auto maybe_response = platform.client->get(config, query_url);

        if (!maybe_response.has_value()) {
                LOG_ERROR(TAG, "Failed to fetch weather data!");
        }

        WeatherData data = parse_weather_data(maybe_response.value());
        return data;
}

std::string assemble_query_url(Location location, int forecast_days)
{
        std::string url = std::string(BASE_URL);
        url += "latitude=" + std::to_string(location.latitude);
        url += "&";
        url += "longitude=" + std::to_string(location.longitude);
        url += "&";
        url += METRICS_TO_QUERY;
        url += "&";
        url += "forecast_days=" + std::to_string(forecast_days);
        return url;
}

WeatherData parse_weather_data(std::string response)
{
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, response);
        if (err) {
                LOG_DEBUG(TAG, "Failed to parse response %s", err.c_str());
                return {};
        }

        const auto &current = doc["current"];
        WeatherDatapoint current_datapoint = {
            .timestamp = current["time"],
            .temperature = current["temperature_2m"].as<float>(),
            .rain = current["rain"].as<float>(),
            .precipitation_probability =
                current["precipitation_probability"].as<float>(),
        };

        const auto &hourly = doc["hourly"];
        const auto &time = hourly["time"];
        const auto &rain = hourly["rain"];
        const auto &temperature = hourly["temperature_2m"];
        const auto &precipitation_probability =
            hourly["precipitation_probability"];

        int size = hourly["time"].size();
        auto empty = WeatherDatapoint{};
        std::vector<WeatherDatapoint> hourly_datapoints(size, empty);
        for (int i = 0; i < size; i++) {
                hourly_datapoints[i] = {
                    time[i],
                    temperature[i].as<float>(),
                    rain[i].as<float>(),
                    precipitation_probability[i].as<float>(),
                };
        }

        return {current_datapoint, hourly_datapoints};
}
std::optional<WeatherAppAction>
WeatherAppActionUtils::from_cstr(const char *str)
{
        return StrEnum::from_cstr(str, TABLE);
}

void WeatherApp::render_thumbnail(
    const Platform &platform, const UserInterfaceCustomization &customization)
{
        const auto &display = *platform.display;
        clear_half_display_and_render_subtitle(platform, customization,
                                               "Weather");

        TftCompatibleDisplay &tft =
            *platform.display->cast_into_tft_compatible();
        // [BEGIN lopaka generated]
        // ellipse 22
        tft.fillEllipse(142, 119, 16, 16, 0xFF47);
        // ellipse 18
        tft.fillEllipse(158, 135, 14, 16, 0xFFFF);
        // ellipse 18 copy 1
        tft.fillEllipse(181, 144, 13, 13, 0xFFFF);
        // ellipse 18 copy 2
        tft.fillEllipse(138, 146, 11, 11, 0xFFFF);
        // rect 21
        tft.fillRect(139, 146, 43, 12, 0xFFFF);
        // ellipse 18 copy 3
        tft.fillEllipse(163, 149, 8, 8, 0xEF7D);
        // ellipse 18 copy 4
        tft.fillEllipse(172, 155, 8, 8, 0xEF7D);
        // ellipse 18 copy 5
        tft.fillEllipse(154, 155, 8, 8, 0xEF7D);
        // rect 26
        tft.fillRect(155, 153, 15, 11, 0xEF7D);
        // line 27
        tft.drawLine(157, 164, 157, 170, 0x24BE);
        // line 27 copy 1
        tft.drawLine(162, 164, 162, 170, 0x24BE);
        // line 27 copy 2
        tft.drawLine(167, 164, 167, 170, 0x24BE);
        // ellipse 18 copy 6
        tft.fillEllipse(186, 111, 8, 8, 0xEF7D);
        // ellipse 18 copy 7
        tft.fillEllipse(177, 112, 9, 9, 0xEF7D);
        // ellipse 18 copy 8
        tft.fillEllipse(186, 118, 8, 8, 0xEF7D);
        // line 27 copy 3
        tft.drawLine(172, 164, 172, 170, 0x24BE);
        // line 27 copy 4
        tft.drawLine(152, 164, 152, 170, 0x24BE);
        // line 27 copy 5
        tft.drawLine(177, 162, 177, 168, 0x24BE);
        // line 27 copy 6
        tft.drawLine(182, 158, 182, 164, 0x24BE);
        // line 27 copy 7
        tft.drawLine(187, 157, 187, 163, 0x24BE);
        // line 27 copy 8
        tft.drawLine(147, 160, 147, 166, 0x24BE);
        // line 27 copy 9
        tft.drawLine(142, 158, 142, 164, 0x24BE);
        // line 27 copy 10
        tft.drawLine(137, 158, 137, 164, 0x24BE);
        // [END lopaka generated]
}
