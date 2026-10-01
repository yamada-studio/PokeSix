#pragma once

#include "core/types/type.h"

#include <cstdint>

namespace com::yamada::studio {
// 기술 분류. 값은 PokéAPI move_damage_classes.id(1 변화 · 2 물리 · 3 특수)와 같다.
enum class DamageClass : std::uint8_t {
    Status = 1,
    Physical = 2,
    Special = 3,
};

// 3세대까지의 규칙: 공격 기술의 물리 · 특수는 기술이 아니라 타입이 정했다
// (노말 · 격투 · 비행 · 독 · 땅 · 바위 · 벌레 · 고스트 · 강철 = 물리, 나머지 = 특수).
constexpr DamageClass typeBasedClass(Type type)
{
    switch (type) {
    case Type::Normal:
    case Type::Fighting:
    case Type::Flying:
    case Type::Poison:
    case Type::Ground:
    case Type::Rock:
    case Type::Bug:
    case Type::Ghost:
    case Type::Steel:
        return DamageClass::Physical;
    default:
        return DamageClass::Special;
    }
}
} // namespace com::yamada::studio
