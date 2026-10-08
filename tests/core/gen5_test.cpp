// H6 — 5세대(BW · B2W2) 형식: ① 판별(체크섬 모음 블록) · ② 추출(파티 블록 · PK5). ctest -R Gen5
#include "core/save/gen5.h"
#include "syntheticsave.h"

#include <gtest/gtest.h>

using namespace com::yamada::studio::save;
using synth::buildGen5Save;
using synth::Gen5SaveSpec;
using synth::MemberSpec;

namespace {
const Gen5Layout &kBW = synth::gen5LayoutOf("black-white");
const Gen5Layout &kB2W2 = synth::gen5LayoutOf("black-2-white-2");
} // namespace

// ── ① 판별 ──────────────────────────────────────────────────

TEST(Gen5, InfoBlockSitsAQuarterKilobyteBeforeTheEndOfTheMainSave) // H6-CP3
{
    EXPECT_EQ(infoBlockStart(kBW), 0x23F00u);
    EXPECT_EQ(infoBlockStart(kB2W2), 0x25F00u);
}

TEST(Gen5, ValidatesTheInfoBlockOfItsOwnGameOnly) // H6-CP3-1
{
    const auto bw = buildGen5Save(kBW, Gen5SaveSpec {.party = {MemberSpec {}}});
    EXPECT_TRUE(infoBlockValid(bw, kBW));
    EXPECT_FALSE(infoBlockValid(bw, kB2W2)); // 본 세이브 크기가 달라 블록 자리가 다르다

    const auto b2w2 = buildGen5Save(kB2W2, Gen5SaveSpec {.party = {MemberSpec {}}});
    EXPECT_TRUE(infoBlockValid(b2w2, kB2W2));
    EXPECT_FALSE(infoBlockValid(b2w2, kBW));
}

TEST(Gen5, RejectsACorruptedInfoBlock) // H6-CP3-1
{
    const auto save
            = buildGen5Save(kBW, Gen5SaveSpec {.party = {MemberSpec {}}, .corruptInfo = true});
    EXPECT_FALSE(infoBlockValid(save, kBW));
}

TEST(Gen5, NeverValidatesOutsideTheFile) // H6-CP3-1 — H1의 inFile 교훈
{
    const std::vector<std::uint8_t> tiny(16, 0);
    EXPECT_FALSE(infoBlockValid(tiny, kBW));
    EXPECT_FALSE(partyBlockValid(tiny, kBW));
}

TEST(Gen5, ChecksThePartyBlockCrcInBothPlaces) // H6-CP3-2
{
    EXPECT_TRUE(partyBlockValid(buildGen5Save(kBW, Gen5SaveSpec {.party = {MemberSpec {}}}), kBW));
    auto save = buildGen5Save(kBW, Gen5SaveSpec {.party = {MemberSpec {}}, .corruptParty = true});
    EXPECT_FALSE(partyBlockValid(save, kBW));

    // 블록 뒤의 CRC는 맞는데 모음 블록의 사본만 틀린 경우도 거절한다
    save = buildGen5Save(kBW, Gen5SaveSpec {.party = {MemberSpec {}}});
    save[infoBlockStart(kBW) + kGen5PartyCrcMirror] ^= 0xFF;
    EXPECT_FALSE(partyBlockValid(save, kBW));
}

// ── ② 추출 ──────────────────────────────────────────────────

TEST(Gen5, Decodes220BytePartyPokemon) // H6-CP4-1
{
    const Pkm encrypted = synth::encodePkm(synth::plainPkm({}));
    const DecodedPkm pkm = decodePkm(Bytes(encrypted).first(kGen5PartyPkmSize));
    EXPECT_TRUE(pkm.ok());
    EXPECT_EQ(readU16(pkm.data, 0x08), 392);
    EXPECT_EQ(pkm.data[0x8C], 50); // 배틀 스탯(레벨)도 풀린다
}

TEST(Gen5, ReadsABlackWhiteParty) // H6-CP4
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

    const auto save = buildGen5Save(kBW, Gen5SaveSpec {.party = {zoroark, second}});
    const auto party = readGen5Party(save);
    ASSERT_TRUE(party.has_value());
    EXPECT_EQ(party->generation, 5);
    EXPECT_EQ(party->versionGroup, "black-white");
    EXPECT_EQ(party->partyOffset, kGen5PartyBlock + kGen5PartyOffset);
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

TEST(Gen5, ReadsABlack2White2Party) // H6-CP4
{
    const auto party = readGen5Party(buildGen5Save(kB2W2, Gen5SaveSpec {.party = {MemberSpec {}}}));
    ASSERT_TRUE(party.has_value());
    EXPECT_EQ(party->versionGroup, "black-2-white-2");
}

TEST(Gen5, RejectsAPartyBlockThatDoesNotCheckOut) // H6-CP4-2
{
    EXPECT_FALSE(readGen5Party(buildGen5Save(kBW, Gen5SaveSpec {.party = {MemberSpec {}},
                                                                .corruptParty = true}))
                         .has_value());
}

TEST(Gen5, RejectsAnEmptyParty) // H6-CP4-3
{
    EXPECT_FALSE(readGen5Party(buildGen5Save(kBW, Gen5SaveSpec {})).has_value());
}

TEST(Gen5, IsNotFooledByAFourthGenerationSave) // H6-CP3 — 같은 512 KiB
{
    EXPECT_FALSE(readGen5Party(synth::buildSave(synth::layoutOf("platinum"),
                                                synth::SlotSpec {.party = {MemberSpec {}}}))
                         .has_value());
}
