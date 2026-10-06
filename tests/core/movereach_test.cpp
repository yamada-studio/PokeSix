#include "core/rules/movereach.h"

#include <gtest/gtest.h>

namespace com::yamada::studio::movereach {
namespace {
// 하트골드 · 소울실버 프테라(회색시티 화석, Lv 20). 기술 번호는 PokéAPI id.
enum : int {
    IceFang = 423,
    FireFang = 424,
    ThunderFang = 422,
    WingAttack = 17,
    Supersonic = 48,
    Bite = 44,
    ScaryFace = 184,
    Roar = 46,
    Agility = 97,
    AncientPower = 246,
};

std::vector<LevelMove> aerodactyl()
{
    return {{Roar, 9, 0},       {IceFang, 1, 1},      {FireFang, 1, 2}, {ThunderFang, 1, 3},
            {WingAttack, 1, 4}, {Supersonic, 1, 5},   {Bite, 1, 6},     {Agility, 17, 0},
            {ScaryFace, 1, 7},  {AncientPower, 25, 0}};
}

TEST(MoveReach, DefaultMovesetKeepsTheLastFour)
{
    EXPECT_EQ(defaultMoveset(aerodactyl(), 20),
              (std::vector<int> {Bite, ScaryFace, Roar, Agility}));
    EXPECT_EQ(defaultMoveset(aerodactyl(), 1),
              (std::vector<int> {WingAttack, Supersonic, Bite, ScaryFace}));
    EXPECT_TRUE(defaultMoveset({}, 50).empty());
}

TEST(MoveReach, DefaultMovesetCountsARepeatedMoveOnce)
{
    const std::vector<LevelMove> learnset = {{1, 1, 0}, {2, 5, 0}, {1, 9, 0}, {3, 12, 0}};
    EXPECT_EQ(defaultMoveset(learnset, 15), (std::vector<int> {1, 2, 3}));
}

TEST(MoveReach, MovesBelowTheCaughtLevelNeedTheDefaultSet)
{
    const Stage fossil {aerodactyl(), 20, 0};
    const Reach reach = reachableMoves(std::span(&fossil, 1));
    ASSERT_TRUE(reach.obtainable);
    EXPECT_EQ(reach.earliestLevel, 20);
    for (const int move : {Bite, ScaryFace, Roar, Agility, AncientPower})
        EXPECT_TRUE(reach.moves.contains(move)) << move;
    for (const int move : {IceFang, FireFang, ThunderFang, WingAttack, Supersonic})
        EXPECT_FALSE(reach.moves.contains(move)) << move;
}

TEST(MoveReach, EvolutionKeepsEarlierMovesAndLearnsFromTheEvolvedLevel)
{
    // 잉어킹(Lv 5) → 갸라도스(Lv 20 진화). 갸라도스의 Lv 1 난동부리기는 하트비늘, Lv 20 물기는 아님
    const std::vector<Stage> path = {
            {{{150, 1, 0}, {33, 15, 0}, {175, 30, 0}}, 5, 0},
            {{{37, 1, 0}, {44, 20, 0}, {82, 23, 0}}, std::nullopt, 20},
    };
    const Reach reach = reachableMoves(path);
    ASSERT_TRUE(reach.obtainable);
    EXPECT_EQ(reach.earliestLevel, 20);
    EXPECT_TRUE(reach.moves.contains(150)); // 잉어킹이 익힌 튀어오르기
    EXPECT_TRUE(reach.moves.contains(175)); // 진화를 미루고 잉어킹으로 Lv 30
    EXPECT_TRUE(reach.moves.contains(44));
    EXPECT_FALSE(reach.moves.contains(37));
}

TEST(MoveReach, ACheaperCatchBeatsEvolving)
{
    // 진화형을 Lv 10에 바로 잡을 수 있으면 그쪽이 가장 이르다
    const std::vector<Stage> path = {
            {{{1, 1, 0}}, 5, 0},
            {{{2, 1, 0}, {3, 12, 0}}, 10, 30},
    };
    const Reach reach = reachableMoves(path);
    EXPECT_EQ(reach.earliestLevel, 10);
    EXPECT_TRUE(reach.moves.contains(2)); // Lv 10으로 잡을 때 기본 기술
    EXPECT_TRUE(reach.moves.contains(3));
}

TEST(MoveReach, UnobtainableWhenNoStageCanBeCaught)
{
    const std::vector<Stage> path
            = {{{{1, 1, 0}}, std::nullopt, 0}, {{{2, 1, 0}}, std::nullopt, 16}};
    EXPECT_FALSE(reachableMoves(path).obtainable);
    EXPECT_FALSE(reachableMoves({}).obtainable);
}

TEST(MoveReach, EvolutionMovesAreLearnedOnEvolving)
{
    const std::vector<Stage> path = {{{{1, 1, 0}}, 3, 0}, {{{9, 0, 0}}, std::nullopt, 0}};
    EXPECT_TRUE(reachableMoves(path).moves.contains(9));
}
} // namespace
} // namespace com::yamada::studio::movereach
