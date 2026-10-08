#include "core/save/pkmcodec.h"

#include <algorithm>

namespace com::yamada::studio::save {
void cryptArray(std::span<std::uint8_t> data, std::uint32_t seed)
{
    for (std::size_t i = 0; i + 1 < data.size(); i += 2) {
        seed = seed * 0x41C64E6D + 0x6073;
        const auto key = static_cast<std::uint16_t>(seed >> 16);
        data[i] ^= static_cast<std::uint8_t>(key & 0xFF);
        data[i + 1] ^= static_cast<std::uint8_t>(key >> 8);
    }
}

int shuffleIndex(std::uint32_t pid)
{
    pid = (pid >> 13) & 0x1F;
    pid %= 24;
    return static_cast<int>(pid);
}

std::array<std::uint8_t, kBlocksSize> unshuffle(Bytes shuffled, int shuffleIndex)
{
    std::array<std::uint8_t, kBlocksSize> blocks {};
    if (shuffled.size() != kBlocksSize || shuffleIndex < 0 || shuffleIndex >= 24)
        return blocks;
    uint8_t pos;
    for (int i = 0; i < 4; ++i) {
        pos = kBlockPosition[shuffleIndex][i];
        std::copy_n(shuffled.begin() + pos * kBlockSize, kBlockSize,
                    blocks.begin() + i * kBlockSize);
    }
    return blocks;
}

std::uint16_t pkmChecksum(const Pkm &decrypted)
{
    // u16에 더하면 넘침(2^16으로 나눈 나머지)이 저절로 처리된다
    std::uint16_t sum = 0;
    for (std::size_t offset = kHeaderSize; offset < kStoredSize; offset += 2) {
        sum += readU16(decrypted, offset);
    }
    return sum;
}

DecodedPkm decodePkm(Bytes encrypted)
{
    DecodedPkm pkm;
    if (encrypted.size() != kPartyPkmSize)
        return pkm;
    std::copy(encrypted.begin(), encrypted.end(), pkm.data.begin());
    pkm.decoded = true;
    pkm.pid = readU32(pkm.data, 0x00);
    pkm.storedChecksum = readU16(pkm.data, 0x06);
    cryptArray(std::span(pkm.data).subspan(kHeaderSize, kStoredSize - kHeaderSize),
               pkm.storedChecksum);
    cryptArray(std::span(pkm.data).subspan(kStoredSize, kPartyPkmSize - kStoredSize), pkm.pid);
    pkm.shuffle = shuffleIndex(pkm.pid);
    std::array<std::uint8_t, kBlocksSize> unshuffled
            = unshuffle(std::span(pkm.data).subspan(kHeaderSize, kBlocksSize), pkm.shuffle);
    std::copy(unshuffled.begin(), unshuffled.end(), pkm.data.begin() + kHeaderSize);
    pkm.computedChecksum = pkmChecksum(pkm.data);
    return pkm;
}
} // namespace com::yamada::studio::save
