#include "core/analysis/resourceledger.h"

#include <gtest/gtest.h>

namespace com::yamada::studio::resourceledger {
namespace {
TEST(ResourceLedger, CountsHeartScales)
{
    const std::vector<TeachPlan> plans = {{0, Means::Reminder, {}, {}},
                                          {2, Means::Reminder, {}, {}},
                                          {3, Means::LevelUp, {}, {}}};
    const Ledger ledger = tally(plans, {});
    EXPECT_EQ(ledger.heartScales, 2);
    EXPECT_TRUE(ledger.machines.empty());
    EXPECT_TRUE(ledger.totals.empty());
}

TEST(ResourceLedger, WarnsWhenOneDropMachineIsPlacedTwice)
{
    // HGSS 기준: 한 번만 얻는 기술머신(필드 1개)을 두 자리에 배치
    const std::vector<TeachPlan> plans
            = {{0, Means::Machine, "tm59", {}}, {4, Means::Machine, "tm59", {}}};
    const std::map<std::string, MachineSupply> supplies = {{"tm59", {true, 1, false, {}, false}}};
    const Ledger ledger = tally(plans, supplies);
    ASSERT_EQ(ledger.machines.size(), 1u);
    const MachineUse &use = ledger.machines.front();
    EXPECT_EQ(use.members, (std::vector<int> {0, 4}));
    EXPECT_TRUE(use.shortfall);
    EXPECT_EQ(use.bought, 0);
    EXPECT_TRUE(ledger.totals.empty());
}

TEST(ResourceLedger, BuysTheMissingCopiesWhenRepeatable)
{
    // 지진(TM26): 필드 1개 + 배틀프런티어 80BP 교환 — 세 자리면 2개를 산다
    const std::vector<TeachPlan> plans = {{0, Means::Machine, "tm26", {}},
                                          {1, Means::Machine, "tm26", {}},
                                          {5, Means::Machine, "tm26", {}}};
    const std::map<std::string, MachineSupply> supplies
            = {{"tm26", {true, 1, true, {{80, "bp"}}, false}}};
    const Ledger ledger = tally(plans, supplies);
    ASSERT_EQ(ledger.machines.size(), 1u);
    EXPECT_FALSE(ledger.machines.front().shortfall);
    EXPECT_EQ(ledger.machines.front().bought, 2);
    ASSERT_EQ(ledger.totals.size(), 1u);
    EXPECT_EQ(ledger.totals.front().amount, 160);
    EXPECT_EQ(ledger.totals.front().unit, "bp");
}

TEST(ResourceLedger, FreeCopiesCoverTheUsesSilently)
{
    const std::vector<TeachPlan> plans = {{0, Means::Machine, "tm13", {}}};
    const std::map<std::string, MachineSupply> supplies
            = {{"tm13", {true, 1, true, {{10000, "coins"}}, true}}};
    const Ledger ledger = tally(plans, supplies);
    ASSERT_EQ(ledger.machines.size(), 1u);
    EXPECT_FALSE(ledger.machines.front().shortfall);
    EXPECT_EQ(ledger.machines.front().bought, 0);
    EXPECT_TRUE(ledger.machines.front().supply.postGameOnly); // 입수 사정은 그대로 넘어온다
    EXPECT_TRUE(ledger.totals.empty());
}

TEST(ResourceLedger, UnknownSupplyNeverWarns)
{
    const std::vector<TeachPlan> plans
            = {{0, Means::Machine, "tm44", {}}, {1, Means::Machine, "tm44", {}}};
    const Ledger ledger = tally(plans, {});
    ASSERT_EQ(ledger.machines.size(), 1u);
    EXPECT_FALSE(ledger.machines.front().shortfall);
    EXPECT_EQ(ledger.machines.front().bought, 0);
}

TEST(ResourceLedger, SumsTutorAndMachineCostsPerUnit)
{
    const std::vector<TeachPlan> plans = {
            {0, Means::Tutor, {}, {{48, "bp"}}},
            {1, Means::Tutor, {}, {{32, "bp"}}},
            {2, Means::Tutor, {}, {{2, "red-shard"}, {2, "blue-shard"}}},
            {3, Means::Machine, "tm06", {}},
    };
    const std::map<std::string, MachineSupply> supplies
            = {{"tm06", {true, 0, true, {{80, "bp"}}, false}}};
    const Ledger ledger = tally(plans, supplies);
    ASSERT_EQ(ledger.totals.size(), 3u); // 단위 이름 순: blue-shard · bp · red-shard
    EXPECT_EQ(ledger.totals[0].unit, "blue-shard");
    EXPECT_EQ(ledger.totals[0].amount, 2);
    EXPECT_EQ(ledger.totals[1].unit, "bp");
    EXPECT_EQ(ledger.totals[1].amount, 160);
    EXPECT_EQ(ledger.totals[2].unit, "red-shard");
    EXPECT_EQ(ledger.totals[2].amount, 2);
}

TEST(ResourceLedger, CountsAMachineOncePerSlot)
{
    const std::vector<TeachPlan> plans
            = {{2, Means::Machine, "tm26", {}}, {2, Means::Machine, "tm26", {}}};
    const std::map<std::string, MachineSupply> supplies = {{"tm26", {true, 1, false, {}, false}}};
    const Ledger ledger = tally(plans, supplies);
    ASSERT_EQ(ledger.machines.size(), 1u);
    EXPECT_EQ(ledger.machines.front().members, (std::vector<int> {2}));
    EXPECT_FALSE(ledger.machines.front().shortfall);
}
} // namespace
} // namespace com::yamada::studio::resourceledger
