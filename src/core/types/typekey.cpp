#include "core/types/typekey.h"

#include <array>

namespace com::yamada::studio {
namespace {
// Type 열거 순서(= PokéAPI type id 1–18)와 같은 순서
constexpr std::array<std::string_view, typeCount> kKeys = {
        "normal", "fighting", "flying", "poison",   "ground",  "rock", "bug",    "ghost", "steel",
        "fire",   "water",    "grass",  "electric", "psychic", "ice",  "dragon", "dark",  "fairy"};
} // namespace

std::string_view typeKey(Type type)
{
    return kKeys[static_cast<std::size_t>(type)];
}

std::optional<Type> typeFromKey(std::string_view key)
{
    for (std::size_t i = 0; i < kKeys.size(); ++i)
        if (kKeys[i] == key)
            return static_cast<Type>(i);
    return std::nullopt;
}
} // namespace com::yamada::studio
