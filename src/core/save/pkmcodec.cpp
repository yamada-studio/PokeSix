#include "core/save/pkmcodec.h"

#include <algorithm>

namespace com::yamada::studio::save {
void cryptArray(std::span<std::uint8_t> data, std::uint32_t seed)
{
    // TODO(CP3-1) i = 0, 2, 4 … (data.size()까지)마다:
    //   seed = seed * 0x41C64E6D + 0x6073;
    //   const auto key = static_cast<std::uint16_t>(seed >> 16);
    //   data[i]     ^= 키의 낮은 바이트 (key & 0xFF)
    //   data[i + 1] ^= 키의 높은 바이트 (key >> 8)          ← 리틀 엔디언
    (void)data;
    (void)seed;
}

int shuffleIndex(std::uint32_t pid)
{
    // TODO(CP3-2) 헤더 주석의 식 그대로. 결과는 0–23
    (void)pid;
    return 0;
}

std::array<std::uint8_t, kBlocksSize> unshuffle(Bytes shuffled, int shuffleIndex)
{
    std::array<std::uint8_t, kBlocksSize> blocks {};
    if (shuffled.size() != kBlocksSize || shuffleIndex < 0 || shuffleIndex >= 24)
        return blocks;
    // TODO(CP3-3) 논리 블록 i = 0..3마다:
    //   pos = kBlockPosition[shuffleIndex][i]  — 섞인 데이터에서 i가 있는 자리
    //   shuffled의 [pos * kBlockSize, +kBlockSize)를 blocks의 [i * kBlockSize, …)로 복사.
    //   std::copy_n(shuffled.begin() + …, kBlockSize, blocks.begin() + …)
    return blocks;
}

std::uint16_t pkmChecksum(const Pkm &decrypted)
{
    // TODO(CP3-4) offset = kHeaderSize부터 kStoredSize 전까지 2씩: sum += readU16(decrypted,
    // offset).
    //   sum을 std::uint16_t로 두면 넘침이 저절로 처리된다
    (void)decrypted;
    return 0;
}

DecodedPkm decodePkm(Bytes encrypted)
{
    DecodedPkm pkm;
    if (encrypted.size() != kPartyPkmSize)
        return pkm;
    std::copy(encrypted.begin(), encrypted.end(), pkm.data.begin());
    pkm.decoded = true;
    // TODO(CP3-5) pid(0x00, u32)와 storedChecksum(0x06, u16)을 읽는다 — 헤더는 평문이다
    // TODO(CP3-6) 블록 영역 [kHeaderSize, kStoredSize)를 시드 storedChecksum으로 cryptArray.
    //   std::span(pkm.data).subspan(시작, 길이)
    // TODO(CP3-7) 배틀 스탯 영역 [kStoredSize, kPartyPkmSize)를 시드 pid로 cryptArray
    // TODO(CP3-8) shuffle = shuffleIndex(pid). 블록 영역을 unshuffle해서 결과를 같은 자리에 되쓴다
    //   (unshuffle은 사본을 돌려준다 — std::copy로 pkm.data.begin() + kHeaderSize에)
    // TODO(CP3-9) computedChecksum = pkmChecksum(pkm.data)
    return pkm;
}
} // namespace com::yamada::studio::save
