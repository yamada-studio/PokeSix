#pragma once

#include "core/save/partyreader.h"

#include <optional>
#include <span>
#include <string_view>

// 세이브 형식 목록과 "세대를 모르는 세이브" 읽기 (H6). 가이드: docs/guides/h6-multigen-save.md
//
//   readParty(save)
//     for (형식 : saveFormats())                  4세대 → 5세대 … (목록 순서)
//         if (auto party = 형식.read(save))     형식이 자기 체크섬을 검증해 맞을 때만 값을 준다
//             return party;
//     return nullopt                            아무 형식도 아니다
//
// 파일 크기로 고르지 않는다 — 4세대 · 5세대는 둘 다 512 KiB다. 형식마다 "이 구조로 읽으면 체크섬이
// 맞는가"를 확인하므로 맞는 형식은 하나뿐이다(4세대 안에서 DP · Pt · HGSS를 가른 것과 같은 원리를
// 세대 단위로 한 단계 올린 것). 세대 차이는 if 분기가 아니라 이 목록의 행이다(CLAUDE.md §4).
namespace com::yamada::studio::save {
struct SaveFormat
{
    int generation = 0;
    std::string_view name; // 로그용 — "4세대 NDS (DP · Pt · HGSS)"
    std::optional<ReadParty> (*read)(Bytes save) = nullptr; // 그 형식이 아니면 nullopt
};

// 시도할 형식들(세대 순서)
std::span<const SaveFormat> saveFormats();

// 세대를 모르는 세이브에서 파티를 읽는다 — 앱(saveimport)이 쓰는 입구
std::optional<ReadParty> readParty(Bytes save);
} // namespace com::yamada::studio::save
