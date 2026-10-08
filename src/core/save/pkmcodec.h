#pragma once

#include "core/save/savebytes.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

// 포켓몬 한 마리(PKM)의 암호 풀기 (H1 CP3). 가이드: docs/guides/h1-save-reader.md
//
// 파티의 포켓몬 한 마리 = 236바이트(0xEC):
//   0x00 PID(u32) · 0x04 플래그(u16) · 0x06 체크섬(u16)        ← 평문
//   0x08 … 0x87  블록 A · B · C · D (32바이트씩) — 섞이고 암호화됨 (시드 = 체크섬)
//   0x88 … 0xEB  배틀 스탯(레벨 · HP · 능력치)   — 암호화됨     (시드 = PID)
// 박스의 포켓몬은 앞 136바이트만 있다. 수치 출처: PKHeX PokeCrypto · PK4(수치만 참고).
namespace com::yamada::studio::save {
inline constexpr std::size_t kPartyPkmSize = 236;
inline constexpr std::size_t kStoredSize = 136; // 헤더 8 + 블록 128
inline constexpr std::size_t kHeaderSize = 8;
inline constexpr std::size_t kBlockSize = 32;
inline constexpr std::size_t kBlocksSize = 4 * kBlockSize; // 128

using Pkm = std::array<std::uint8_t, kPartyPkmSize>;

// 블록 순서 표: kBlockPosition[s][i] = 섞인 데이터에서 논리 블록 i(A=0 · B=1 · C=2 · D=3)가 있는
// **자리**. 예) s = 3 → {0, 3, 1, 2}: A는 0번 자리, B는 3번, C는 1번, D는 2번 → 섞인 순서 "ACDB"
inline constexpr std::array<std::array<std::uint8_t, 4>, 24> kBlockPosition = {{
        {0, 1, 2, 3}, {0, 1, 3, 2}, {0, 2, 1, 3}, {0, 3, 1, 2}, {0, 2, 3, 1}, {0, 3, 2, 1},
        {1, 0, 2, 3}, {1, 0, 3, 2}, {2, 0, 1, 3}, {3, 0, 1, 2}, {2, 0, 3, 1}, {3, 0, 2, 1},
        {1, 2, 0, 3}, {1, 3, 0, 2}, {2, 1, 0, 3}, {3, 1, 0, 2}, {2, 3, 0, 1}, {3, 2, 0, 1},
        {1, 2, 3, 0}, {1, 3, 2, 0}, {2, 1, 3, 0}, {3, 1, 2, 0}, {2, 3, 1, 0}, {3, 2, 1, 0},
}};

// LCRNG 스트림 XOR. 2바이트(u16, 리틀 엔디언)마다:
//   seed = seed × 0x41C64E6D + 0x6073   (std::uint32_t라 넘치면 저절로 2³²로 나눈 나머지)
//   그 u16 ^= (seed >> 16)
// XOR은 두 번 하면 원래대로 → 암호화 · 복호화가 같은 함수다. data 길이는 짝수
void cryptArray(std::span<std::uint8_t> data, std::uint32_t seed);

// 섞인 순서 번호 0–23: ((pid >> 13) & 0x1F) % 24
int shuffleIndex(std::uint32_t pid);

// 섞인 128바이트(블록 4개) → A · B · C · D 순서로 되돌린 128바이트
std::array<std::uint8_t, kBlocksSize> unshuffle(Bytes shuffled, int shuffleIndex);

// 풀린 PKM의 0x08–0x87을 u16 64개로 읽어 더한 값(16비트에서 넘침). 0x06의 값과 같아야 한다
std::uint16_t pkmChecksum(const Pkm &decrypted);

struct DecodedPkm
{
    Pkm data {}; // 평문 236바이트(블록은 A · B · C · D 순서)
    std::uint32_t pid = 0;
    int shuffle = 0; // shuffleIndex(pid) — 로그용
    std::uint16_t storedChecksum = 0;
    std::uint16_t computedChecksum = 0;
    bool decoded = false; // 236바이트를 받아 풀었다(크기가 틀리면 false)
    bool ok() const { return decoded && storedChecksum == computedChecksum; }
};

// encrypted: 세이브에서 자른 236바이트. 크기가 다르면 빈 DecodedPkm
DecodedPkm decodePkm(Bytes encrypted);
} // namespace com::yamada::studio::save
