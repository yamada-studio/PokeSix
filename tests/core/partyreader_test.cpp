// H1 CP4 · CP5 — 필드 읽기와 세이브 → 파티 조립. ctest -R PartyReader
#include "core/save/saveformat.h"
#include "syntheticsave.h"

#include <gtest/gtest.h>

using namespace com::yamada::studio::save;
using synth::buildSave;
using synth::MemberSpec;
using synth::SlotSpec;

namespace {
ReadMember roundTrip(const MemberSpec &spec)
{
    return parseMember(decodePkm(synth::encodePkm(synth::plainPkm(spec))));
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

// ── CP5 readParty ───────────────────────────────────────────

TEST(PartyReader, ReadsAPlatinumParty)
{
    MemberSpec a, b, c;
    a.species = 392;
    b.species = 398;
    b.pid = 0x0BADF00D;
    c.species = 448;
    c.pid = 0x7777AAAA;
    const auto save = buildSave(synth::layoutOf("platinum"), SlotSpec {.party = {a, b, c}});

    const auto party = readGen4Party(save);
    ASSERT_TRUE(party.has_value());
    EXPECT_EQ(party->generation, 4);
    EXPECT_EQ(party->versionGroup, "platinum");
    EXPECT_EQ(party->partyOffset, synth::layoutOf("platinum").partyOffset); // 슬롯 0의 파티
    ASSERT_EQ(party->members.size(), 3u);
    EXPECT_EQ(party->members[0].species, 392);
    EXPECT_EQ(party->members[1].species, 398);
    EXPECT_EQ(party->members[2].species, 448);
    for (const ReadMember &m : party->members)
        EXPECT_TRUE(m.checksumOk);
}

TEST(PartyReader, DetectsEachGameFromTheBlockSize)
{
    for (const SaveLayout &layout : kGen4Layouts) {
        const auto party = readGen4Party(buildSave(layout, SlotSpec {.party = {MemberSpec {}}}));
        ASSERT_TRUE(party.has_value()) << layout.versionGroup;
        EXPECT_EQ(party->versionGroup, layout.versionGroup);
    }
}

TEST(PartyReader, ReadsTheNewestSlot)
{
    MemberSpec old, new1, new2;
    old.species = 387;
    new1.species = 388;
    new2.species = 16;
    const auto save = buildSave(synth::layoutOf("heartgold-soulsilver"),
                                SlotSpec {.major = 10, .party = {old}},
                                SlotSpec {.major = 11, .party = {new1, new2}});
    const auto party = readGen4Party(save);
    ASSERT_TRUE(party.has_value());
    // 슬롯 1(major 11)이 최신 — 파티 위치가 그 슬롯 안이다
    EXPECT_EQ(party->partyOffset, kSlotSize + synth::layoutOf("heartgold-soulsilver").partyOffset);
    ASSERT_EQ(party->members.size(), 2u);
    EXPECT_EQ(party->members[0].species, 388);
}

TEST(PartyReader, AcceptsADeSmuMEFooter)
{
    auto save = buildSave(synth::layoutOf("platinum"), SlotSpec {.party = {MemberSpec {}}});
    save.resize(kSaveSize + 122, 0); // .dsv 꼬리
    EXPECT_TRUE(readGen4Party(save).has_value());
}

TEST(PartyReader, RejectsWhatIsNotASave)
{
    EXPECT_FALSE(readGen4Party(std::vector<std::uint8_t>(1000, 0)).has_value()); // 너무 작다
    EXPECT_FALSE(
            readGen4Party(std::vector<std::uint8_t>(kSaveSize, 0xFF)).has_value()); // 빈 플래시
    EXPECT_FALSE(readGen4Party(std::vector<std::uint8_t>(kSaveSize, 0x00)).has_value());
}

TEST(PartyReader, RejectsAnEmptyParty)
{
    EXPECT_FALSE(readGen4Party(buildSave(synth::layoutOf("platinum"), SlotSpec {})).has_value());
}
