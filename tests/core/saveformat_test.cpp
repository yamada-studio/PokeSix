// H6 — 형식 목록으로 세대를 모르는 세이브 읽기(① 판별의 입구). ctest -R SaveFormat
#include "core/save/saveformat.h"
#include "syntheticsave.h"

#include <gtest/gtest.h>

using namespace com::yamada::studio::save;
using synth::MemberSpec;

TEST(SaveFormat, ListsTheGenerationsInOrder) // H6-CP2-1
{
    const auto formats = saveFormats();
    ASSERT_EQ(formats.size(), 2u);
    EXPECT_EQ(formats[0].generation, 4);
    EXPECT_EQ(formats[1].generation, 5);
    for (const SaveFormat &format : formats) {
        EXPECT_NE(format.read, nullptr);
        EXPECT_FALSE(format.name.empty());
    }
}

TEST(SaveFormat, ReadsAFourthGenerationSave) // H6-CP2-2 (+ CP1)
{
    const auto party = readParty(synth::buildSave(synth::layoutOf("heartgold-soulsilver"),
                                                  synth::SlotSpec {.party = {MemberSpec {}}}));
    ASSERT_TRUE(party.has_value());
    EXPECT_EQ(party->generation, 4);
    EXPECT_EQ(party->versionGroup, "heartgold-soulsilver");
}

TEST(SaveFormat, ReadsAFifthGenerationSave) // H6-CP2-2 + CP3 · CP4
{
    const auto party
            = readParty(synth::buildGen5Save(synth::gen5LayoutOf("black-2-white-2"),
                                             synth::Gen5SaveSpec {.party = {MemberSpec {}}}));
    ASSERT_TRUE(party.has_value());
    EXPECT_EQ(party->generation, 5);
    EXPECT_EQ(party->versionGroup, "black-2-white-2");
}

TEST(SaveFormat, RejectsWhatNoFormatRecognises) // H6-CP2-2
{
    EXPECT_FALSE(readParty(std::vector<std::uint8_t>(kSaveSize, 0x00)).has_value());
    EXPECT_FALSE(readParty(std::vector<std::uint8_t>(kSaveSize, 0xFF)).has_value());
    EXPECT_FALSE(readParty(std::vector<std::uint8_t>(100, 0)).has_value());
}
