#pragma once

#include <cstddef>
#include <cstdint>

namespace com::yamada::studio {

// 열거 순서는 PokéAPI 의 type id (1 ~ 18) 와 일치시킨다.
// DB/API 매핑 시 `static_cast<Type>(apiId - 1)` 로 변환할 수 있게 하기 위함.
// 순서를 바꾸면 매핑이 깨지므로 주의.
enum class Type : std::uint8_t {
    Normal,   //  1
    Fighting, //  2
    Flying,   //  3
    Poison,   //  4
    Ground,   //  5
    Rock,     //  6
    Bug,      //  7
    Ghost,    //  8
    Steel,    //  9  (2세대~)
    Fire,     // 10
    Water,    // 11
    Grass,    // 12
    Electric, // 13
    Psychic,  // 14
    Ice,      // 15
    Dragon,   // 16
    Dark,     // 17  (2세대~)
    Fairy,    // 18  (6세대~)
};

inline constexpr std::size_t typeCount = 18;

} // namespace com::yamada::studio
