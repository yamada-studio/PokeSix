#pragma once

#include "core/types/type.h"

namespace com::yamada::studio {
// 세대마다 있고 없는 것. `if (gen >= 6)`을 곳곳에 두지 않고 이 표 하나에서 읽는다(CLAUDE.md §4).
struct GenerationFeatures
{
    bool heldItems = false;    // 지닌 물건(2세대 금 · 은부터)
    bool specialSplit = false; // 특공 · 특방이 나뉨(2세대부터). 1세대는 "특수" 하나
    bool abilities = false;    // 특성(3세대 RS부터)
    bool natures = false;      // 성격(3세대부터)
    bool hiddenAbilities = false; // 숨겨진 특성(5세대 BW부터)
    bool darkAndSteel = false;    // 악 · 강철 타입(2세대부터)
    bool fairy = false; // 페어리 타입(6세대 XY부터). 강철이 고스트 · 악을 반감하지 않게 됨
    bool splitByMove = false; // 물리 · 특수를 기술마다 판정(4세대 DP부터). 아니면 타입이 정한다
};

inline constexpr int kFirstGeneration = 1;
inline constexpr int kLastGeneration = 9;

constexpr GenerationFeatures featuresOf(int generation)
{
    // 1세대 · 2세대 · 3세대 · 4세대 · 5세대 · 6세대 이후
    constexpr GenerationFeatures kTable[] = {
            {},
            {.heldItems = true, .specialSplit = true, .darkAndSteel = true},
            {.heldItems = true,
             .specialSplit = true,
             .abilities = true,
             .natures = true,
             .darkAndSteel = true},
            {.heldItems = true,
             .specialSplit = true,
             .abilities = true,
             .natures = true,
             .darkAndSteel = true,
             .splitByMove = true},
            {.heldItems = true,
             .specialSplit = true,
             .abilities = true,
             .natures = true,
             .hiddenAbilities = true,
             .darkAndSteel = true,
             .splitByMove = true},
            {.heldItems = true,
             .specialSplit = true,
             .abilities = true,
             .natures = true,
             .hiddenAbilities = true,
             .darkAndSteel = true,
             .fairy = true,
             .splitByMove = true},
    };
    constexpr int kRows = int(sizeof(kTable) / sizeof(kTable[0]));
    if (generation < kFirstGeneration)
        return kTable[0];
    return kTable[(generation > kRows ? kRows : generation) - 1];
}

// 그 세대에 있는 타입인가(1세대: 악 · 강철 · 페어리 없음, 2–5세대: 페어리 없음)
constexpr bool typeExistsIn(Type type, int generation)
{
    const GenerationFeatures features = featuresOf(generation);
    if (type == Type::Dark || type == Type::Steel)
        return features.darkAndSteel;
    if (type == Type::Fairy)
        return features.fairy;
    return true;
}
} // namespace com::yamada::studio
