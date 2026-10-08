#include "core/save/saveblock.h"

#include "core/save/savebytes.h"

#include <utility> // std::pair

namespace com::yamada::studio::save {
BlockFooter readFooter(Bytes save, std::size_t blockStart, const FooterSlots &check)
{
    BlockFooter footer;
    const std::size_t end = blockStart + check.generalSize;
    if (end > save.size())
        return footer;
    footer.inFile = true;
    footer.major = readU32(save, end - 0x14);
    footer.minor = readU32(save, end - 0x10);
    footer.size = readU32(save, end - 0x0C);
    footer.magic = readU32(save, end - 0x08);
    footer.storedCrc = readU16(save, end - 0x02);
    footer.computedCrc = crc16Ccitt(save.subspan(blockStart, check.generalSize - check.footerSize));

    return footer;
}

std::optional<std::size_t> activeGeneralBlock(Bytes save, const FooterSlots &check)
{
    const BlockFooter slot0Footer = readFooter(save, 0, check);
    const BlockFooter slot1Footer = readFooter(save, kSlotSize, check);
    if (slot0Footer.crcOk() && slot1Footer.crcOk()) {
        if (std::pair {slot0Footer.major, slot0Footer.minor}
            > std::pair {slot1Footer.major, slot1Footer.minor}) {
            return 0;
        } else if (std::pair {slot0Footer.major, slot0Footer.minor}
                   < std::pair {slot1Footer.major, slot1Footer.minor}) {
            return kSlotSize;
        } else {
            return 0;
        }
    } else if (!slot0Footer.crcOk() && slot1Footer.crcOk()) {
        return kSlotSize;
    } else if (slot0Footer.crcOk() && !slot1Footer.crcOk()) {
        return 0;
    } else {
        return std::nullopt;
    }
}

std::size_t checksumTableStart(const ChecksumTable &check)
{
    return check.mainSize - 0x100;
}

bool checksumTableValid(Bytes save, const ChecksumTable &check)
{
    // TODO(H6-CP3-1) 모음 블록 = [checksumTableStart, + tableLength). 그 CRC(crc16Ccitt)가
    //   checksumTableStart + tableLength + 0x0E에 저장된 u16과 같은가. 파일이 그 u16의 끝보다
    //   짧으면 먼저 false — readFooter의 inFile과 같은 이유(파일 밖은 0 = 0으로 맞아 보인다)
    (void)save;
    (void)check;
    return false;
}

bool partyBlockValid(Bytes save, const ChecksumTable &check)
{
    // TODO(H6-CP3-2) 파티 블록 [partyBlock, + partyBlockSize)의 CRC를 계산해서
    //   ① partyCrcAt에 저장된 u16  ② checksumTableStart + partyCrcMirror에 저장된 u16
    //   둘 다와 같은가. 여기서도 두 위치가 파일 안에 있는지부터
    (void)save;
    (void)check;
    return false;
}
} // namespace com::yamada::studio::save
