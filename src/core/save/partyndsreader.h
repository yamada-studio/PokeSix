#pragma once

#include "core/save/partyreader.h"
#include "core/save/saveblock.h"

#include <array>
#include <cstddef>
#include <optional>
#include <string_view>
#include <variant>

// NDS 세이브 리더 (H1 · H6). 가이드: docs/guides/h6-multigen-save.md, 수치: docs/data/gen4/ · gen5/
//
// NDS 시리즈 다섯 개는 **표의 행**이고, 행끼리 다른 것은 숫자와 "검증 방식" 하나다:
//
//   시리즈   세대  검증 방식(saveblock.h)   파티 위치            포켓몬 형식
//   DP        4    FooterSlots              일반 블록 + 0x98     PK4
//   Pt        4    FooterSlots              일반 블록 + 0xA0     PK4
//   HGSS      4    FooterSlots              일반 블록 + 0x98     PK4
//   BW        5    ChecksumTable            0x18E08              PK5
//   B2W2      5    ChecksumTable            0x18E08              PK5
//
// 검증 방식은 std::variant — "FooterSlots 또는 ChecksumTable 중 하나"를 담는 상자다. std::visit이
// 들어 있는 쪽에 맞는 findBase 오버로드를 골라 부른다. 시리즈마다 함수를 만들지 않고, 구조가 다른
// 검증 방식마다 함수 하나씩이다. 세대 · 시리즈 차이를 if로 가르지 않는다(CLAUDE.md §4).
namespace com::yamada::studio::save {
struct NdsSeries
{
    int generation = 0;
    std::string_view versionGroup; // PokéAPI version_group
    std::variant<FooterSlots, ChecksumTable> check;
    // 아래 둘은 "기준(base)"에서부터 센 위치다. 기준 = 검증이 돌려준 값:
    //   FooterSlots → 고른 슬롯의 일반 블록 시작(0 또는 kSlotSize) · ChecksumTable → 0(파일 시작)
    std::size_t partyCountOffset = 0; // u8, 1–6
    std::size_t partyOffset = 0;      // 첫 포켓몬
    const PkmFormat *pkm = nullptr;
};

inline constexpr std::array kNdsSeries = {
        NdsSeries {4, "diamond-pearl", FooterSlots {0xC100, 0x14}, 0x94, 0x98, &kPk4},
        NdsSeries {4, "platinum", FooterSlots {0xCF2C, 0x14}, 0x9C, 0xA0, &kPk4},
        NdsSeries {4, "heartgold-soulsilver", FooterSlots {0xF628, 0x10}, 0x94, 0x98, &kPk4},
        NdsSeries {5, "black-white", ChecksumTable {0x24000, 0x8C, 0x18E00, 0x534, 0x19336, 0x34},
                   0x18E04, 0x18E08, &kPk5},
        NdsSeries {5, "black-2-white-2",
                   ChecksumTable {0x26000, 0x94, 0x18E00, 0x534, 0x19336, 0x34}, 0x18E04, 0x18E08,
                   &kPk5},
};

class PartyNdsReader final : public PartyReader
{
public:
    std::string_view platform() const override;
    // kNdsSeries를 차례로 readSeries — 체크섬이 맞는 첫 시리즈의 파티
    std::optional<ReadParty> read(Bytes save) const override;

    // 시리즈 하나로 읽기: ① 판별(findBase) → ② 파티 수 · 포켓몬. 그 시리즈가 아니면 nullopt.
    // 시리즈를 정해 놓고 확인하는 테스트 · readsav가 직접 부른다
    static std::optional<ReadParty> readSeries(Bytes save, const NdsSeries &series);

private:
    // 그 검증 방식으로 맞으면 파티 위치의 기준(base), 아니면 nullopt — 검증 방식마다 하나
    static std::optional<std::size_t> findBase(Bytes save, const FooterSlots &check);
    static std::optional<std::size_t> findBase(Bytes save, const ChecksumTable &check);
};
} // namespace com::yamada::studio::save
