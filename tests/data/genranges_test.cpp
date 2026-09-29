#include "data/update/genranges.h"

#include <gtest/gtest.h>

using namespace com::yamada::studio::genranges;

TEST(GenRanges, CurrentValueOnlyIsOneOpenRange)
{
    const auto ranges = toGenRanges<int>(4, {}, 200);
    ASSERT_EQ(ranges.size(), 1u);
    EXPECT_EQ(ranges[0], (GenRange<int> {4, std::nullopt, 200}));
}

TEST(GenRanges, PastThenCurrent)
{
    // 고스트 → 에스퍼: 1세대까지 ×0, 지금 ×2
    const auto ranges = toGenRanges<int>(1, {{1, 0}}, 200);
    ASSERT_EQ(ranges.size(), 2u);
    EXPECT_EQ(ranges[0], (GenRange<int> {1, 1, 0}));
    EXPECT_EQ(ranges[1], (GenRange<int> {2, std::nullopt, 200}));
}

TEST(GenRanges, SeveralPastRowsInAnyOrder)
{
    const auto ranges = toGenRanges<int>(1, {{5, 70}, {2, 50}}, 90);
    ASSERT_EQ(ranges.size(), 3u);
    EXPECT_EQ(ranges[0], (GenRange<int> {1, 2, 50}));
    EXPECT_EQ(ranges[1], (GenRange<int> {3, 5, 70}));
    EXPECT_EQ(ranges[2], (GenRange<int> {6, std::nullopt, 90}));
}

TEST(GenRanges, PastOnlyEndsTheRange)
{
    // 1세대 전용 능력치 "특수": 옛 값만 있고 현재 값은 없다
    const auto ranges = toGenRanges<int>(1, {{1, 65}}, std::nullopt);
    ASSERT_EQ(ranges.size(), 1u);
    EXPECT_EQ(ranges[0], (GenRange<int> {1, 1, 65}));
}

TEST(GenRanges, PastBeforeFirstGenIsIgnored)
{
    // 2세대에 처음 나온 대상에 "1세대까지" 옛 값이 있으면 쓸 수 없다
    EXPECT_TRUE(toGenRanges<int>(2, {{1, 65}}, std::nullopt).empty());
}

TEST(GenRanges, EqualNeighboursAreMerged)
{
    const auto ranges = toGenRanges<int>(1, {{3, 50}, {5, 50}}, 50);
    ASSERT_EQ(ranges.size(), 1u);
    EXPECT_EQ(ranges[0], (GenRange<int> {1, std::nullopt, 50}));
}
