#include "core/save/partyreader.h"

namespace com::yamada::studio::save {
ReadMember parseMember(const DecodedPkm &pkm)
{
    ReadMember member;
    const Bytes data(pkm.data);
    member.pid = pkm.pid;
    member.checksumOk = pkm.ok();
    member.species = readU16(data, 0x08);
    member.heldItem = readU16(data, 0x0A);
    member.ability = data[0x15];
    member.form = data[0x40] >> 3;
    member.nature = member.pid % 25;
    for (int k = 0; k < 4; ++k) {
        member.moves[k] = readU16(data, 0x28 + 2 * k);
    }
    for (int k = 0; k < 6; ++k) {
        member.evs[k] = data[0x18 + k];
    }
    member.egg = (readU32(data, 0x38) >> 30) & 1;
    const std::uint32_t iv32 = readU32(data, 0x38);
    for (int k = 0; k < 6; ++k) {
        member.ivs[k] = (iv32 >> (5 * k)) & 0x1F;
    }
    member.originGame = data[0x5F];
    member.level = data[0x8C];
    member.hp = readU16(data, 0x8E);
    member.maxHp = readU16(data, 0x90);
    return member;
}

std::optional<ReadParty> readParty(Bytes save)
{
    if (save.size() < kSaveSize)
        return std::nullopt;
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
                const std::size_t at
                        = party.generalStart + layout.partyOffset + std::size_t(i) * kPartyPkmSize;
                const DecodedPkm pkm = decodePkm(save.subspan(at, kPartyPkmSize));
                party.members.push_back(parseMember(pkm));
            }
            return party;
        }
    }
    return std::nullopt;
}
} // namespace com::yamada::studio::save
