#pragma once

#include "core/save/savebytes.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

// 4세대 세이브 파일의 큰 구조 (H1 CP2). 가이드: docs/guides/h1-save-reader.md
//
//   0x00000 ┌ 슬롯 0 ─ 일반(general) 블록 [데이터 … | footer] ─ 보관(storage) 블록 …
//   0x40000 └ 슬롯 1 ─ 같은 모양의 사본
//
// 게임은 저장할 때마다 두 슬롯에 번갈아 쓴다(쓰다가 전원이 꺼져도 하나는 남도록). 그래서
// "지금" 데이터는 footer의 저장 카운터가 더 크고 CRC가 맞는 쪽이다. 파티는 일반 블록 안에
// 있으므로 H1은 일반 블록만 읽는다. 수치 출처: PKHeX SAV4DP/Pt/HGSS · SAV4BlockDetection
// (GPL-3.0 — 수치만 참고, 코드는 직접 쓴다).
namespace com::yamada::studio::save {
inline constexpr std::size_t kSaveSize
        = 0x80000; // 512 KiB. DeSmuME .dsv는 뒤에 122바이트가 더 붙는다
inline constexpr std::size_t kSlotSize = 0x40000; // 슬롯 1의 시작

// 게임마다 다른 수치. `if (game == ...)` 대신 이 표의 행 하나로 게임 차이를 표현한다(CLAUDE.md §4)
struct SaveLayout
{
    std::string_view versionGroup; // PokéAPI version_group identifier
    std::size_t generalSize;       // 일반 블록 크기(footer 포함)
    std::size_t footerSize;        // CRC 범위에서 빼는 끝부분 길이
    std::size_t partyCountOffset;  // 일반 블록 시작 기준. 1바이트(0–6)
    std::size_t partyOffset;       // 일반 블록 시작 기준. 236바이트 × 6
};

inline constexpr std::array kGen4Layouts = {
        SaveLayout {"diamond-pearl", 0xC100, 0x14, 0x94, 0x98},
        SaveLayout {"platinum", 0xCF2C, 0x14, 0x9C, 0xA0},
        SaveLayout {"heartgold-soulsilver", 0xF628, 0x10, 0x94, 0x98},
};

// 일반 블록 끝의 footer. 위치는 모두 "블록 끝"(blockStart + generalSize) 기준
struct BlockFooter
{
    // 끝 − 0x14: 저장 카운터 major. HGSS에서는 0이고 실제 저장 횟수는 minor에 있다(실파일 확인) —
    // 그래서 항상 (major, minor) 쌍으로 비교한다
    std::uint32_t major = 0;
    std::uint32_t minor = 0; // 끝 − 0x10: major가 같을 때 비교
    // 끝 − 0x0C: 블록 크기 = generalSize (SS 한국판 실파일로 확인 · 2026-10-08)
    std::uint32_t size = 0;
    std::uint32_t magic = 0; // 끝 − 0x08: 0x20060623(일본 · 해외판) · 0x20070903(한국판)
    std::uint16_t storedCrc = 0;   // 끝 − 0x02
    std::uint16_t computedCrc = 0; // crc16Ccitt([blockStart, 끝 − footerSize))
    // 블록이 파일 안에 있어 실제로 읽었다. 아니면 위 값이 전부 0이라 CRC가 "맞아" 보이므로 따로
    // 둔다
    bool inFile = false;
    bool crcOk() const { return inFile && storedCrc == computedCrc; }
};

BlockFooter readFooter(Bytes save, std::size_t blockStart, const SaveLayout &layout);

// 지금 쓰는 일반 블록의 시작(0 또는 kSlotSize). CRC가 맞는 슬롯 중 (major, minor)가 큰 쪽.
// 둘 다 CRC가 틀리면 nullopt — 이 레이아웃(게임)이 아니거나 세이브가 깨졌다
std::optional<std::size_t> activeGeneralBlock(Bytes save, const SaveLayout &layout);
} // namespace com::yamada::studio::save
