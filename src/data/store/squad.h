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

// 게임(시리즈)마다 스쿼드 하나 — 세대 → 게임 묶음(version group) → 스쿼드. 같은 세대라도 게임마다
// 도감 · 나오는 포켓몬 · 기술이 달라 정주행이 따로다(4세대: DP · Pt · HGSS).
struct Squad
{
    QString name;
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
