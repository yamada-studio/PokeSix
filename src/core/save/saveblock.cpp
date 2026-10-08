#include "core/save/saveblock.h"

#include "core/save/savebytes.h"

namespace com::yamada::studio::save {
BlockFooter readFooter(Bytes save, std::size_t blockStart, const SaveLayout &layout)
{
    BlockFooter footer;
    const std::size_t end = blockStart + layout.generalSize;
    if (end > save.size())
        return footer;
    // TODO(CP2-1) footer의 다섯 칸을 readU32 · readU16으로 읽는다. 위치는 saveblock.h의 주석
    //   (끝 − 0x14 · − 0x10 · − 0x0C · − 0x08 · − 0x02)
    footer.major = readU32(save, end - 0x14);
    footer.minor = readU32(save, end - 0x10);
    footer.size = readU32(save, end - 0x0C);
    footer.magic = readU32(save, end - 0x08);
    footer.storedCrc = readU16(save, end - 0x02);
    // TODO(CP2-2) computedCrc = crc16Ccitt(블록 데이터). 범위는 [blockStart, end − footerSize) —
    //   save.subspan(시작, 길이)로 자른다
    footer.computedCrc = crc16Ccitt(save.subspan(blockStart, layout.generalSize - layout.footerSize));

    return footer;
}

std::optional<std::size_t> activeGeneralBlock(Bytes save, const SaveLayout &layout)
{
    // TODO(CP2-3) 두 슬롯의 footer를 읽는다: readFooter(save, 0, layout) · readFooter(save,
    //   kSlotSize, layout)
    const BlockFooter slot0Footer = readFooter(save, 0, layout);
    const BlockFooter slot1Footer = readFooter(save, kSlotSize, layout);
    // TODO(CP2-4) CRC가 하나만 맞으면 그 슬롯. 둘 다 틀리면 std::nullopt
    // TODO(CP2-5) 둘 다 맞으면 major가 큰 쪽, 같으면 minor가 큰 쪽(그것도 같으면 슬롯 0).
    //   std::pair{major, minor}끼리 > 로 비교하면 한 줄로 된다
    if (slot0Footer.crcOk() && slot1Footer.crcOk()) {
        if (std::pair{slot0Footer.major, slot0Footer.minor} > std::pair{slot1Footer.major, slot1Footer.minor}) {
            return 0;
        } else if (std::pair{slot0Footer.major, slot0Footer.minor} < std::pair{slot1Footer.major, slot1Footer.minor}) {
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

    return std::nullopt;
}
} // namespace com::yamada::studio::save
