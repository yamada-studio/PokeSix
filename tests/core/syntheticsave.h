#pragma once

// 테스트용 합성 세이브. 진짜 세이브는 리포에 넣지 않으므로(개인 데이터 · 게임 데이터) 테스트가
// 알려진 포켓몬으로 세이브를 직접 만든다 — 파서의 역방향(평문 → 섞기 → 암호화 → footer).
//
// 낮은 층 함수(crc16Ccitt · cryptArray · pkmChecksum)는 각자의 테스트가 **외부 검증 값**으로 먼저
// 확인한다(savebytes_test · pkmcodec_test). 이 도우미는 그 함수들을 그대로 써서 위층(블록 ·
// 파티) 테스트용 바이트를 만든다. 그래서 CP 순서대로 통과시키면 된다.
#include "core/save/gen5.h"
#include "core/save/partyreader.h"

#include <algorithm>
#include <optional>
#include <vector>

namespace synth {
using namespace com::yamada::studio::save;

inline void putU16(std::span<std::uint8_t> data, std::size_t offset, std::uint16_t value)
{
    data[offset] = std::uint8_t(value & 0xFF);
    data[offset + 1] = std::uint8_t(value >> 8);
}

inline void putU32(std::span<std::uint8_t> data, std::size_t offset, std::uint32_t value)
{
    putU16(data, offset, std::uint16_t(value & 0xFFFF));
    putU16(data, offset + 2, std::uint16_t(value >> 16));
}

struct MemberSpec
{
    std::uint32_t pid = 0x12345678;
    std::uint16_t species = 392; // 초염몽
    std::uint16_t heldItem = 0;
    std::uint8_t ability = 66; // 맹화
    std::uint8_t formByte = 0; // 비트 0 운명 · 1–2 성별 · 3–7 폼
    std::array<std::uint16_t, 4> moves {7, 53, 394, 0};
    std::array<std::uint8_t, 6> evs {};
    std::uint32_t iv32 = 0;
    std::uint8_t level = 50;
    std::uint16_t hp = 100;
    std::uint16_t maxHp = 120;
    std::uint8_t originGame = 8; // 0x5F 출신 게임(8 = 소울실버)
    std::uint8_t natureByte = 0; // 0x41 — 5세대의 성격(4세대에서는 빛나는 잎 자리, 테스트는 0)
    bool hiddenAbility = false; // 0x42 비트 0 — 5세대
};

// 평문 PKM(블록 A · B · C · D 순서, 체크섬 포함)
inline Pkm plainPkm(const MemberSpec &m)
{
    Pkm p {};
    putU32(p, 0x00, m.pid);
    putU16(p, 0x08, m.species);
    putU16(p, 0x0A, m.heldItem);
    p[0x15] = m.ability;
    for (std::size_t k = 0; k < 6; ++k)
        p[0x18 + k] = m.evs[k];
    for (std::size_t k = 0; k < 4; ++k)
        putU16(p, 0x28 + 2 * k, m.moves[k]);
    putU32(p, 0x38, m.iv32);
    p[0x40] = m.formByte;
    p[0x41] = m.natureByte;
    p[0x42] = m.hiddenAbility ? 1 : 0;
    p[0x5F] = m.originGame;
    p[0x8C] = m.level;
    putU16(p, 0x8E, m.hp);
    putU16(p, 0x90, m.maxHp);
    putU16(p, 0x06, pkmChecksum(p));
    return p;
}

// 세이브에 들어 있는 모양: 블록을 PID 순서로 섞고 두 영역을 암호화한다
inline Pkm encodePkm(const Pkm &plain)
{
    Pkm out = plain;
    const std::uint32_t pid = readU32(plain, 0x00);
    const std::uint16_t checksum = readU16(plain, 0x06);
    const int s = int(((pid >> 13) & 0x1F) % 24); // 파서의 shuffleIndex와 독립으로 계산
    for (std::size_t i = 0; i < 4; ++i) {
        const std::size_t pos = kBlockPosition[std::size_t(s)][i];
        std::copy_n(plain.begin() + std::ptrdiff_t(kHeaderSize + i * kBlockSize), kBlockSize,
                    out.begin() + std::ptrdiff_t(kHeaderSize + pos * kBlockSize));
    }
    cryptArray(std::span(out).subspan(kHeaderSize, kBlocksSize), checksum);
    cryptArray(std::span(out).subspan(kStoredSize, kPartyPkmSize - kStoredSize), pid);
    return out;
}

struct SlotSpec
{
    std::uint32_t major = 1;
    std::uint32_t minor = 0;
    std::vector<MemberSpec> party = {};
    bool corrupt = false; // CRC를 쓴 뒤 데이터 한 바이트를 바꾼다
};

inline void writeSlot(std::vector<std::uint8_t> &save, std::size_t start, const SaveLayout &layout,
                      const SlotSpec &slot)
{
    const std::span block(save.data() + start, layout.generalSize);
    std::fill(block.begin(), block.end(), std::uint8_t {0});
    block[layout.partyCountOffset] = std::uint8_t(slot.party.size());
    for (std::size_t i = 0; i < slot.party.size(); ++i) {
        const Pkm pkm = encodePkm(plainPkm(slot.party[i]));
        std::copy(pkm.begin(), pkm.end(),
                  block.begin() + std::ptrdiff_t(layout.partyOffset + i * kPartyPkmSize));
    }
    const std::size_t end = layout.generalSize;
    putU32(block, end - 0x14, slot.major);
    putU32(block, end - 0x10, slot.minor);
    putU32(block, end - 0x0C, std::uint32_t(layout.generalSize));
    putU32(block, end - 0x08, 0x20060623);
    putU16(block, end - 0x02, crc16Ccitt(block.first(layout.generalSize - layout.footerSize)));
    if (slot.corrupt)
        block[0x10] ^= 0xFF;
}

// 슬롯을 비워 두면(nullopt) 한 번도 안 쓴 플래시처럼 0xFF로 남는다
inline std::vector<std::uint8_t> buildSave(const SaveLayout &layout,
                                           const std::optional<SlotSpec> &slot0,
                                           const std::optional<SlotSpec> &slot1 = std::nullopt)
{
    std::vector<std::uint8_t> save(kSaveSize, 0xFF);
    if (slot0)
        writeSlot(save, 0, layout, *slot0);
    if (slot1)
        writeSlot(save, kSlotSize, layout, *slot1);
    return save;
}

inline const SaveLayout &layoutOf(std::string_view versionGroup)
{
    for (const SaveLayout &layout : kGen4Layouts)
        if (layout.versionGroup == versionGroup)
            return layout;
    return kGen4Layouts[0];
}
// ── 5세대(H6) ────────────────────────────────────────────────
// 본 세이브의 파티 블록 + 체크섬 모음 블록만 채운 BW · B2W2 세이브(나머지는 0).
// 순서가 중요하다: 파티 CRC를 먼저 계산해 두 곳(블록 뒤 · 모음 블록의 사본)에 쓰고, 그 사본이 든
// 모음 블록 전체의 CRC를 마지막에 쓴다
struct Gen5SaveSpec
{
    std::vector<MemberSpec> party = {};
    bool corruptInfo = false; // 모음 블록 CRC를 쓴 뒤 그 안의 한 바이트를 바꾼다
    bool corruptParty = false; // 파티 블록 CRC를 쓴 뒤 파티 블록 한 바이트를 바꾼다
};

inline std::vector<std::uint8_t> buildGen5Save(const Gen5Layout &layout, const Gen5SaveSpec &spec)
{
    std::vector<std::uint8_t> save(kSaveSize, 0);
    const std::span all(save);
    all[kGen5PartyBlock + kGen5PartyCountOffset] = std::uint8_t(spec.party.size());
    for (std::size_t i = 0; i < spec.party.size(); ++i) {
        // 220바이트 = 236바이트 암호문의 앞부분(배틀 스탯 스트림은 앞에서부터 같은 키라 잘라도
        // 맞다)
        const Pkm pkm = encodePkm(plainPkm(spec.party[i]));
        std::copy_n(pkm.begin(), kGen5PartyPkmSize,
                    save.begin()
                            + std::ptrdiff_t(kGen5PartyBlock + kGen5PartyOffset
                                             + i * kGen5PartyPkmSize));
    }
    const std::uint16_t partyCrc = crc16Ccitt(all.subspan(kGen5PartyBlock, kGen5PartyBlockSize));
    putU16(all, kGen5PartyCrc, partyCrc);
    const std::size_t info = layout.mainSize - 0x100;
    putU16(all, info + kGen5PartyCrcMirror, partyCrc);
    putU16(all, info + layout.infoLength + 0x0E, crc16Ccitt(all.subspan(info, layout.infoLength)));
    if (spec.corruptInfo)
        save[info + 0x10] ^= 0xFF;
    if (spec.corruptParty)
        save[kGen5PartyBlock + kGen5PartyOffset + 0x20] ^= 0xFF;
    return save;
}

inline const Gen5Layout &gen5LayoutOf(std::string_view versionGroup)
{
    for (const Gen5Layout &layout : kGen5Layouts)
        if (layout.versionGroup == versionGroup)
            return layout;
    return kGen5Layouts[0];
}
} // namespace synth
