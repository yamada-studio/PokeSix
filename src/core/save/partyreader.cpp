#include "core/save/partyreader.h"

#include "core/save/partyndsreader.h"

namespace com::yamada::studio::save {
ReadMember parseMember(const DecodedPkm &pkm, const PkmFormat &format)
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
    // TODO(H6-CP4-2) 형식이 자리를 알려 주면(optional에 값이 있으면) 그 자리에서 읽는다:
    //   format.natureAt이 있으면 member.nature = 그 바이트(5세대는 성격을 따로 저장한다)
    //   format.hiddenAbilityAt이 있으면 member.hiddenAbility = 그 바이트의 비트 0
    (void)format;
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

std::span<const PartyReader *const> partyReaders()
{
    // TODO(H6-CP1-1) 기기 리더 목록. 리더 객체와 목록을 함수 안의 static으로 둔다:
    //   static const PartyNdsReader nds;
    //   static const std::array<const PartyReader *, 1> readers = {&nds};
    //   return readers;
    // 자식(PartyNdsReader)의 주소를 부모 포인터(const PartyReader *)로 담는다 — 부르는 쪽은 어느
    // 기기인지 몰라도 reader->read(save)만 부르면 자식의 read가 불린다(가상 함수)
    return {};
}

std::optional<ReadParty> readParty(Bytes save)
{
    // TODO(H6-CP1-2) partyReaders()를 차례로 돌며 reader->read(save)가 값을 주면 그것을 돌려준다.
    //   끝까지 없으면 nullopt
    (void)save;
    return std::nullopt;
}
} // namespace com::yamada::studio::save
