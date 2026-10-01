#pragma once

#include <QString>

// 게임별 스타팅 포켓몬(resources/data/starters.json). 스쿼드에 스타팅이 둘 들어가면 경고하는 데
// 쓴다.
namespace com::yamada::studio::starters {
// 그 게임의 스타팅 계통 번호(0–2). 스타팅이 아니거나 표에 없는 게임이면 −1
int lineOf(const QString &versionGroup, int speciesId);
} // namespace com::yamada::studio::starters
