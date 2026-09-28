#pragma once

#include "core/types/type.h"

namespace com::yamada::studio {

// 공격 타입 → 방어 타입 상성 배율.
//
// 현재는 빌드·테스트 파이프라인 확인용 골격이다. 채워야 할 것:
//   - 18 x 18 배율 테이블
//   - 세대별 차이 (2세대 악/강철 추가, 6세대 페어리 추가 및 강철 내성 변경)
//     → 세대를 어떻게 표현하고 테이블을 어떻게 고를지는 설계 과제
//   - 복합 타입 방어 (배율의 곱)
class TypeChart
{
public:
    // 단일 타입 방어자에 대한 배율: 0, 0.5, 1, 2
    double multiplier(Type attacker, Type defender) const;
};

} // namespace com::yamada::studio
