#include "core/save/savebytes.h"

namespace com::yamada::studio::save {
std::uint16_t readU16(Bytes data, std::size_t offset)
{
    if (offset + 2 > data.size())
        return 0;
    std::uint8_t lo = data[offset];
    std::uint8_t hi = data[offset + 1];
    return static_cast<std::uint16_t>(lo | (hi << 8));
}

std::uint32_t readU32(Bytes data, std::size_t offset)
{
    if (offset + 4 > data.size())
        return 0;
    std::uint16_t lo = readU16(data, offset);
    std::uint16_t hi = readU16(data, offset + 2);
    return static_cast<std::uint32_t>(lo | (static_cast<std::uint32_t>(hi) << 16));
}

std::uint16_t crc16Ccitt(Bytes data)
{
    std::uint16_t crc = 0xFFFF;
    for (const auto &b : data) {
        crc ^= (static_cast<std::uint16_t>(b) << 8);

        for (int i = 0; i < 8; ++i) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc = crc << 1;
            }
        }
    }
    return crc;
}
} // namespace com::yamada::studio::save
