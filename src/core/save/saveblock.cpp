#include "core/save/saveblock.h"

#include "core/save/savebytes.h"

#include <utility> // std::pair

namespace com::yamada::studio::save {
BlockFooter readFooter(Bytes save, std::size_t blockStart, const SaveLayout &layout)
{
    BlockFooter footer;
    const std::size_t end = blockStart + layout.generalSize;
    if (end > save.size())
        return footer;
    footer.inFile = true;
    footer.major = readU32(save, end - 0x14);
    footer.minor = readU32(save, end - 0x10);
    footer.size = readU32(save, end - 0x0C);
    footer.magic = readU32(save, end - 0x08);
    footer.storedCrc = readU16(save, end - 0x02);
    footer.computedCrc
            = crc16Ccitt(save.subspan(blockStart, layout.generalSize - layout.footerSize));

    return footer;
}

std::optional<std::size_t> activeGeneralBlock(Bytes save, const SaveLayout &layout)
{
    const BlockFooter slot0Footer = readFooter(save, 0, layout);
    const BlockFooter slot1Footer = readFooter(save, kSlotSize, layout);
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
} // namespace com::yamada::studio::save
