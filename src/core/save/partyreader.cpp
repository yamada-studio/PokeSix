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

std::optional<ReadParty> readGen4Party(Bytes save)
{
    if (save.size() < kSaveSize)
        return std::nullopt;
    for (const SaveLayout &layout : kGen4Layouts) {
        if (const auto start = activeGeneralBlock(save, layout)) {
            ReadParty party;
            const std::size_t general = *start; // 고른 슬롯의 일반 블록 시작(0 또는 kSlotSize)
            // TODO(H6-CP1) 세대와 상관없는 칸 세 개를 채운다:
            //   party.generation = 4;  party.versionGroup = layout.versionGroup;
            //   party.partyOffset = general + layout.partyOffset  (파일에서 첫 포켓몬의 위치)
            const int count = save[general + layout.partyCountOffset];
            if (count < 1 || count > 6) {
                return std::nullopt;
            }
            for (int i = 0; i < count; ++i) {
                const std::size_t at
                        = general + layout.partyOffset + std::size_t(i) * kPartyPkmSize;
                const DecodedPkm pkm = decodePkm(save.subspan(at, kPartyPkmSize));
                party.members.push_back(parseMember(pkm));
            }
            return party;
        }
    }
    return std::nullopt;
}
} // namespace com::yamada::studio::save
