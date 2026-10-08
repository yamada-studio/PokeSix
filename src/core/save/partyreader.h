#pragma once

#include "core/save/pkmcodec.h"
#include "core/save/saveblock.h"

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

// 세이브 → 파티 6마리 (H1 CP4 · CP5). 가이드: docs/guides/h1-save-reader.md
//
// 여기서 나오는 번호는 **게임 내부 번호**다. 종 · 기술 · 특성은 PokéAPI id와 같지만, 지닌
// 물건 · 성격은 다르다 — PokéAPI id로 바꾸는 것은 data 레이어(H2)의 일이다.
namespace com::yamada::studio::save {
struct ReadMember
{
    std::uint32_t pid = 0;
    int species = 0;  // 0x08 u16: 전국도감 번호 = PokéAPI pokemon_species.id
    int heldItem = 0; // 0x0A u16: 게임 내부 아이템 번호(PokéAPI id 아님). 0 = 없음
    int ability = 0;  // 0x15 u8: PokéAPI ability id와 같다
    int form = 0;     // 0x40 u8의 위 5비트(>> 3). 0 = 기본 폼
    // 게임 순서(0 노력 · 1 외로움 · 2 용감 · 3 고집 …). 4세대 = pid % 25, 5세대 = 0x41 바이트(H6)
    int nature = 0;
    std::array<int, 4> moves {}; // 0x28 u16 × 4: PokéAPI move id와 같다. 0 = 빈 칸
    std::array<int, 6> evs {}; // 0x18 u8 × 6: HP · 공격 · 방어 · 스피드 · 특공 · 특방
    std::array<int, 6> ivs {};  // 0x38 u32의 5비트 × 6, 같은 순서(비트 0–4가 HP)
    bool egg = false;           // 0x38 u32의 비트 30
    int originGame = 0;         // 0x5F u8: 출신 게임 7 HG · 8 SS · 10 D · 11 P · 12 Pt (H2)
    int level = 0;              // 0x8C u8 (배틀 스탯)
    int hp = 0;                 // 0x8E u16
    int maxHp = 0;              // 0x90 u16
    bool checksumOk = false;    // DecodedPkm::ok()
    bool hiddenAbility = false; // 0x42 비트 0 — 5세대부터(4세대는 늘 false) (H6)
};

ReadMember parseMember(const DecodedPkm &pkm);

// 세대와 상관없는 결과 모양(H6). 세대별 형식(4세대 · 5세대 …)이 모두 이 모양으로 돌려준다 —
// 4세대 전용 정보(슬롯 · footer)는 담지 않는다. 그런 값이 필요하면 그 세대의 단계 함수를 직접
// 부른다
struct ReadParty
{
    int generation = 0;            // 4 · 5
    std::string_view versionGroup; // PokéAPI version_group ("heartgold-soulsilver" · "black-white")
    std::size_t partyOffset = 0; // 파일에서 첫 포켓몬이 시작하는 위치(로그 · 디버깅용)
    std::vector<ReadMember> members; // 파티 수만큼(1–6)
};

// 4세대 형식의 읽기(H1). kGen4Layouts를 차례로 activeGeneralBlock에 넣어 CRC가 맞는 첫 행을
// 그 게임으로 본다. 크기가 kSaveSize보다 작거나, 맞는 행이 없거나, 파티 수가 1–6이 아니면 nullopt.
// 세대를 모르는 세이브는 이것 대신 saveformat.h의 readParty(모든 형식을 차례로 시도)를 쓴다
std::optional<ReadParty> readGen4Party(Bytes save);
} // namespace com::yamada::studio::save
