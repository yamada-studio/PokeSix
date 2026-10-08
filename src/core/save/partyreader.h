#pragma once

#include "core/save/pkmcodec.h"
#include "core/save/savebytes.h"

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

// 세이브 → 파티 6마리 (H1 CP4 · CP5, H6). 가이드: docs/guides/h1-save-reader.md ·
// h6-multigen-save.md
//
// 이 파일은 **기기와 상관없는 부모**다: 결과 모양(ReadMember · ReadParty), 포켓몬 한 마리 해석
// (parseMember), 기기별 리더의 인터페이스(PartyReader), 그리고 앱이 쓰는 입구(readParty).
// 기기마다 세이브 구조가 다르므로 실제 읽기는 자식 클래스가 한다:
//
//   PartyReader (인터페이스, 여기)
//     └ PartyNdsReader   partyndsreader.h — NDS: 4세대(DP · Pt · HGSS) · 5세대(BW · B2W2)
//     (나중에) PartyGbaReader · Party3dsReader · PartySwitchReader — 기기마다 한 쌍
//
//   readParty(save)
//     for (리더 : partyReaders())     기기 순서
//         if (auto party = 리더->read(save))   리더가 자기 체크섬을 검증해 맞을 때만 값을 준다
//             return party;
//
// 파일 크기로 고르지 않는다 — 4 · 5세대는 둘 다 512 KiB다. 세대 · 시리즈는 리더 안의 표가
// 정하고(ReadParty::generation · versionGroup), 바깥은 그 값만 본다.
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
    // 게임 순서(0 노력 · 1 외로움 · 2 용감 · 3 고집 …). PK4 = pid % 25, PK5 = 0x41 바이트(H6)
    int nature = 0;
    std::array<int, 4> moves {}; // 0x28 u16 × 4: PokéAPI move id와 같다. 0 = 빈 칸
    std::array<int, 6> evs {}; // 0x18 u8 × 6: HP · 공격 · 방어 · 스피드 · 특공 · 특방
    std::array<int, 6> ivs {}; // 0x38 u32의 5비트 × 6, 같은 순서(비트 0–4가 HP)
    bool egg = false;          // 0x38 u32의 비트 30
    // 0x5F u8: 출신 게임 7 HG · 8 SS · 10 D · 11 P · 12 Pt · 20 W · 21 B · 22 W2 · 23 B2 (H2 · H6)
    int originGame = 0;
    int level = 0;              // 0x8C u8 (배틀 스탯)
    int hp = 0;                 // 0x8E u16
    int maxHp = 0;              // 0x90 u16
    bool checksumOk = false;    // DecodedPkm::ok()
    bool hiddenAbility = false; // PK5 0x42 비트 0 (PK4는 늘 false) (H6)
};

// 풀린 한 마리 → 칸들. 형식마다 다른 칸(성격 · 숨겨진 특성)은 format의 숫자로 읽는다
ReadMember parseMember(const DecodedPkm &pkm, const PkmFormat &format = kPk4);

// 기기 · 세대와 상관없는 결과 모양(H6). 4세대 전용 정보(슬롯 · footer)는 담지 않는다 — 그런 값이
// 필요하면 saveblock.h의 단계 함수를 직접 부른다(readsav처럼)
struct ReadParty
{
    int generation = 0;            // 4 · 5
    std::string_view versionGroup; // PokéAPI version_group ("heartgold-soulsilver" · "black-white")
    std::size_t partyOffset = 0; // 파일에서 첫 포켓몬이 시작하는 위치(로그 · 디버깅용)
    std::vector<ReadMember> members; // 파티 수만큼(1–6)
};

// 기기 하나의 세이브 리더. ROS 2 pluginlib의 base class처럼, 기기마다 자식 클래스가 구현한다
class PartyReader
{
public:
    virtual ~PartyReader() = default;
    // "nds" · "gba" … — 로그용
    virtual std::string_view platform() const = 0;
    // 이 기기의 세이브면 파티, 아니면(어느 시리즈의 체크섬도 안 맞으면) nullopt
    virtual std::optional<ReadParty> read(Bytes save) const = 0;
};

// 시도할 기기 리더들(기기 순서). 지금은 NDS 하나
std::span<const PartyReader *const> partyReaders();

// 기기 · 세대를 모르는 세이브에서 파티를 읽는다 — 앱(saveimport)이 쓰는 입구
std::optional<ReadParty> readParty(Bytes save);
} // namespace com::yamada::studio::save
