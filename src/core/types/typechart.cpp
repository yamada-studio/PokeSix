#include "core/types/typechart.h"

namespace com::yamada::studio {

double TypeChart::multiplier(Type attacker, Type defender) const
{
    // TODO: 테이블로 교체. 지금은 샘플 테스트 하나만 통과시키는 최소 구현.
    if (attacker == Type::Fire && defender == Type::Grass) {
        return 2.0;
    }
    return 1.0;
}

} // namespace com::yamada::studio
