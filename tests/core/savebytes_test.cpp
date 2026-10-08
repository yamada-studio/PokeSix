// H1 CP1 — 리틀 엔디언 읽기와 CRC-16/CCITT. ctest -R SaveBytes
#include "core/save/savebytes.h"

#include <gtest/gtest.h>

#include <array>

using namespace com::yamada::studio::save;

TEST(SaveBytes, ReadsLittleEndian)
{
    const std::array<std::uint8_t, 4> data {0x34, 0x12, 0x78, 0x56};
    EXPECT_EQ(readU16(data, 0), 0x1234);
    EXPECT_EQ(readU16(data, 2), 0x5678);
    EXPECT_EQ(readU32(data, 0), 0x56781234u);
}

TEST(SaveBytes, ReadsTheHighBit)
{
    // 부호 확장 함정: uint8_t → int 승격 뒤 << 해도 부호가 붙지 않아야 한다
    const std::array<std::uint8_t, 4> data {0xFF, 0xFE, 0xFD, 0xFC};
    EXPECT_EQ(readU16(data, 0), 0xFEFF);
    EXPECT_EQ(readU32(data, 0), 0xFCFDFEFFu);
}

TEST(SaveBytes, OutOfRangeReadsZero)
{
    const std::array<std::uint8_t, 4> data {1, 2, 3, 4};
    EXPECT_EQ(readU16(data, 3), 0);
    EXPECT_EQ(readU32(data, 1), 0u);
}

// CRC 카탈로그의 공식 검증 값(check value): CRC-16/CCITT-FALSE("123456789") = 0x29B1
TEST(SaveBytes, Crc16MatchesTheCatalogCheckValue)
{
    const std::array<std::uint8_t, 9> digits {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    EXPECT_EQ(crc16Ccitt(digits), 0x29B1);
}

TEST(SaveBytes, Crc16OfNothingIsTheInitialValue)
{
    EXPECT_EQ(crc16Ccitt({}), 0xFFFF);
}
