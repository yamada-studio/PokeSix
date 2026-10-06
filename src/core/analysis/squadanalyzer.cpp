#include "core/analysis/squadanalyzer.h"

namespace com::yamada::studio {
namespace {
constexpr std::size_t index(Type type)
{
    return static_cast<std::size_t>(type);
}

void count(MoveSplit &split, DamageClass damageClass)
{
    switch (damageClass) {
    case DamageClass::Physical:
        ++split.physical;
        break;
    case DamageClass::Special:
        ++split.special;
        break;
    case DamageClass::Status:
        ++split.status;
        break;
    }
}
} // namespace

SquadAnalysis analyzeSquad(const SquadInput &input)
{
    SquadAnalysis result;
    for (auto &row : result.received)
        row.fill(1.0);

    // 1) 받는 배율 = 방어 타입마다 배율의 곱. 약점 · 내성 수
    for (std::size_t slot = 0; slot < kSquadSize; ++slot) {
        const std::optional<MemberInput> &member = input.members[slot];
        if (!member)
            continue;
        ++result.filled;
        for (const Type attack : input.types) {
            double multiplier = 1.0;
            for (const Type defense : member->types)
                multiplier *= input.chart[index(attack)][index(defense)];
            result.received[slot][index(attack)] = multiplier;
            if (multiplier > 1.0)
                ++result.weak[index(attack)];
            else if (multiplier < 1.0)
                ++result.resist[index(attack)];
        }

        // 2) 기술 분류(빈 칸 포함 4칸)
        for (const MoveInput &move : member->moves)
            count(result.split, move.damageClass);
        const int empty = int(kMoveSlots) - int(member->moves.size());
        result.split.empty += empty > 0 ? empty : 0;

        // 3) 공격 커버: 공격 기술의 타입이 그 (단일) 방어 타입에 효과가 굉장한가
        for (const MoveInput &move : member->moves) {
            if (move.damageClass == DamageClass::Status || !move.type)
                continue;
            result.hasAttackingMove = true;
            for (const Type defense : input.types)
                if (input.chart[index(*move.type)][index(defense)] > 1.0)
                    result.covered[index(defense)] = true;
        }
    }

    // 4) 문제: 방어(종류 순 → 타입 순) → 공격
    auto slotsWhere = [&](Type attack, auto predicate) {
        std::vector<int> members;
        for (std::size_t slot = 0; slot < kSquadSize; ++slot)
            if (input.members[slot] && predicate(result.received[slot][index(attack)]))
                members.push_back(int(slot));
        return members;
    };
    for (const ProblemKind kind :
         {ProblemKind::WeakStack, ProblemKind::NoResist, ProblemKind::Quad}) {
        for (const Type type : input.types) {
            const int weak = result.weak[index(type)];
            const int resist = result.resist[index(type)];
            bool hit = false;
            switch (kind) {
            case ProblemKind::WeakStack:
                hit = weak >= 3;
                break;
            case ProblemKind::NoResist: // 약점 3 이상이면 WeakStack 하나로 충분하다
                hit = weak == 2 && resist == 0;
                break;
            case ProblemKind::Quad:
                hit = !slotsWhere(type, [](double m) { return m >= 4.0; }).empty();
                break;
            case ProblemKind::NoCoverage:
                break;
            }
            if (!hit)
                continue;
            result.problems.push_back(
                    {kind, type,
                     kind == ProblemKind::Quad
                             ? slotsWhere(type, [](double m) { return m >= 4.0; })
                             : slotsWhere(type, [](double m) { return m > 1.0; })});
        }
    }
    if (result.hasAttackingMove)
        for (const Type type : input.types)
            if (!result.covered[index(type)])
                result.problems.push_back({ProblemKind::NoCoverage, type, {}});
    return result;
}
} // namespace com::yamada::studio
