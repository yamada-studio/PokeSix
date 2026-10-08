// H1 CP2 — footer 읽기와 "지금" 슬롯 고르기. ctest -R SaveBlock
// 합성 세이브를 쓰므로 CP1(crc16Ccitt)이 먼저 통과해야 한다.
#include "syntheticsave.h"

#include <gtest/gtest.h>

using namespace com::yamada::studio::save;
using synth::buildSave;
using synth::SlotSpec;

namespace {
const SaveLayout &kPlatinum = synth::layoutOf("platinum");
const SaveLayout &kHgss = synth::layoutOf("heartgold-soulsilver");
} // namespace

TEST(SaveBlock, ReadsFooterFields)
{
    const auto save = buildSave(kPlatinum, SlotSpec {.major = 7, .minor = 3});
    const BlockFooter footer = readFooter(save, 0, kPlatinum);
    EXPECT_EQ(footer.major, 7u);
    EXPECT_EQ(footer.minor, 3u);
    EXPECT_EQ(footer.size, kPlatinum.generalSize);
    EXPECT_EQ(footer.magic, 0x20060623u);
    EXPECT_TRUE(footer.crcOk());
}

TEST(SaveBlock, HeartGoldFooterCrcCoversTheMajorCounter)
{
    // HGSS의 footer는 0x10이라 끝 − 0x14(major)가 CRC 범위 안에 있다
    const auto save = buildSave(kHgss, SlotSpec {.major = 2});
    EXPECT_TRUE(readFooter(save, 0, kHgss).crcOk());
}

TEST(SaveBlock, CorruptedBlockFailsCrc)
{
    const auto save = buildSave(kPlatinum, SlotSpec {.corrupt = true});
    EXPECT_FALSE(readFooter(save, 0, kPlatinum).crcOk());
}

TEST(SaveBlock, PicksTheOnlyWrittenSlot)
{
    EXPECT_EQ(activeGeneralBlock(buildSave(kPlatinum, SlotSpec {}), kPlatinum), 0u);
    EXPECT_EQ(activeGeneralBlock(buildSave(kPlatinum, std::nullopt, SlotSpec {}), kPlatinum),
              kSlotSize);
}

TEST(SaveBlock, PicksTheHigherMajorCounter)
{
    EXPECT_EQ(activeGeneralBlock(buildSave(kPlatinum, SlotSpec {.major = 5}, SlotSpec {.major = 6}),
                                 kPlatinum),
              kSlotSize);
    EXPECT_EQ(activeGeneralBlock(buildSave(kPlatinum, SlotSpec {.major = 6}, SlotSpec {.major = 5}),
                                 kPlatinum),
              0u);
}

TEST(SaveBlock, MinorCounterBreaksATie)
{
    const auto save = buildSave(kPlatinum, SlotSpec {.major = 4, .minor = 9},
                                SlotSpec {.major = 4, .minor = 2});
    EXPECT_EQ(activeGeneralBlock(save, kPlatinum), 0u);
}

TEST(SaveBlock, IgnoresANewerSlotWithABadCrc)
{
    // 저장 도중 전원이 꺼진 경우: 더 최신이지만 깨진 슬롯 대신 이전 슬롯
    const auto save
            = buildSave(kPlatinum, SlotSpec {.major = 3}, SlotSpec {.major = 4, .corrupt = true});
    EXPECT_EQ(activeGeneralBlock(save, kPlatinum), 0u);
}

TEST(SaveBlock, BlankSaveHasNoActiveBlock)
{
    EXPECT_EQ(activeGeneralBlock(buildSave(kPlatinum, std::nullopt), kPlatinum), std::nullopt);
}

TEST(SaveBlock, BlockOutsideTheFileIsNeverValid)
{
    // 파일보다 큰 블록: 읽지 못한 footer는 저장 CRC · 계산 CRC가 둘 다 0이지만 "맞음"이 아니어야
    // 한다
    const std::vector<std::uint8_t> tiny(12, 0);
    EXPECT_FALSE(readFooter(tiny, 0, kPlatinum).crcOk());
    EXPECT_EQ(activeGeneralBlock(tiny, kPlatinum), std::nullopt);
}

TEST(SaveBlock, WrongGameLayoutNeverValidates)
{
    // 블록 크기가 다르면 CRC 범위 · footer 위치가 달라져 맞을 수 없다 → 이것이 게임 판별의 근거
    const auto save = buildSave(kPlatinum, SlotSpec {.major = 1}, SlotSpec {.major = 2});
    for (const SaveLayout &other : kGen4Layouts) {
        if (other.versionGroup != kPlatinum.versionGroup) {
            EXPECT_EQ(activeGeneralBlock(save, other), std::nullopt) << other.versionGroup;
        }
    }
}
