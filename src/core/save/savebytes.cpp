#include "core/save/savebytes.h"

namespace com::yamada::studio::save {
std::uint16_t readU16(Bytes data, std::size_t offset)
{
    if (offset + 2 > data.size())
        return 0;
    // TODO(CP1-1) data[offset]이 낮은 바이트, data[offset + 1]이 높은 바이트다.
    //   높은 바이트를 8비트 왼쪽으로 밀고(<<) 낮은 바이트와 OR(|)로 합친다.
    //   uint8_t끼리의 연산 결과는 int로 승격되므로 static_cast<std::uint16_t>(...)로 감싼다
    std::uint8_t lo = data[offset];
    std::uint8_t hi = data[offset + 1];
    return static_cast<std::uint16_t>(lo | (hi << 8));
}

std::uint32_t readU32(Bytes data, std::size_t offset)
{
    if (offset + 4 > data.size())
        return 0;
    // TODO(CP1-2) readU16을 두 번 쓴다: 낮은 반은 offset, 높은 반은 offset + 2.
    //   높은 반을 std::uint32_t로 바꾼 뒤 16비트 밀어서 OR
    std::uint16_t lo = readU16(data, offset);
    std::uint16_t hi = readU16(data, offset + 2);
    return static_cast<std::uint32_t>(lo | (static_cast<std::uint32_t>(hi) << 16));
}

std::uint16_t crc16Ccitt(Bytes data)
{
    // TODO(CP1-3) 16비트 레지스터 crc를 0xFFFF로 시작한다(std::uint16_t)
    std::uint16_t crc = 0xFFFF;
    // TODO(CP1-4) 바이트 b마다: crc ^= (b << 8) — 바이트를 레지스터의 위쪽 8비트에 XOR
    for (const auto &b : data) {
        crc ^= (static_cast<std::uint16_t>(b) << 8);
        
        // TODO(CP1-5) 이어서 8번 반복: 맨 위 비트(crc & 0x8000)가 켜져 있으면
        //   crc = (crc << 1) ^ 0x1021, 아니면 crc = crc << 1. 매번 16비트로 자른다(uint16_t에 대입)
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
