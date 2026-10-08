#include "core/save/gen5.h"

#include "core/save/pkmcodec.h"
#include "core/save/saveblock.h"

namespace com::yamada::studio::save {
std::size_t infoBlockStart(const Gen5Layout &layout)
{
    return layout.mainSize - 0x100;
}

bool infoBlockValid(Bytes save, const Gen5Layout &layout)
{
    // TODO(H6-CP3-1) 체크섬 모음 블록 = [infoBlockStart, + infoLength). 그 CRC(crc16Ccitt)가
    //   infoBlockStart + infoLength + 0x0E에 저장된 u16과 같은가. 파일이 그 끝보다 짧으면 false
    //   (H1에서 겪은 "파일 밖을 읽으면 0 = 0으로 맞아 보인다"를 먼저 막는다)
    (void)save;
    (void)layout;
    return false;
}

bool partyBlockValid(Bytes save, const Gen5Layout &layout)
{
    // TODO(H6-CP3-2) 파티 블록 [kGen5PartyBlock, + kGen5PartyBlockSize)의 CRC를 계산해서
    //   ① kGen5PartyCrc에 저장된 u16  ② infoBlockStart(layout) + kGen5PartyCrcMirror에 저장된 u16
    //   둘 다와 같은가
    (void)save;
    (void)layout;
    return false;
}

std::optional<ReadParty> readGen5Party(Bytes save)
{
    if (save.size() < kSaveSize)
        return std::nullopt;
    // TODO(H6-CP3-3) 판별: kGen5Layouts 중 infoBlockValid인 첫 행. 없으면 nullopt
    // TODO(H6-CP4-2) 그 행으로 partyBlockValid가 아니면 nullopt
    // TODO(H6-CP4-3) 파티 수 = save[kGen5PartyBlock + kGen5PartyCountOffset], 1–6이 아니면 nullopt
    // TODO(H6-CP4-4) i번째 포켓몬 = kGen5PartyBlock + kGen5PartyOffset + i ×
    // kGen5PartyPkmSize(220).
    //   decodePkm → parseMember(4 · 5세대 공통 칸) 뒤에 5세대만 다른 두 칸을 덮어쓴다:
    //     member.nature = 풀린 data[0x41]        (5세대는 성격을 PID로 계산하지 않고 따로 저장한다)
    //     member.hiddenAbility = data[0x42]의 비트 0
    // TODO(H6-CP4-5) ReadParty { generation = 5, versionGroup = 행의 versionGroup,
    //   partyOffset = kGen5PartyBlock + kGen5PartyOffset, members }
    return std::nullopt;
}
} // namespace com::yamada::studio::save
