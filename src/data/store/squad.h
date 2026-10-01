#pragma once

#include <QList>
#include <QString>

#include <array>

namespace com::yamada::studio {
// 스쿼드 한 자리. id만 저장한다(이름 · 타입 · 위력은 그 세대 값으로 DB에서 다시 읽는다).
struct SquadMember
{
    int pokemonId = 0;           // 0 = 빈 자리
    QString memo;                // 역할 메모(최대 kMemoLength자)
    std::array<int, 4> moves {}; // move id, 0 = 빈 칸
    int abilityId = 0;           // 0 = 고르지 않음(3세대부터)
    int natureId = 0;            // 0 = 고르지 않음(3세대부터)
    int itemId = 0;              // 지닌 물건, 0 = 없음(2세대부터)

    static constexpr int kMemoLength = 40;

    bool isEmpty() const { return pokemonId <= 0; }
    bool operator==(const SquadMember &) const = default;
};

// 세대마다 스쿼드 하나(정주행 하나 = 세대 하나). 세대를 바꾸면 그 세대의 스쿼드로 바뀐다.
struct Squad
{
    QString name;
    QString versionGroup; // 기술 기준 게임("platinum"). 비어 있으면 세대의 대표 게임
    std::array<SquadMember, 6> members;

    int filled() const
    {
        int n = 0;
        for (const SquadMember &member : members)
            n += member.isEmpty() ? 0 : 1;
        return n;
    }
    bool operator==(const Squad &) const = default;
};
} // namespace com::yamada::studio
