// NDS 블록 검증 — H1 CP2(4세대식: footer · 슬롯) · H6 CP3(5세대식: 체크섬 모음 블록). ctest -R
// SaveBlock 합성 세이브를 쓰므로 H1 CP1(crc16Ccitt)이 먼저 통과해야 한다.
#include "syntheticsave.h"

#include <gtest/gtest.h>

using namespace com::yamada::studio::save;
using synth::buildSave;
using synth::buildTableSave;
using synth::SlotSpec;
using synth::TableSaveSpec;

namespace {
const NdsSeries &kPlatinumSeries = synth::seriesOf("platinum");
const NdsSeries &kHgssSeries = synth::seriesOf("heartgold-soulsilver");
const FooterSlots &kPlatinum = synth::footerSlotsOf(kPlatinumSeries);
const FooterSlots &kHgss = synth::footerSlotsOf(kHgssSeries);

const NdsSeries &kBwSeries = synth::seriesOf("black-white");
const NdsSeries &kB2w2Series = synth::seriesOf("black-2-white-2");
const ChecksumTable &kBw = synth::checksumTableOf(kBwSeries);
const ChecksumTable &kB2w2 = synth::checksumTableOf(kB2w2Series);
} // namespace

// ── 4세대식(FooterSlots) — H1 CP2 ───────────────────────────

TEST(SaveBlock, ReadsFooterFields)
{
    const auto save = buildSave(kPlatinumSeries, SlotSpec {.major = 7, .minor = 3});
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
    const auto save = buildSave(kHgssSeries, SlotSpec {.major = 2});
    EXPECT_TRUE(readFooter(save, 0, kHgss).crcOk());
}

TEST(SaveBlock, CorruptedBlockFailsCrc)
{
    const auto save = buildSave(kPlatinumSeries, SlotSpec {.corrupt = true});
    EXPECT_FALSE(readFooter(save, 0, kPlatinum).crcOk());
}

TEST(SaveBlock, PicksTheOnlyWrittenSlot)
{
    EXPECT_EQ(activeGeneralBlock(buildSave(kPlatinumSeries, SlotSpec {}), kPlatinum), 0u);
    EXPECT_EQ(activeGeneralBlock(buildSave(kPlatinumSeries, std::nullopt, SlotSpec {}), kPlatinum),
              kSlotSize);
}

TEST(SaveBlock, PicksTheHigherMajorCounter)
{
    EXPECT_EQ(activeGeneralBlock(
                      buildSave(kPlatinumSeries, SlotSpec {.major = 5}, SlotSpec {.major = 6}),
                      kPlatinum),
              kSlotSize);
    EXPECT_EQ(activeGeneralBlock(
                      buildSave(kPlatinumSeries, SlotSpec {.major = 6}, SlotSpec {.major = 5}),
                      kPlatinum),
              0u);
}

TEST(SaveBlock, MinorCounterBreaksATie)
{
    const auto save = buildSave(kPlatinumSeries, SlotSpec {.major = 4, .minor = 9},
                                SlotSpec {.major = 4, .minor = 2});
    EXPECT_EQ(activeGeneralBlock(save, kPlatinum), 0u);
}

TEST(SaveBlock, IgnoresANewerSlotWithABadCrc)
{
    // 저장 도중 전원이 꺼진 경우: 더 최신이지만 깨진 슬롯 대신 이전 슬롯
    const auto save = buildSave(kPlatinumSeries, SlotSpec {.major = 3},
                                SlotSpec {.major = 4, .corrupt = true});
    EXPECT_EQ(activeGeneralBlock(save, kPlatinum), 0u);
}

TEST(SaveBlock, BlankSaveHasNoActiveBlock)
{
    EXPECT_EQ(activeGeneralBlock(buildSave(kPlatinumSeries, std::nullopt), kPlatinum),
              std::nullopt);
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
    const auto save = buildSave(kPlatinumSeries, SlotSpec {.major = 1}, SlotSpec {.major = 2});
    for (const NdsSeries &other : kNdsSeries) {
        const auto *check = std::get_if<FooterSlots>(&other.check);
        if (check && other.versionGroup != kPlatinumSeries.versionGroup) {
            EXPECT_EQ(activeGeneralBlock(save, *check), std::nullopt) << other.versionGroup;
        }
    }
}

// ── 5세대식(ChecksumTable) — H6 CP3 ─────────────────────────

TEST(SaveBlock, ChecksumTableSitsAQuarterKilobyteBeforeTheEndOfTheMainSave)
{
    EXPECT_EQ(checksumTableStart(kBw), 0x23F00u);
    EXPECT_EQ(checksumTableStart(kB2w2), 0x25F00u);
}

TEST(SaveBlock, ValidatesTheChecksumTableOfItsOwnSeriesOnly) // H6-CP3-1
{
    const auto bw = buildTableSave(kBwSeries, TableSaveSpec {.party = {synth::MemberSpec {}}});
    EXPECT_TRUE(checksumTableValid(bw, kBw));
    EXPECT_FALSE(checksumTableValid(bw, kB2w2)); // 본 세이브 크기가 달라 모음 블록 자리가 다르다

    const auto b2w2 = buildTableSave(kB2w2Series, TableSaveSpec {.party = {synth::MemberSpec {}}});
    EXPECT_TRUE(checksumTableValid(b2w2, kB2w2));
    EXPECT_FALSE(checksumTableValid(b2w2, kBw));
}

TEST(SaveBlock, RejectsACorruptedChecksumTable) // H6-CP3-1
{
    const auto save = buildTableSave(
            kBwSeries, TableSaveSpec {.party = {synth::MemberSpec {}}, .corruptTable = true});
    EXPECT_FALSE(checksumTableValid(save, kBw));
}

TEST(SaveBlock, ChecksumTableOutsideTheFileIsNeverValid) // H6-CP3-1 · 2 — H1의 inFile 교훈
{
    const std::vector<std::uint8_t> tiny(16, 0);
    EXPECT_FALSE(checksumTableValid(tiny, kBw));
    EXPECT_FALSE(partyBlockValid(tiny, kBw));
}

TEST(SaveBlock, ChecksThePartyBlockCrcInBothPlaces) // H6-CP3-2
{
    EXPECT_TRUE(partyBlockValid(
            buildTableSave(kBwSeries, TableSaveSpec {.party = {synth::MemberSpec {}}}), kBw));
    auto save = buildTableSave(
            kBwSeries, TableSaveSpec {.party = {synth::MemberSpec {}}, .corruptParty = true});
    EXPECT_FALSE(partyBlockValid(save, kBw));

    // 블록 뒤의 CRC는 맞는데 모음 블록의 사본만 틀린 경우도 거절한다
    save = buildTableSave(kBwSeries, TableSaveSpec {.party = {synth::MemberSpec {}}});
    save[checksumTableStart(kBw) + kBw.partyCrcMirror] ^= 0xFF;
    EXPECT_FALSE(partyBlockValid(save, kBw));
}
