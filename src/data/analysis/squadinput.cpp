#include "data/analysis/squadinput.h"

#include "core/rules/generationfeatures.h"
#include "core/types/typekey.h"

namespace com::yamada::studio {
namespace {
std::optional<Type> toType(const QString &identifier)
{
    return typeFromKey(identifier.toStdString());
}

DamageClass toClass(int damageClass)
{
    switch (damageClass) {
    case int(DamageClass::Physical):
        return DamageClass::Physical;
    case int(DamageClass::Special):
        return DamageClass::Special;
    default:
        return DamageClass::Status;
    }
}
} // namespace

SquadInput makeSquadInput(const TypeChart &chart, int generation,
                          const std::array<std::optional<ResolvedMember>, kSquadSize> &members)
{
    SquadInput input;
    for (auto &row : input.chart)
        row.fill(1.0);
    for (const QString &attack : chart.types) {
        const std::optional<Type> a = toType(attack);
        if (!a)
            continue;
        input.types.push_back(*a);
        for (const QString &defense : chart.types)
            if (const std::optional<Type> d = toType(defense))
                input.chart[std::size_t(*a)][std::size_t(*d)] = chart.at(attack, defense);
    }

    const bool splitByMove = featuresOf(generation).splitByMove;
    for (std::size_t slot = 0; slot < kSquadSize; ++slot) {
        if (!members[slot])
            continue;
        MemberInput member;
        for (const QString &type : members[slot]->types)
            if (const std::optional<Type> t = toType(type))
                member.types.push_back(*t);
        for (const MoveEntry &move : members[slot]->moves) {
            MoveInput m;
            m.type = toType(move.type);
            m.damageClass = toClass(move.damageClass);
            // 반대 규칙: 4세대 이후라면 "타입 기준이었다면", 3세대까지라면 "기술마다였다면"
            if (m.damageClass == DamageClass::Status || !m.type)
                m.otherRuleClass = m.damageClass;
            else
                m.otherRuleClass
                        = splitByMove ? typeBasedClass(*m.type) : toClass(move.ownDamageClass);
            member.moves.push_back(m);
        }
        input.members[slot] = std::move(member);
    }
    return input;
}
} // namespace com::yamada::studio
