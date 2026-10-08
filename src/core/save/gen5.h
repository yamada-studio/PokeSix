#pragma once

#include "core/save/partyreader.h"

#include <array>
#include <cstddef>
#include <optional>
#include <string_view>

// 5세대(BW · B2W2) 세이브 형식 (H6). 가이드: docs/guides/h6-multigen-save.md, 수치: docs/data/gen5/
// 수치 출처: PKHeX SaveUtil · SaveBlockAccessor5BW/B2W2 · SAV5 · PK5 (GPL-3.0 — 수치만 참고).
//
//   파일 512 KiB. 앞쪽 "본 세이브"(BW 0x24000 · B2W2 0x26000 바이트)만 읽는다.
//
//   0x18E00     파티 블록(0x534 B): +4 파티 수 · +8부터 PK5 220 B × 6
//   0x19336     파티 블록의 CRC (u16)
//   …
//   끝 − 0x100  체크섬 모음 블록(BW 0x8C · B2W2 0x94 B) — 모든 블록의 CRC를 다시 모은 표
//               +0x34 = 파티 블록 CRC의 사본(mirror)
//   그 뒤       모음 블록 + 길이 + 0x0E = 모음 블록 자신의 CRC (u16)
//   ("끝" = 본 세이브 크기)
//
// 4세대와 달리 슬롯이 번갈아 바뀌지 않는다(최신 판정이 없다). 게임 판별은 4세대와 같은 원리 —
// 표의 행(BW · B2W2)마다 체크섬 모음 블록의 CRC를 검증해 맞는 행이 그 게임이다(본 세이브 크기가
// 달라 블록 위치가 다르다). CRC는 4세대와 같은 crc16Ccitt.
namespace com::yamada::studio::save {
struct Gen5Layout
{
    std::string_view versionGroup; // PokéAPI version_group
    std::size_t mainSize;          // 본 세이브 크기
    std::size_t infoLength;        // 체크섬 모음 블록 길이
};

inline constexpr std::array kGen5Layouts = {
        Gen5Layout {"black-white", 0x24000, 0x8C},
        Gen5Layout {"black-2-white-2", 0x26000, 0x94},
};

inline constexpr std::size_t kGen5PartyBlock = 0x18E00;
inline constexpr std::size_t kGen5PartyBlockSize = 0x534;
inline constexpr std::size_t kGen5PartyCrc = 0x19336; // 파티 블록 CRC가 저장된 곳
inline constexpr std::size_t kGen5PartyCrcMirror = 0x34; // 체크섬 모음 블록 안의 사본 위치
inline constexpr std::size_t kGen5PartyCountOffset = 0x04; // 파티 블록 기준
inline constexpr std::size_t kGen5PartyOffset = 0x08;      // 파티 블록 기준

// 체크섬 모음 블록의 시작 = mainSize − 0x100
std::size_t infoBlockStart(const Gen5Layout &layout);
// 체크섬 모음 블록의 CRC가 맞는가 — 이 행이 그 게임인가
bool infoBlockValid(Bytes save, const Gen5Layout &layout);
// 파티 블록의 CRC가 블록 뒤(kGen5PartyCrc)와 체크섬 모음 블록의 사본, 두 곳 모두와 맞는가
bool partyBlockValid(Bytes save, const Gen5Layout &layout);

// 5세대 형식의 읽기: 판별(kGen5Layouts) → 파티 블록 검증 → PK5 × 파티 수. 아니면 nullopt
std::optional<ReadParty> readGen5Party(Bytes save);
} // namespace com::yamada::studio::save
