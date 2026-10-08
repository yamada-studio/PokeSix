// NDS 리더 — H1 CP5(4세대 파티 조립)를 옮겨 온 것 + H6 CP2 · CP4(시리즈 판별 · 5세대 파티).
// ctest -R PartyNdsReader
#include "core/save/partyndsreader.h"
#include "syntheticsave.h"

#include <gtest/gtest.h>

using namespace com::yamada::studio::save;
using synth::buildSave;
using synth::buildTableSave;
using synth::MemberSpec;
using synth::seriesOf;
using synth::SlotSpec;
using synth::TableSaveSpec;

// ── 표 ──────────────────────────────────────────────────────

TEST(PartyNdsReader, SeriesTableHoldsFourthAndFifthGeneration)
{
    ASSERT_EQ(kNdsSeries.size(), 5u);
    for (const NdsSeries &series : kNdsSeries) {
        ASSERT_NE(series.pkm, nullptr) << series.versionGroup;
        // 검증 방식 · 포켓몬 형식은 세대를 따라간다 — 표가 어긋나지 않았는지
        if (series.generation == 4) {
            EXPECT_TRUE(std::holds_alternative<FooterSlots>(series.check)) << series.versionGroup;
            EXPECT_EQ(series.pkm, &kPk4) << series.versionGroup;
        } else {
            EXPECT_TRUE(std::holds_alternative<ChecksumTable>(series.check)) << series.versionGroup;
            EXPECT_EQ(series.pkm, &kPk5) << series.versionGroup;
        }
    }
}

// ── 4세대(FooterSlots) — H1 CP5에서 옮김 · H6 CP2 ──────────

TEST(PartyNdsReader, ReadsAPlatinumParty) // H6-CP2-2 · 3
{
    MemberSpec a, b, c;
    a.species = 392;
    b.species = 398;
    b.pid = 0x0BADF00D;
    c.species = 448;
    c.pid = 0x7777AAAA;
    const NdsSeries &platinum = seriesOf("platinum");
    const auto save = buildSave(platinum, SlotSpec {.party = {a, b, c}});

    const auto party = PartyNdsReader::readSeries(save, platinum);
    ASSERT_TRUE(party.has_value());
    EXPECT_EQ(party->generation, 4);
    EXPECT_EQ(party->versionGroup, "platinum");
    EXPECT_EQ(party->partyOffset, platinum.partyOffset); // 슬롯 0의 파티
    ASSERT_EQ(party->members.size(), 3u);
    EXPECT_EQ(party->members[0].species, 392);
    EXPECT_EQ(party->members[1].species, 398);
    EXPECT_EQ(party->members[2].species, 448);
    for (const ReadMember &m : party->members)
        EXPECT_TRUE(m.checksumOk);
}

TEST(PartyNdsReader, ReadsTheNewestSlot) // H6-CP2-2 · 3
{
    MemberSpec old, new1, new2;
    old.species = 387;
    new1.species = 388;
    new2.species = 16;
    const NdsSeries &hgss = seriesOf("heartgold-soulsilver");
    const auto save = buildSave(hgss, SlotSpec {.major = 10, .party = {old}},
                                SlotSpec {.major = 11, .party = {new1, new2}});
    const auto party = PartyNdsReader::readSeries(save, hgss);
    ASSERT_TRUE(party.has_value());
    // 슬롯 1(major 11)이 최신 — 파티 위치가 그 슬롯 안이다
    EXPECT_EQ(party->partyOffset, kSlotSize + hgss.partyOffset);
    ASSERT_EQ(party->members.size(), 2u);
    EXPECT_EQ(party->members[0].species, 388);
}

TEST(PartyNdsReader, AcceptsADeSmuMEFooter) // H6-CP2-1 · 2 · 3
{
    auto save = buildSave(seriesOf("platinum"), SlotSpec {.party = {MemberSpec {}}});
    save.resize(kSaveSize + 122, 0); // .dsv 꼬리
    EXPECT_TRUE(PartyNdsReader {}.read(save).has_value());
}

TEST(PartyNdsReader, RejectsWhatIsNotASave)
{
    const PartyNdsReader reader;
    EXPECT_FALSE(reader.read(std::vector<std::uint8_t>(1000, 0)).has_value()); // 너무 작다
    EXPECT_FALSE(reader.read(std::vector<std::uint8_t>(kSaveSize, 0xFF)).has_value()); // 빈 플래시
    EXPECT_FALSE(reader.read(std::vector<std::uint8_t>(kSaveSize, 0x00)).has_value());
}

TEST(PartyNdsReader, RejectsAnEmptyParty)
{
    const NdsSeries &platinum = seriesOf("platinum");
    EXPECT_FALSE(
            PartyNdsReader::readSeries(buildSave(platinum, SlotSpec {}), platinum).has_value());
    const NdsSeries &bw = seriesOf("black-white");
    EXPECT_FALSE(PartyNdsReader::readSeries(buildTableSave(bw, TableSaveSpec {}), bw).has_value());
}

// ── 5세대(ChecksumTable) — H6 CP3 · CP4 ─────────────────────

TEST(PartyNdsReader, ReadsABlackWhiteParty) // H6-CP3 · CP4
{
    MemberSpec zoroark;
    zoroark.species = 571;
    zoroark.pid = 0x2313;   // % 25 = 4 — 5세대는 이 값을 쓰지 않는다
    zoroark.natureByte = 9; // 5세대의 진짜 성격: 9 = 촐랑(Lax)
    zoroark.hiddenAbility = true;
    zoroark.originGame = 21; // 블랙
    MemberSpec second;
    second.species = 495;
    second.pid = 0x0BADF00D;

    const NdsSeries &bw = seriesOf("black-white");
    const auto party = PartyNdsReader::readSeries(
            buildTableSave(bw, TableSaveSpec {.party = {zoroark, second}}), bw);
    ASSERT_TRUE(party.has_value());
    EXPECT_EQ(party->generation, 5);
    EXPECT_EQ(party->versionGroup, "black-white");
    EXPECT_EQ(party->partyOffset, 0x18E08u); // 기준 0(파일 시작) + 0x18E08
    ASSERT_EQ(party->members.size(), 2u);
    EXPECT_EQ(party->members[0].species, 571);
    EXPECT_EQ(party->members[0].nature, 9); // PID % 25(4)가 아니라 0x41 바이트
    EXPECT_TRUE(party->members[0].hiddenAbility);
    EXPECT_EQ(party->members[0].originGame, 21);
    EXPECT_EQ(party->members[1].species, 495);
    EXPECT_FALSE(party->members[1].hiddenAbility);
    for (const ReadMember &m : party->members)
        EXPECT_TRUE(m.checksumOk);
}

TEST(PartyNdsReader, RejectsAPartyBlockThatDoesNotCheckOut) // H6-CP3
{
    const NdsSeries &bw = seriesOf("black-white");
    EXPECT_FALSE(
            PartyNdsReader::readSeries(buildTableSave(bw, TableSaveSpec {.party = {MemberSpec {}},
                                                                         .corruptParty = true}),
                                       bw)
                    .has_value());
}

// ── 판별: 다섯 시리즈 모두 · 서로 헷갈리지 않는다 ──────────

TEST(PartyNdsReader, DetectsEverySeries) // H6-CP2 · CP3 · CP4 — 전부 채워야 초록
{
    const PartyNdsReader reader;
    for (const NdsSeries &series : kNdsSeries) {
        const auto save
                = std::holds_alternative<FooterSlots>(series.check)
                          ? buildSave(series, SlotSpec {.party = {MemberSpec {}}})
                          : buildTableSave(series, TableSaveSpec {.party = {MemberSpec {}}});
        const auto party = reader.read(save);
        ASSERT_TRUE(party.has_value()) << series.versionGroup;
        EXPECT_EQ(party->generation, series.generation) << series.versionGroup;
        EXPECT_EQ(party->versionGroup, series.versionGroup);
    }
}

TEST(PartyNdsReader, NoSeriesReadsAnotherSeriesSave) // H6-CP2 · CP3 — 같은 512 KiB
{
    for (const NdsSeries &written : kNdsSeries) {
        const auto save
                = std::holds_alternative<FooterSlots>(written.check)
                          ? buildSave(written, SlotSpec {.party = {MemberSpec {}}})
                          : buildTableSave(written, TableSaveSpec {.party = {MemberSpec {}}});
        for (const NdsSeries &other : kNdsSeries) {
            if (other.versionGroup != written.versionGroup) {
                EXPECT_FALSE(PartyNdsReader::readSeries(save, other).has_value())
                        << written.versionGroup << " read as " << other.versionGroup;
            }
        }
    }
}
