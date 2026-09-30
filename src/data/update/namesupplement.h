#pragma once

#include <string_view>

// PokéAPI에 한국어 이름이 없는 아이템의 보충 표. 변환(CsvImporter::importItems)이 한국어 칸이 비어
// 있을 때만 이 이름을 넣는다 — PokéAPI에 이름이 생기면 그쪽이 이긴다.
//
// 넣는 기준: 한국어 정식판이 있던 게임의 이름만. 4세대(DPPt · HGSS)는 한국어판이 있었다.
// 3세대(RSE · FRLG)는 한국어판이 없어서, 3세대에만 있던 아이템(3세대 메일 · 데봉의 짐 등)은 넣지
// 않는다 — 지어낸 이름을 공식처럼 보이게 하지 않는다. 그런 아이템은 화면에서 다른 언어로 대신
// 보인다(LocalizedText의 대체 순서).
//
// 틀린 이름을 찾으면 여기를 고치고, 앱에서 데이터를 다시 받으면(첫 실행 패널) 반영된다.
namespace com::yamada::studio::namesupplement {
struct ItemName
{
    std::string_view identifier; // PokéAPI items.identifier
    std::string_view nameKo;     // UTF-8
};

// 4세대 메일(DPPt · HGSS 한국어판) · 비전머신08(기술머신 · 비전머신 이름 규칙)
inline constexpr ItemName kItemNamesKo[] = {
        {"grass-mail", "그래스메일"},    {"flame-mail", "플레임메일"},
        {"bubble-mail", "블루메일"},     {"bloom-mail", "블룸메일"},
        {"tunnel-mail", "터널메일"},     {"steel-mail", "스틸메일"},
        {"heart-mail", "러브러브메일"},  {"snow-mail", "블리자드메일"},
        {"space-mail", "스페이스메일"},  {"air-mail", "에어메일"},
        {"mosaic-mail", "모자이크메일"}, {"brick-mail", "브릭메일"},
        {"hm08", "비전머신08"},
};
} // namespace com::yamada::studio::namesupplement
