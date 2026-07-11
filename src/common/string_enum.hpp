#pragma once
#include <optional>
#include <utility>

// Shared utilities for mapping enum values to/from string representations.

namespace StrEnum
{

/**
 * Converts an enum variant to its corresponding string representation
 * given a provided name lookup table.
 */
template <typename Enum, std::size_t N>
constexpr const char *
to_cstr(Enum variant, const std::pair<Enum, const char *> (&name_table)[N])
{
        for (auto [v, s] : name_table) {
                if (variant == v) {
                        return s;
                }
        }
        return "UNKNOWN";
}
/**
 * 'Deserializes' a string representation of an enum variant to its
 * corresponding enum value given a provided name lookup table. Returns
 * std::nullopt if the string does not match any of the known enum variant
 * names.
 */
template <typename Enum, std::size_t N>
std::optional<Enum>
from_cstr(const char *str, const std::pair<Enum, const char *> (&name_table)[N])
{
        for (auto [v, s] : name_table) {
                if (strcmp(s, str) == 0) {
                        return v;
                }
        }
        return std::nullopt;
}

} // namespace StrEnum
