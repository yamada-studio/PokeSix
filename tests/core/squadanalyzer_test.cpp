#include "core/analysis/squadanalyzer.h"
#include "core/rules/generationfeatures.h"
#include "core/types/typekey.h"

#include <gtest/gtest.h>

#include <tuple>

namespace com::yamada::studio {
namespace {
using enum Type;

// 2–5세대 상성표(1이 아닌 칸만). 실제 앱은 DB에서 읽는다 — 테스트는 core만으로 돌린다.
TypeMatrix chartForGeneration4()
{
    TypeMatrix chart;
    for (auto &row : chart)
        row.fill(1.0);
    const std::tuple<Type, Type, double> cells[] = {
            {Normal, Rock, .5},       {Normal, Ghost, 0},     {Normal, Steel, .5},
            {Fire, Fire, .5},         {Fire, Water, .5},      {Fire, Grass, 2},
            {Fire, Ice, 2},           {Fire, Bug, 2},         {Fire, Rock, .5},
            {Fire, Dragon, .5},       {Fire, Steel, 2},       {Water, Fire, 2},
            {Water, Water, .5},       {Water, Grass, .5},     {Water, Ground, 2},
            {Water, Rock, 2},         {Water, Dragon, .5},    {Electric, Water, 2},
            {Electric, Electric, .5}, {Electric, Grass, .5},  {Electric, Ground, 0},
            {Electric, Flying, 2},    {Electric, Dragon, .5}, {Grass, Fire, .5},
            {Grass, Water, 2},        {Grass, Grass, .5},     {Grass, Poison, .5},
            {Grass, Ground, 2},       {Grass, Flying, .5},    {Grass, Bug, .5},
            {Grass, Rock, 2},         {Grass, Dragon, .5},    {Grass, Steel, .5},
            {Ice, Fire, .5},          {Ice, Water, .5},       {Ice, Grass, 2},
            {Ice, Ice, .5},           {Ice, Ground, 2},       {Ice, Flying, 2},
            {Ice, Dragon, 2},         {Ice, Steel, .5},       {Fighting, Normal, 2},
            {Fighting, Ice, 2},       {Fighting, Poison, .5}, {Fighting, Flying, .5},
            {Fighting, Psychic, .5},  {Fighting, Bug, .5},    {Fighting, Rock, 2},
            {Fighting, Ghost, 0},     {Fighting, Dark, 2},    {Fighting, Steel, 2},
            {Poison, Grass, 2},       {Poison, Poison, .5},   {Poison, Ground, .5},
            {Poison, Rock, .5},       {Poison, Ghost, .5},    {Poison, Steel, 0},
            {Ground, Fire, 2},        {Ground, Electric, 2},  {Ground, Grass, .5},
            {Ground, Poison, 2},      {Ground, Flying, 0},    {Ground, Bug, .5},
            {Ground, Rock, 2},        {Ground, Steel, 2},     {Flying, Electric, .5},
            {Flying, Grass, 2},       {Flying, Fighting, 2},  {Flying, Bug, 2},
            {Flying, Rock, .5},       {Flying, Steel, .5},    {Psychic, Fighting, 2},
            {Psychic, Poison, 2},     {Psychic, Psychic, .5}, {Psychic, Dark, 0},
            {Psychic, Steel, .5},     {Bug, Fire, .5},        {Bug, Grass, 2},
            {Bug, Fighting, .5},      {Bug, Poison, .5},      {Bug, Flying, .5},
            {Bug, Psychic, 2},        {Bug, Ghost, .5},       {Bug, Dark, 2},
            {Bug, Steel, .5},         {Rock, Fire, 2},        {Rock, Ice, 2},
            {Rock, Fighting, .5},     {Rock, Ground, .5},     {Rock, Flying, 2},
            {Rock, Bug, 2},           {Rock, Steel, .5},      {Ghost, Normal, 0},
            {Ghost, Psychic, 2},      {Ghost, Ghost, 2},      {Ghost, Dark, .5},
            {Ghost, Steel, .5},       {Dragon, Dragon, 2},    {Dragon, Steel, .5},
            {Dark, Fighting, .5},     {Dark, Psychic, 2},     {Dark, Ghost, 2},
            {Dark, Dark, .5},         {Dark, Steel, .5},      {Steel, Fire, .5},
            {Steel, Water, .5},       {Steel, Electric, .5},  {Steel, Ice, 2},
            {Steel, Rock, 2},         {Steel, Steel, .5},
    };
    for (const auto &[attack, defense, multiplier] : cells)
        chart[std::size_t(attack)][std::size_t(defense)] = multiplier;
    return chart;
}

MoveInput move(Type type, DamageClass damageClass)
{
    return {type, damageClass};
}

// 04 §3 T1 — 샘플 스쿼드 "신오 정주행"
SquadInput sinnohRun()
{
    constexpr DamageClass P = DamageClass::Physical;
    constexpr DamageClass S = DamageClass::Special;
    constexpr DamageClass V = DamageClass::Status;
    SquadInput input;
    for (std::size_t i = 0; i < typeCount; ++i)
        if (typeExistsIn(Type(i), 4))
            input.types.push_back(Type(i));
    input.chart = chartForGeneration4();
    input.members[0] = MemberInput {
            {Fire, Fighting},
            {move(Fire, S), move(Fighting, P), move(Bug, P), move(Fighting, P)}}; // 초염몽
    input.members[1] = MemberInput {
            {Water, Steel},
            {move(Water, S), move(Ice, S), move(Steel, S), move(Grass, S)}}; // 엠페르트
    input.members[2] = MemberInput {
            {Dragon, Ground},
            {move(Ground, P), move(Dragon, P), move(Rock, P), move(Normal, V)}}; // 한카리아스
    input.members[3] = MemberInput {
            {Electric}, {move(Electric, P), move(Normal, P), move(Normal, V)}}; // 렌트라
    input.members[4] = MemberInput {
            {Normal, Flying},
            {move(Flying, S), move(Fighting, S), move(Rock, S), move(Electric, V)}}; // 토게키스
    return input;
}

TEST(SquadAnalyzer, ReceivedMultipliersMatchTheT1Table)
{
    const SquadAnalysis a = analyzeSquad(sinnohRun());
    EXPECT_EQ(a.filled, 5);
    EXPECT_DOUBLE_EQ(a.received[0][std::size_t(Bug)], .25);  // 초염몽 ← 벌레
    EXPECT_DOUBLE_EQ(a.received[1][std::size_t(Poison)], 0); // 엠페르트 ← 독
    EXPECT_DOUBLE_EQ(a.received[1][std::size_t(Ghost)], .5); // 4세대 강철은 고스트 반감
    EXPECT_DOUBLE_EQ(a.received[2][std::size_t(Ice)], 4);    // 한카리아스 ← 얼음
    EXPECT_DOUBLE_EQ(a.received[4][std::size_t(Ghost)], 0);  // 토게키스 ← 고스트
    EXPECT_DOUBLE_EQ(a.received[5][std::size_t(Fire)], 1);   // 빈 슬롯
}

TEST(SquadAnalyzer, CountsWeaknessesResistancesAndCoverage)
{
    const SquadAnalysis a = analyzeSquad(sinnohRun());
    // 노 불 물 풀 전 얼 격 독 땅 비 에 벌 바 고 드 악 강
    const std::pair<Type, std::array<int, 2>> expected[] = {
            {Normal, {0, 1}},   {Fire, {0, 2}},   {Water, {1, 1}},    {Grass, {0, 2}},
            {Electric, {2, 2}}, {Ice, {2, 2}},    {Fighting, {1, 0}}, {Poison, {0, 2}},
            {Ground, {3, 1}},   {Flying, {1, 2}}, {Psychic, {1, 1}},  {Bug, {0, 3}},
            {Rock, {1, 2}},     {Ghost, {0, 2}},  {Dragon, {1, 1}},   {Dark, {0, 2}},
            {Steel, {0, 3}},
    };
    for (const auto &[type, counts] : expected) {
        EXPECT_EQ(a.weak[std::size_t(type)], counts[0]) << typeKey(type);
        EXPECT_EQ(a.resist[std::size_t(type)], counts[1]) << typeKey(type);
        EXPECT_EQ(a.covered[std::size_t(type)], type != Ghost) << typeKey(type);
    }
}

TEST(SquadAnalyzer, FindsTheThreeProblemsInOrder)
{
    const SquadAnalysis a = analyzeSquad(sinnohRun());
    ASSERT_EQ(a.problems.size(), 3u);
    EXPECT_EQ(a.problems[0].kind, ProblemKind::WeakStack);
    EXPECT_EQ(a.problems[0].type, Ground);
    EXPECT_EQ(a.problems[0].members, (std::vector<int> {0, 1, 3}));
    EXPECT_EQ(a.problems[1].kind, ProblemKind::Quad);
    EXPECT_EQ(a.problems[1].type, Ice);
    EXPECT_EQ(a.problems[1].members, (std::vector<int> {2}));
    EXPECT_EQ(a.problems[2].kind, ProblemKind::NoCoverage);
    EXPECT_EQ(a.problems[2].type, Ghost);
}

TEST(SquadAnalyzer, SplitsMovesByTheGenerationRule)
{
    // 분류는 입력이 이미 그 세대 규칙으로 판정한 값이다 — 여기서는 세고 빈 칸만 더한다
    const SquadAnalysis a = analyzeSquad(sinnohRun());
    EXPECT_EQ(a.split.physical, 8);
    EXPECT_EQ(a.split.special, 8);
    EXPECT_EQ(a.split.status, 3);
    EXPECT_EQ(a.split.empty, 1);
}

TEST(SquadAnalyzer, EmptySquadHasNoProblems)
{
    SquadInput input = sinnohRun();
    input.members = {};
    const SquadAnalysis a = analyzeSquad(input);
    EXPECT_EQ(a.filled, 0);
    EXPECT_TRUE(a.problems.empty());
    // 기술이 하나도 없으면 공격 문제(커버 없음 17건)를 내지 않는다
    input.members[0] = MemberInput {{Water}, {}};
    EXPECT_TRUE(analyzeSquad(input).problems.empty());
}

TEST(GenerationFeatures, FollowTheGenerationTable)
{
    EXPECT_FALSE(featuresOf(1).heldItems);
    EXPECT_FALSE(featuresOf(1).specialSplit); // 1세대는 "특수" 하나
    EXPECT_TRUE(featuresOf(2).specialSplit);
    EXPECT_TRUE(featuresOf(2).heldItems);
    EXPECT_FALSE(featuresOf(2).abilities);
    EXPECT_TRUE(featuresOf(3).natures);
    EXPECT_FALSE(featuresOf(3).splitByMove);
    EXPECT_TRUE(featuresOf(4).splitByMove);
    EXPECT_FALSE(featuresOf(4).hiddenAbilities);
    EXPECT_TRUE(featuresOf(9).fairy);
    EXPECT_TRUE(featuresOf(1).hiddenMachines);
    EXPECT_TRUE(featuresOf(6).hiddenMachines); // XY · ORAS까지는 비전머신
    EXPECT_FALSE(featuresOf(7).hiddenMachines); // 7세대부터 내장 시스템(포켓라이드 …)
    EXPECT_FALSE(featuresOf(8).hiddenMachines); // BDSP 포함 — 포켓치가 대신한다
    EXPECT_FALSE(featuresOf(9).hiddenMachines);
    EXPECT_FALSE(typeExistsIn(Steel, 1));
    EXPECT_FALSE(typeExistsIn(Fairy, 5));
    EXPECT_TRUE(typeExistsIn(Fairy, 6));
    EXPECT_EQ(typeFromKey("electric"), Electric);
    EXPECT_FALSE(typeFromKey("shadow").has_value());
}
} // namespace
} // namespace com::yamada::studio
