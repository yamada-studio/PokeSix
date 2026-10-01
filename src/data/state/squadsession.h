#pragma once

#include "core/analysis/squadanalyzer.h"
#include "data/repository/repository.h"
#include "data/store/squad.h"

#include <QHash>
#include <QObject>

#include <array>
#include <optional>

namespace com::yamada::studio {
class AppState;
class SquadStore;

// 지금 세대의 스쿼드 하나를 편집하는 창구(D3). 화면(SquadPage)은 이 객체만 보고, 값이 바뀌면
// changed()를 받아 다시 그린다 — ROS 2로 치면 상태를 들고 있는 노드가 바뀔 때마다 토픽을 내는 것.
//
// - 저장: 바꿀 때마다 SquadStore에 넘긴다(저장소가 디바운스해서 파일에 쓴다)
// - 세대: AppState의 세대를 따라간다. 세대마다 스쿼드가 따로 있다
// - 풀어 둔 값: 포켓몬 · 기술 · 특성 · 물건을 그 세대(게임) 값으로 DB에서 읽어 둔다
// - 분석: 바뀔 때마다 core SquadAnalyzer로 처음부터 다시 계산한다(6 × 18, 동기)
class SquadSession : public QObject
{
    Q_OBJECT
public:
    SquadSession(Repository *repository, SquadStore *store, AppState *state,
                 QObject *parent = nullptr);

    // 기술 칸 하나: 고른 기술의 그 세대 값. learnable = 그 게임에서 이 포켓몬이 배울 수 있다
    struct SlotMove
    {
        MoveEntry move;
        bool learnable = false;
    };
    // 기술 선택 창의 한 줄: 배우는 방법을 모두 모은다
    struct LearnableMove
    {
        MoveEntry move;
        int level = -1;  // 레벨업(−1 = 레벨업으로는 못 배움, 1 = 처음부터)
        QString machine; // "TM26" · "HM03"(기술머신으로 못 배우면 빈 칸)
        bool tutor = false;
        bool egg = false;
    };

    int generation() const { return m_generation; }
    const Squad &squad() const { return m_squad; }
    const SquadMember &member(int slot) const { return m_squad.members[std::size_t(slot)]; }
    const PokemonDetail &detail(int slot) const { return m_details[std::size_t(slot)]; }
    const std::array<std::optional<SlotMove>, 4> &slotMoves(int slot) const
    {
        return m_moves[std::size_t(slot)];
    }
    QList<LearnableMove> learnableMoves(int slot) const;
    const AbilityEntry *ability(int slot) const; // 고른 특성(그 세대에 없으면 nullptr)
    const Nature *nature(int slot) const;
    const ItemRow *item(int slot) const;
    const SquadAnalysis &analysis() const { return m_analysis; }
    const TypeChart &chart() const { return m_chart; }
    QString versionGroup() const { return m_versionGroup; } // 지금 게임(비어 있지 않다)
    const QList<GameInfo> &games() const { return m_games; }
    const QList<Nature> &natures();
    const QList<ItemRow> &items(); // 그 세대의 아이템(지닌 물건 선택지)

    void setName(const QString &name);
    // 게임을 바꾼다 = 그 게임의 스쿼드로 바꾼다(게임마다 스쿼드가 따로 저장된다)
    void setVersionGroup(const QString &versionGroup);
    void setPokemon(int slot, int pokemonId); // 기술 · 메모 · 특성 · 물건은 비운다
    void clearSlot(int slot);
    void swapSlots(int a, int b);
    // from 자리를 빼서 to 자리에 끼운다(사이의 자리는 한 칸씩 당겨지거나 밀린다). 카드 끌기
    void moveSlot(int from, int to);
    void setMove(int slot, int index, int moveId); // 0 = 비우기
    void setMemo(int slot, const QString &memo);
    void setAbility(int slot, int abilityId);
    void setNature(int slot, int natureId);
    void setItem(int slot, int itemId);

    QString defaultName() const; // 이름을 비워 두면 보이는 이름("4세대 스쿼드")

signals:
    void changed();

public slots:
    void reload(); // 세대가 바뀌었다(또는 DB가 새로 생겼다) → 그 세대 스쿼드를 다시 읽는다

private:
    void resolve(int slot); // 포켓몬 · 기술을 그 세대 값으로
    void commit();          // 저장소에 넘기고 분석을 다시 한다 → changed()
    void analyze();

    Repository *m_repository = nullptr;
    SquadStore *m_store = nullptr;
    AppState *m_state = nullptr;
    int m_generation = 0;
    QString m_versionGroup;
    Squad m_squad;
    std::array<PokemonDetail, kSquadSize> m_details;
    std::array<std::array<std::optional<SlotMove>, 4>, kSquadSize> m_moves;
    TypeChart m_chart;
    QList<GameInfo> m_games;
    QList<Nature> m_natures;
    QList<ItemRow> m_items;
    bool m_itemsLoaded = false;
    SquadAnalysis m_analysis;
};
} // namespace com::yamada::studio
