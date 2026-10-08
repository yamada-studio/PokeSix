#include "core/save/partyndsreader.h"

namespace com::yamada::studio::save {
std::string_view PartyNdsReader::platform() const
{
    return "nds";
}

std::optional<ReadParty> PartyNdsReader::read(Bytes save) const
{
    // TODO(H6-CP2-1) kNdsSeries를 차례로 readSeries(save, series)에 넣어 값이 나오면 돌려준다.
    //   끝까지 없으면 nullopt (H1의 kGen4Layouts 반복과 같은 모양)
    (void)save;
    return std::nullopt;
}

std::optional<ReadParty> PartyNdsReader::readSeries(Bytes save, const NdsSeries &series)
{
    if (save.size() < kSaveSize)
        return std::nullopt;
    // TODO(H6-CP2-2) ① 판별: series.check에 든 검증 방식에 맞는 findBase를 std::visit으로 부른다
    //   const std::optional<std::size_t> base = std::visit(
    //           [&](const auto &check) { return findBase(save, check); }, series.check);
    //   람다의 auto 인자는 variant에 든 실제 타입(FooterSlots 또는 ChecksumTable)이 되고, 그 타입에
    //   맞는 findBase 오버로드가 컴파일 때 정해진다
    const std::optional<std::size_t> base;
    if (!base)
        return std::nullopt;

    ReadParty party;
    party.generation = series.generation;
    party.versionGroup = series.versionGroup;
    party.partyOffset = *base + series.partyOffset;

    const int count = save[*base + series.partyCountOffset];
    if (count < 1 || count > 6)
        return std::nullopt;
    for (int i = 0; i < count; ++i) {
        // TODO(H6-CP4-3) ② 추출: 한 마리 크기를 kPartyPkmSize(PK4 고정) 대신
        //   series.pkm->partySize로, parseMember에는 형식(*series.pkm)을 넘긴다
        //   — 그래야 BW · B2W2가 PK5로 읽힌다
        const std::size_t at = party.partyOffset + std::size_t(i) * kPartyPkmSize;
        const DecodedPkm pkm = decodePkm(save.subspan(at, kPartyPkmSize));
        party.members.push_back(parseMember(pkm));
    }
    return party;
}

std::optional<std::size_t> PartyNdsReader::findBase(Bytes save, const FooterSlots &check)
{
    // TODO(H6-CP2-3) 4세대식: H1의 activeGeneralBlock이 바로 이것이다 — 한 줄
    (void)save;
    (void)check;
    return std::nullopt;
}

std::optional<std::size_t> PartyNdsReader::findBase(Bytes save, const ChecksumTable &check)
{
    // TODO(H6-CP3-3) 5세대식: checksumTableValid와 partyBlockValid가 둘 다 맞으면 0(파일 시작이
    //   기준), 아니면 nullopt
    (void)save;
    (void)check;
    return std::nullopt;
}
} // namespace com::yamada::studio::save
