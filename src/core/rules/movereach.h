#pragma once

#include <optional>
#include <set>
#include <span>
#include <vector>

namespace com::yamada::studio::movereach {
// 한 게임 안에서 포켓몬이 "기술 떠올리기(하트비늘) 없이" 익힐 수 있는 레벨업 기술을 센다.
//
// 길은 세 가지다: 얻을 때 기본으로 가진 기술, 얻은 뒤 레벨업, 진화 전 단계가 익혀 둔 기술.
// 교배(알)는 정주행 동선이 아니라서 세지 않는다.

struct LevelMove
{
    int move = 0;
    int level = 0; // 0 = 진화할 때 배우는 기술(8세대 이후)
    int order = 0; // 같은 레벨 안에서 게임이 익히는 순서
};

// 게임이 레벨 level의 포켓몬을 줄 때 가진 기술: level 이하의 레벨업 기술을 (레벨 · 순서)대로
// 익히고, 4개를 넘으면 가장 먼저 익힌 것을 잊는다. 같은 기술은 한 번만 센다.
std::vector<int> defaultMoveset(std::span<const LevelMove> learnset, int level);

struct Stage
{
    std::vector<LevelMove> learnset;
    std::optional<int> caughtLevel; // 이 게임에서 잡기 · 받기로 얻는 가장 낮은 레벨
    int evolveLevel = 0; // 앞 단계에서 진화할 수 있는 가장 낮은 레벨(레벨 조건이 없으면 0)
};

struct Reach
{
    bool obtainable = false; // 이 게임에서 (진화를 거쳐서라도) 얻을 수 있다
    int earliestLevel = 0;   // 가장 일찍 가질 수 있는 레벨
    std::set<int> moves;     // 하트비늘 없이 익힐 수 있는 기술
};

// path: 진화 사슬의 뿌리 → 대상 순서. 진화 전 단계는 원하는 레벨까지 키운 뒤 진화시킬 수 있다고
// 본다(B 버튼으로 미루기).
Reach reachableMoves(std::span<const Stage> path);
} // namespace com::yamada::studio::movereach
