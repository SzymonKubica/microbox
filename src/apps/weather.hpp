#pragma once
#include "../application_executor.hpp"
#include "../common/common_transitions.hpp"
#include "../common/configuration.hpp"
#include "../common/string_enum.hpp"
#include "geolocation.hpp"

#define AVAILABLE_CONFIGURATION_SLOTS 5

enum class WeatherAppAction : uint8_t {
        Fetch,
        UpdateLocation,
        AddNew,
};
namespace WeatherAppActionUtils
{
constexpr std::pair<WeatherAppAction, const char *> TABLE[] = {
    {WeatherAppAction::Fetch, "Fetch"},
    {WeatherAppAction::UpdateLocation, "Update Place"},
    {WeatherAppAction::AddNew, "Add New"}};
constexpr const char *to_cstr(WeatherAppAction action)
{
        return StrEnum::to_cstr(action, TABLE);
}
std::optional<WeatherAppAction> from_cstr(const char *str);
} // namespace WeatherAppActionUtils

struct WeatherAppConfiguration {
        ConfigurationHeader header;
        std::size_t curr_config_idx;
        int occupied_config_slots;
        char locations[AVAILABLE_CONFIGURATION_SLOTS][100];
        WeatherAppAction action;
        int forecast_days;

        std::vector<const char *> get_saved_locations() const;
};

struct WeatherDatapoint {
        std::string timestamp;
        float temperature;
        float rain;
        float precipitation_probability;
};

struct WeatherData {
        WeatherDatapoint current;
        std::vector<WeatherDatapoint> hourly;
};

class WeatherProvider
{
        const Platform &platform;

      public:
        WeatherProvider(const Platform &platform) : platform(platform) {}
        WeatherData get_weather_data(Location location, int forecast_days);
};

class WeatherApp : public ApplicationExecutor<WeatherAppConfiguration>,
                   public ThumbnailRenderer
{
      public:
        UserAction
        app_loop(const Platform &p,
                 const UserInterfaceCustomization &customization,
                 const WeatherAppConfiguration &config) const override;
        std::optional<UserAction>
        collect_config(const Platform &p,
                       const UserInterfaceCustomization &customization,
                       WeatherAppConfiguration &game_config) const override;
        const char *get_game_name() const override;
        const char *get_help_text() const override;
        void render_thumbnail(
            const Platform &platform,
            const UserInterfaceCustomization &customization) override;

        WeatherApp() {}
};
