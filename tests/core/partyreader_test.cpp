// 부모(기기와 상관없는 쪽) — H1 CP4 필드 읽기 · H6 CP1 기기 리더 등록 · CP4 PK5 칸. ctest -R
// PartyReader 시리즈별 읽기(슬롯 · 게임 판별)는 partyndsreader_test.cpp
#include "core/save/partyreader.h"
#include "syntheticsave.h"

#include <gtest/gtest.h>

using namespace com::yamada::studio::save;
using synth::buildSave;
using synth::buildTableSave;
using synth::MemberSpec;
using synth::SlotSpec;
using synth::TableSaveSpec;

namespace {
ReadMember roundTrip(const MemberSpec &spec)
{
    return parseMember(decodePkm(synth::encodePkm(synth::plainPkm(spec))));
}

// PK5: 세이브에 있는 모양 그대로 220바이트로 잘라서 푼다
ReadMember roundTripPk5(const MemberSpec &spec)
{
    const Pkm encrypted = synth::encodePkm(synth::plainPkm(spec));
    return parseMember(decodePkm(Bytes(encrypted).first(kPk5.partySize)), kPk5);
}
} // namespace

// ── CP4 parseMember ─────────────────────────────────────────

TEST(PartyReader, ParsesEveryField)
{
    MemberSpec spec;
    spec.pid = 0x8F3A21C7; // % 25 = 23 (신중)
    spec.species = 479;    // 로토무
    spec.heldItem = 0x00EA;
    spec.ability = 26;            // 부유
    spec.formByte = (2 << 3) | 4; // 폼 2 · 성별 비트
    spec.moves = {85, 247, 109, 0};
    spec.evs = {4, 0, 252, 0, 252, 0};
    // 개체값 HP 31 · 공격 0 · 방어 15 · 스피드 20 · 특공 1 · 특방 30
    spec.iv32 = 31u | (0u << 5) | (15u << 10) | (20u << 15) | (1u << 20) | (30u << 25);
    spec.level = 47;
    spec.hp = 88;
    spec.maxHp = 131;

    const ReadMember m = roundTrip(spec);
    EXPECT_TRUE(m.checksumOk);
    EXPECT_EQ(m.pid, spec.pid);
    EXPECT_EQ(m.species, 479);
    EXPECT_EQ(m.heldItem, 0x00EA);
    EXPECT_EQ(m.ability, 26);
    EXPECT_EQ(m.form, 2);
    EXPECT_EQ(m.nature, 23);
    EXPECT_EQ(m.moves, (std::array<int, 4> {85, 247, 109, 0}));
    EXPECT_EQ(m.evs, (std::array<int, 6> {4, 0, 252, 0, 252, 0}));
    EXPECT_EQ(m.ivs, (std::array<int, 6> {31, 0, 15, 20, 1, 30}));
    EXPECT_FALSE(m.egg);
    EXPECT_EQ(m.level, 47);
    EXPECT_EQ(m.hp, 88);
    EXPECT_EQ(m.maxHp, 131);
}

TEST(PartyReader, ReadsTheOriginGame) // H2-CP1
{
    MemberSpec spec;
    spec.originGame = 12; // 플라티나
    EXPECT_EQ(roundTrip(spec).originGame, 12);
}

TEST(PartyReader, ReadsTheEggBitApartFromIvs)
{
    MemberSpec spec;
    spec.iv32 = (1u << 30) | 0x3FFFFFFFu; // 알 + 개체값 전부 31
    const ReadMember m = roundTrip(spec);
    EXPECT_TRUE(m.egg);
    EXPECT_EQ(m.ivs, (std::array<int, 6> {31, 31, 31, 31, 31, 31}));
}

TEST(PartyReader, IgnoresTheFifthGenerationBytesInAPk4) // H6-CP4-2
{
    MemberSpec spec;
    spec.pid = 0x2313;   // % 25 = 4
    spec.natureByte = 9; // 4세대에서는 빛나는 잎 자리 — 성격이 아니다
    spec.hiddenAbility = true;
    const ReadMember m = roundTrip(spec);
    EXPECT_EQ(m.nature, 4);
    EXPECT_FALSE(m.hiddenAbility);
}

TEST(PartyReader, ReadsTheNatureAndHiddenAbilityOfAPk5) // H6-CP4-1 · 2
{
    MemberSpec spec;
    spec.species = 571;  // 조로아크
    spec.pid = 0x2313;   // % 25 = 4 — PK5는 이 값을 쓰지 않는다
    spec.natureByte = 9; // PK5의 진짜 성격: 9 = 촐랑(Lax)
    spec.hiddenAbility = true;
    const ReadMember m = roundTripPk5(spec);
    EXPECT_TRUE(m.checksumOk);
    EXPECT_EQ(m.species, 571);
    EXPECT_EQ(m.nature, 9);
    EXPECT_TRUE(m.hiddenAbility);
    EXPECT_EQ(m.level, 50); // 배틀 스탯(84바이트)도 풀린다
}

// ── H6 CP1 기기 리더 등록 · readParty ─────────────────────────

TEST(PartyReader, ListsTheNdsReader) // H6-CP1-1
{
    const auto readers = partyReaders();
    ASSERT_EQ(readers.size(), 1u);
    ASSERT_NE(readers[0], nullptr);
    EXPECT_EQ(readers[0]->platform(), "nds");
}

TEST(PartyReader, ReadsAFourthGenerationSave) // H6-CP1-2 + CP2
{
    const auto party = readParty(buildSave(synth::seriesOf("heartgold-soulsilver"),
                                           SlotSpec {.party = {MemberSpec {}}}));
    ASSERT_TRUE(party.has_value());
    EXPECT_EQ(party->generation, 4);
    EXPECT_EQ(party->versionGroup, "heartgold-soulsilver");
}

TEST(PartyReader, ReadsAFifthGenerationSave) // H6-CP1-2 + CP2 · CP3 · CP4
{
    const auto party = readParty(buildTableSave(synth::seriesOf("black-2-white-2"),
                                                TableSaveSpec {.party = {MemberSpec {}}}));
    ASSERT_TRUE(party.has_value());
    EXPECT_EQ(party->generation, 5);
    EXPECT_EQ(party->versionGroup, "black-2-white-2");
}

TEST(PartyReader, RejectsWhatNoReaderRecognises) // H6-CP1-2
{
    EXPECT_FALSE(readParty(std::vector<std::uint8_t>(kSaveSize, 0x00)).has_value());
    EXPECT_FALSE(readParty(std::vector<std::uint8_t>(kSaveSize, 0xFF)).has_value());
    EXPECT_FALSE(readParty(std::vector<std::uint8_t>(100, 0)).has_value());
}
