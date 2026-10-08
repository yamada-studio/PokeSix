#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

// 세이브 바이트를 읽는 기본 도구 (H1 CP1). 가이드: docs/guides/h1-save-reader.md
//
// DS는 리틀 엔디언이다 — 여러 바이트로 된 정수는 **낮은 바이트가 앞**에 온다.
// 파일에 34 12 가 있으면 그 u16 값은 0x1234 다.
namespace com::yamada::studio::save {
// 세이브 · PKM을 읽기 전용으로 보는 창. std::vector<std::uint8_t>는 그대로 넘어온다
using Bytes = std::span<const std::uint8_t>;

// offset 위치의 2 · 4바이트 리틀 엔디언 정수. 범위를 넘으면 0
std::uint16_t readU16(Bytes data, std::size_t offset);
std::uint32_t readU32(Bytes data, std::size_t offset);

// CRC-16/CCITT-FALSE — 다항식 0x1021, 초깃값 0xFFFF, 비트 반사 없음, 마지막 XOR 없음.
// 4세대 세이브 블록 footer의 체크섬이다. 검증 값: "123456789" → 0x29B1, 빈 입력 → 0xFFFF
std::uint16_t crc16Ccitt(Bytes data);
} // namespace com::yamada::studio::save
