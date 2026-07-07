#pragma once
#include "../application_executor.hpp"
#include "../common/common_transitions.hpp"
#include "../common/configuration.hpp"
#include <string.h>

enum class WeatherQueryType : uint8_t { Current, Forecast, Both };

namespace WeatherQueryTypeUtils
{
constexpr std::pair<WeatherQueryType, const char *> TABLE[] = {
    {WeatherQueryType::Current, "Current"},
    {WeatherQueryType::Forecast, "Forecast"},
    {WeatherQueryType::Both, "Both"}};
constexpr const char *to_cstr(WeatherQueryType type)
{
        for (auto [t, str] : TABLE) {
                if (t == type)
                        return str;
        }
        return "UNKNOWN";
}
std::optional<WeatherQueryType> from_cstr(const char *str);
} // namespace WeatherQueryTypeUtils

enum class WeatherAppAction : uint8_t {
        Fetch,
        UpdateLocation,
};

namespace WeatherAppActionUtils
{
constexpr std::pair<WeatherAppAction, const char *> TABLE[] = {
    {WeatherAppAction::Fetch, "Fetch"},
    {WeatherAppAction::UpdateLocation, "Update Location"}};
constexpr const char *to_cstr(WeatherAppAction action)
{
        for (auto [a, str] : TABLE) {
                if (a == action)
                        return str;
        }
        return "UNKNOWN";
}
std::optional<WeatherAppAction> from_cstr(const char *str);
} // namespace WeatherAppActionUtils

enum class WeatherMetric : uint8_t {
        Temperature,
        Rain,
        PrecipitationProbability
};

struct WeatherAppConfiguration {
        ConfigurationHeader header;
        char location[100];
        WeatherQueryType query_type;
        WeatherAppAction action;
        int forecast_days;
};

class WeatherApp : public ApplicationExecutor<WeatherAppConfiguration>
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

        WeatherApp() {}
};
