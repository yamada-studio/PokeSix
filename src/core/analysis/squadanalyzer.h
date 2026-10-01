#pragma once

#include "core/rules/damageclass.h"
#include "core/types/type.h"

#include <array>
#include <optional>
#include <vector>

namespace com::yamada::studio {
// SixSquad 분석(02 SCR-04 · 04 §3 T1). 6자리 × 타입 18 — 계산은 동기, 매번 처음부터 한다.
//
// 상성표는 입력으로 받는다(그 세대 표는 data 계층이 DB에서 읽어 넘긴다). core는 세대를 몰라도
// 된다: 어떤 타입이 있는지(types)와 배율(chart)만 보면 된다.
inline constexpr std::size_t kSquadSize = 6;
inline constexpr std::size_t kMoveSlots = 4;

using TypeMatrix = std::array<std::array<double, typeCount>, typeCount>; // [공격][방어]

struct MoveInput
{
    std::optional<Type> type;                      // 없음 = ??? 타입(저주 2–4세대)
    DamageClass damageClass = DamageClass::Status; // 그 세대 규칙의 분류
    DamageClass otherRuleClass = DamageClass::Status; // 반대 규칙(타입 기준 ↔ 기술 기준)이었다면
};

struct MemberInput
{
    std::vector<Type> types;      // 1–2개
    std::vector<MoveInput> moves; // 0–4개(빈 칸은 넣지 않는다)
};

struct SquadInput
{
    std::vector<Type> types; // 그 세대에 있는 타입(분석 열). 순서 = 문제 목록 안에서의 순서
    TypeMatrix chart {}; // 없는 칸은 1
    std::array<std::optional<MemberInput>, kSquadSize> members;
};

enum class ProblemKind {
    WeakStack, // 방어: 약점 3마리 이상
    NoResist, // 방어: 약점 2마리 이상인데 받아낼(내성 · 무효) 포켓몬이 없다
    Quad,     // 방어: 누군가 ×4
    NoCoverage, // 공격: 그 타입을 효과가 굉장하게 칠 기술이 하나도 없다
};

struct Problem
{
    ProblemKind kind = ProblemKind::WeakStack;
    Type type = Type::Normal; // 방어 문제 = 공격해 오는 타입, 공격 문제 = 상대(방어) 타입
    std::vector<int> members; // 관련 슬롯(0–5). 방어 = 약점인 슬롯(Quad는 ×4인 슬롯)
};

struct MoveSplit
{
    int physical = 0;
    int special = 0;
    int status = 0;
    int empty = 0; // 채운 포켓몬의 빈 기술 칸
};

struct SquadAnalysis
{
    int filled = 0;
    // 받는 배율 [슬롯][공격 타입]. 빈 슬롯 · 세대에 없는 타입은 1
    std::array<std::array<double, typeCount>, kSquadSize> received {};
    std::array<int, typeCount> weak {};   // 약점(> 1)인 포켓몬 수
    std::array<int, typeCount> resist {}; // 내성 · 무효(< 1)인 포켓몬 수
    std::array<bool, typeCount> covered {}; // 그 (방어) 타입을 효과가 굉장하게 칠 기술이 있다
    bool hasAttackingMove = false; // 공격 기술이 하나라도 있다(없으면 공격 문제를 내지 않는다)
    std::vector<Problem> problems; // 방어(WeakStack → NoResist → Quad) → 공격 순
    MoveSplit split;
    MoveSplit otherRuleSplit;
};

SquadAnalysis analyzeSquad(const SquadInput &input);
} // namespace com::yamada::studio
