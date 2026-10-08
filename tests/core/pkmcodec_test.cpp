// H1 CP3 — PKM 암호 풀기(LCRNG · 블록 섞기 · 체크섬). ctest -R PkmCodec
#include "syntheticsave.h"

#include <gtest/gtest.h>

#include <string>

using namespace com::yamada::studio::save;

// LCRNG 검증 값은 파서와 독립으로(파이썬으로) 계산했다:
//   seed 0x1234 → 상위 16비트 0x4DCB · 0xE161 · 0x4340 · 0xFFF1 → 리틀 엔디언 바이트
TEST(PkmCodec, CryptXorsTheKnownLcrngStream)
{
    std::array<std::uint8_t, 8> data {};
    cryptArray(data, 0x1234);
    const std::array<std::uint8_t, 8> expected {0xCB, 0x4D, 0x61, 0xE1, 0x40, 0x43, 0xF1, 0xFF};
    EXPECT_EQ(data, expected);
}

TEST(PkmCodec, CryptAdvancesBeforeTheFirstWord)
{
    // seed 0: 첫 키는 seed를 한 번 굴린 0x00006073의 상위 16비트 = 0x0000, 둘째는 0xE97E
    std::array<std::uint8_t, 4> data {};
    cryptArray(data, 0);
    const std::array<std::uint8_t, 4> expected {0x00, 0x00, 0x7E, 0xE9};
    EXPECT_EQ(data, expected);
}

TEST(PkmCodec, CryptTwiceRestoresTheInput)
{
    std::array<std::uint8_t, 16> data {};
    for (std::size_t i = 0; i < data.size(); ++i)
        data[i] = std::uint8_t(i * 17);
    const auto original = data;
    cryptArray(data, 0xCAFEBABE);
    EXPECT_NE(data, original);
    cryptArray(data, 0xCAFEBABE);
    EXPECT_EQ(data, original);
}

TEST(PkmCodec, ShuffleIndexUsesPidBits13To17)
{
    EXPECT_EQ(shuffleIndex(0), 0);
    EXPECT_EQ(shuffleIndex(3u << 13), 3);
    EXPECT_EQ(shuffleIndex(27u << 13), 3);   // 27 % 24
    EXPECT_EQ(shuffleIndex(0x1FFFu), 0);     // 아래 13비트는 안 쓴다
    EXPECT_EQ(shuffleIndex(0xFFFFFFFFu), 7); // 31 % 24
}

// 섞인 순서 24가지(Bulbapedia "Pokémon data structure (Generation IV)"의 표 — 파서의
// kBlockPosition과 독립된 출처)
TEST(PkmCodec, UnshuffleRestoresEveryBulbapediaOrder)
{
    const char *orders[24] = {"ABCD", "ABDC", "ACBD", "ACDB", "ADBC", "ADCB", "BACD", "BADC",
                              "BCAD", "BCDA", "BDAC", "BDCA", "CABD", "CADB", "CBAD", "CBDA",
                              "CDAB", "CDBA", "DABC", "DACB", "DBAC", "DBCA", "DCAB", "DCBA"};
    std::array<std::uint8_t, kBlocksSize> expected {};
    for (std::size_t i = 0; i < kBlocksSize; ++i)
        expected[i] = std::uint8_t('A' + i / kBlockSize);

    for (int s = 0; s < 24; ++s) {
        std::array<std::uint8_t, kBlocksSize> shuffled {};
        for (std::size_t i = 0; i < kBlocksSize; ++i)
            shuffled[i] = std::uint8_t(orders[s][i / kBlockSize]);
        EXPECT_EQ(unshuffle(shuffled, s), expected) << "shuffle " << s << " = " << orders[s];
    }
}

TEST(PkmCodec, ChecksumSumsTheBlockWords)
{
    Pkm pkm {};
    std::fill(pkm.begin() + kHeaderSize, pkm.begin() + kStoredSize, std::uint8_t {0x01});
    EXPECT_EQ(pkmChecksum(pkm), 0x4040); // 0x0101 × 64

    std::fill(pkm.begin() + kHeaderSize, pkm.begin() + kStoredSize, std::uint8_t {0xFF});
    EXPECT_EQ(pkmChecksum(pkm), 0xFFC0); // 0xFFFF × 64 = 0x3FFFC0 → 아래 16비트
}

TEST(PkmCodec, ChecksumIgnoresHeaderAndBattleStats)
{
    Pkm pkm {};
    pkm[0x00] = 0xFF;
    pkm[0x06] = 0xFF;
    pkm[0x88] = 0xFF;
    pkm[0xEB] = 0xFF;
    EXPECT_EQ(pkmChecksum(pkm), 0);
}

TEST(PkmCodec, DecodesAnEncodedPkmForEveryShuffle)
{
    for (std::uint32_t s = 0; s < 24; ++s) {
        synth::MemberSpec spec;
        spec.pid = (s << 13) | (0x01234ABCu & ~(0x1Fu << 13));
        const Pkm plain = synth::plainPkm(spec);
        const Pkm encrypted = synth::encodePkm(plain);
        ASSERT_NE(encrypted, plain);

        const DecodedPkm decoded = decodePkm(encrypted);
        EXPECT_TRUE(decoded.ok()) << "shuffle " << s;
        EXPECT_EQ(decoded.pid, spec.pid);
        EXPECT_EQ(decoded.shuffle, int(s));
        EXPECT_EQ(decoded.data, plain) << "shuffle " << s;
    }
}

TEST(PkmCodec, TamperedPkmFailsTheChecksum)
{
    Pkm encrypted = synth::encodePkm(synth::plainPkm({}));
    encrypted[0x20] ^= 0x01;
    EXPECT_FALSE(decodePkm(encrypted).ok());
}

TEST(PkmCodec, WrongSizeIsNeverOk)
{
    const std::array<std::uint8_t, kStoredSize> boxSized {};
    EXPECT_FALSE(decodePkm(boxSized).ok());
}

TEST(PkmCodec, DecodesA220BytePk5) // H6-CP4-1
{
    const Pkm plain = synth::plainPkm({});
    const Pkm encrypted = synth::encodePkm(plain);
    const DecodedPkm pkm = decodePkm(Bytes(encrypted).first(kPk5.partySize));
    EXPECT_TRUE(pkm.ok());
    EXPECT_EQ(readU16(pkm.data, 0x08), 392);
    EXPECT_EQ(pkm.data[0x8C],
              50); // 배틀 스탯(레벨)도 풀린다 — 84바이트 스트림 = 100바이트 스트림의 앞부분
}
