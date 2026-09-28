#include "core/types/typechart.h"

#include <gtest/gtest.h>

namespace com::yamada::studio {
namespace {

TEST(TypeChart, FireIsSuperEffectiveAgainstGrass)
{
    const TypeChart chart;
    EXPECT_DOUBLE_EQ(chart.multiplier(Type::Fire, Type::Grass), 2.0);
}

} // namespace
} // namespace com::yamada::studio
