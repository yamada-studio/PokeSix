#include "core/save/partyreader.h"

namespace com::yamada::studio::save {
ReadMember parseMember(const DecodedPkm &pkm)
{
    ReadMember member;
    const Bytes data(pkm.data);
    member.pid = pkm.pid;
    member.checksumOk = pkm.ok();
    // TODO(CP4-1) species · heldItem(readU16) · ability(data[0x15]) — 위치는 partyreader.h 주석
    member.species = readU16(data, 0x08);
    member.heldItem = readU16(data, 0x0A);
    member.ability = data[0x15];
    // TODO(CP4-2) form = data[0x40] >> 3,  nature = pid % 25
    member.form = data[0x40] >> 3;
    member.nature = member.pid % 25;
    // TODO(CP4-3) moves[k] = readU16(data, 0x28 + 2 * k)   (k = 0..3)
    for (int k = 0; k < 4; ++k) {
        member.moves[k] = readU16(data, 0x28 + 2 * k);
    }
    // TODO(CP4-4) evs[k] = data[0x18 + k]                   (k = 0..5)
    for (int k = 0 ; k < 6 ; ++k) {
        member.evs[k] = data[0x18 + k];
    }
    // TODO(CP4-5) iv32 = readU32(data, 0x38). ivs[k] = (iv32 >> (5 * k)) & 0x1F,
    //   egg = 비트 30이 켜져 있는가 ((iv32 >> 30) & 1)
    member.egg = (readU32(data, 0x38) >> 30) & 1;
    const std::uint32_t iv32 = readU32(data, 0x38);
    for (int k = 0; k < 6; ++k) {
        member.ivs[k] = (iv32 >> (5 * k)) & 0x1F;
    }
    // TODO(CP4-6) level = data[0x8C], hp = readU16(data, 0x8E), maxHp = readU16(data, 0x90)
    member.level = data[0x8C];
    member.hp = readU16(data, 0x8E);
    member.maxHp = readU16(data, 0x90);
    return member;
}

std::optional<ReadParty> readParty(Bytes save)
{
    if (save.size() < kSaveSize)
        return std::nullopt;
    // TODO(CP5-1) for (const SaveLayout &layout : kGen4Layouts): activeGeneralBlock(save, layout)이
    //   값을 주는 첫 layout을 찾는다. 하나도 없으면 std::nullopt
    // TODO(CP5-2) ReadParty를 채운다: layout = &layout, generalStart, footer = readFooter(…)
    // TODO(CP5-3) count = save[generalStart + layout.partyCountOffset]. 1–6이 아니면 std::nullopt
    // TODO(CP5-4) i = 0..count−1: 시작 = generalStart + layout.partyOffset + i * kPartyPkmSize,
    //   save.subspan(시작, kPartyPkmSize) → decodePkm → parseMember → members.push_back
    for (const SaveLayout &layout : kGen4Layouts) {
        if (const auto start = activeGeneralBlock(save, layout)) {
            ReadParty party;
            party.layout = &layout;
            party.generalStart = *start;
            party.footer = readFooter(save, *start, layout);
            const int count = save[party.generalStart + layout.partyCountOffset];
            if (count < 1 || count > 6) {
                return std::nullopt;
            }
            for (int i = 0; i < count; ++i) {
                const std::size_t at = party.generalStart + layout.partyOffset + std::size_t(i) * kPartyPkmSize;
                const DecodedPkm pkm = decodePkm(save.subspan(at, kPartyPkmSize));
                party.members.push_back(parseMember(pkm));
            }
            return party;
        }
    }
    return std::nullopt;
}
} // namespace com::yamada::studio::save
