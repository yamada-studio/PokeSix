#pragma once

#include "core/save/savebytes.h"

#include <cstddef>
#include <cstdint>
#include <optional>

// NDS 세이브의 블록 검증 (H1 CP2 · H6). 가이드: docs/guides/h1-save-reader.md · h6-multigen-save.md
//
// "이 파일이 그 구조로 쓰였는가"를 체크섬으로 확인하는 방식이 두 가지 있다. 시리즈(DP · BW …)는
// 이 중 하나와 숫자들로 표현된다 — 표는 partyndsreader.h의 kNdsSeries.
//
//   FooterSlots    4세대식(DP · Pt · HGSS)
//     0x00000 ┌ 슬롯 0 ─ 일반(general) 블록 [데이터 … | footer] ─ 보관 블록 …
//     0x40000 └ 슬롯 1 ─ 같은 모양의 사본
//     게임은 두 슬롯에 번갈아 쓴다(쓰다가 전원이 꺼져도 하나는 남도록). "지금" 데이터는 footer의
//     CRC가 맞고 저장 카운터가 더 큰 쪽이다.
//
//   ChecksumTable  5세대식(BW · B2W2)
//     슬롯을 번갈아 쓰지 않는다. 본 세이브 끝 − 0x100에 "체크섬 모음 블록"(모든 블록의 CRC를 다시
//     모은 표)이 있고, 블록마다 CRC가 블록 뒤와 모음 블록 안의 사본, 두 곳에 있다.
//
// CRC는 둘 다 crc16Ccitt. 수치 출처: PKHeX SAV4 · SAV5 · SaveUtil (GPL-3.0 — 수치만 참고).
namespace com::yamada::studio::save {
inline constexpr std::size_t kSaveSize
        = 0x80000; // 512 KiB(4 · 5세대 공통). DeSmuME .dsv는 뒤에 122바이트가 더 붙는다
inline constexpr std::size_t kSlotSize = 0x40000; // 4세대 슬롯 1의 시작

// ── FooterSlots (4세대식) ───────────────────────────────────

struct FooterSlots
{
    std::size_t generalSize = 0; // 일반 블록 크기(footer 포함)
    std::size_t footerSize = 0;  // CRC 범위에서 빼는 끝부분 길이
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

BlockFooter readFooter(Bytes save, std::size_t blockStart, const FooterSlots &check);

// 지금 쓰는 일반 블록의 시작(0 또는 kSlotSize). CRC가 맞는 슬롯 중 (major, minor)가 큰 쪽.
// 둘 다 CRC가 틀리면 nullopt — 이 시리즈가 아니거나 세이브가 깨졌다
std::optional<std::size_t> activeGeneralBlock(Bytes save, const FooterSlots &check);

// ── ChecksumTable (5세대식) ─────────────────────────────────
//
//   0x18E00     파티 블록(0x534 B)          ← partyBlock · partyBlockSize
//   0x19336     파티 블록의 CRC (u16)        ← partyCrcAt
//   …
//   끝 − 0x100  체크섬 모음 블록(tableLength) ← checksumTableStart
//               +0x34 = 파티 블록 CRC의 사본  ← partyCrcMirror
//   모음 블록 + tableLength + 0x0E = 모음 블록 자신의 CRC (u16)
//   ("끝" = mainSize, 본 세이브 크기)

struct ChecksumTable
{
    std::size_t mainSize = 0;       // 본 세이브 크기(BW 0x24000 · B2W2 0x26000)
    std::size_t tableLength = 0;    // 체크섬 모음 블록 길이(BW 0x8C · B2W2 0x94)
    std::size_t partyBlock = 0;     // 파티 블록 시작(파일 기준)
    std::size_t partyBlockSize = 0; // 파티 블록 길이 — CRC 범위
    std::size_t partyCrcAt = 0;     // 파티 블록 CRC가 저장된 곳(파일 기준)
    std::size_t partyCrcMirror = 0; // 모음 블록 안의 사본 위치(모음 블록 기준)
};

// 체크섬 모음 블록의 시작 = mainSize − 0x100
std::size_t checksumTableStart(const ChecksumTable &check);
// 체크섬 모음 블록의 CRC가 맞는가 — 본 세이브 크기가 맞는가(= 이 시리즈인가)
bool checksumTableValid(Bytes save, const ChecksumTable &check);
// 파티 블록의 CRC가 블록 뒤(partyCrcAt)와 모음 블록의 사본, 두 곳 모두와 맞는가
bool partyBlockValid(Bytes save, const ChecksumTable &check);
} // namespace com::yamada::studio::save
