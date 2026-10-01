#pragma once

#include "core/types/type.h"

#include <optional>
#include <string_view>

namespace com::yamada::studio {
// 타입 ↔ PokéAPI identifier("dragon"). data 계층이 DB의 문자열을 core 타입으로 바꿀 때 쓴다.
std::string_view typeKey(Type type);
std::optional<Type> typeFromKey(std::string_view key); // 모르는 key("shadow", "")면 비어 있다
} // namespace com::yamada::studio
