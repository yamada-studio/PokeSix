#include "core/save/saveformat.h"

#include "core/save/gen5.h"

#include <array>

namespace com::yamada::studio::save {
namespace {
// TODO(H6-CP2-1) 형식 표: 4세대(readGen4Party)와 5세대(readGen5Party) 두 행.
//   constexpr std::array kFormats = {
//       SaveFormat{4, "4세대 NDS (DP · Pt · HGSS)", &readGen4Party},
//       SaveFormat{5, "…", &…},
//   };
//   함수 이름 앞의 &는 "그 함수를 가리키는 포인터"다 — 부르지 않고 주소만 표에 담는다
constexpr std::array<SaveFormat, 0> kFormats = {};
} // namespace

std::span<const SaveFormat> saveFormats()
{
    return kFormats;
}

std::optional<ReadParty> readParty(Bytes save)
{
    // TODO(H6-CP2-2) saveFormats()를 차례로 돌며 format.read(save)가 값을 주면 그것을 돌려준다.
    //   함수 포인터는 보통 함수처럼 부른다: format.read(save)
    (void)save;
    return std::nullopt;
}
} // namespace com::yamada::studio::save
