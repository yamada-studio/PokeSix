#pragma once

#include "core/analysis/squadanalyzer.h"
#include "data/repository/repository.h"

#include <array>
#include <optional>

namespace com::yamada::studio {
// DB에서 읽은 값(문자열 identifier · Qt 컨테이너) → core 분석 입력. 계층 경계의 변환은 data가
// 한다(CLAUDE.md §4).
struct ResolvedMember
{
    QStringList types;      // 그 세대 타입
    QList<MoveEntry> moves; // 그 게임에서 배울 수 있는, 고른 기술(빈 칸 제외)
};

SquadInput makeSquadInput(const TypeChart &chart, int generation,
                          const std::array<std::optional<ResolvedMember>, kSquadSize> &members);
} // namespace com::yamada::studio
