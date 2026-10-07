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
// - 세대 · 게임: AppState의 세대 · 게임(버전)을 따라간다. 버전마다 스쿼드가 따로 있다
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
    QString version() const { return m_version; } // 지금 게임(버전 "soulsilver" — 비어 있지 않다)
    QString versionGroup() const { return m_versionGroup; } // 그 버전의 묶음(기술 · 사전의 기준)
    const QList<GameInfo> &games() const { return m_games; }
    const QList<Nature> &natures();
    const QList<ItemRow> &items(); // 그 세대의 아이템(지닌 물건 선택지)

    // 비전셔틀(7번째 멤버) — 본편 6자리 분석에는 들어가지 않는다. 기술 칸은 비전머신 기술만 담는다
    static constexpr int kShuttleSlot = 6; // 포켓몬 선택 창이 본편 슬롯과 구분하는 번호
    const SquadMember &shuttle() const { return m_squad.shuttle; }
    const PokemonDetail &shuttleDetail() const { return m_shuttleDetail; }
    const std::array<std::optional<SlotMove>, 4> &shuttleMoves() const { return m_shuttleMoves; }

    void setName(const QString &name);
    // 게임(버전)을 바꾼다 = 그 버전의 스쿼드로 바꾼다(버전마다 스쿼드가 따로 저장된다). 앱 전체의
    // 게임(AppState)도 같이 바뀐다
    void setVersion(const QString &version);
    void setPokemon(int slot, int pokemonId); // 기술 · 메모 · 특성 · 물건은 비운다
    void clearSlot(int slot);
    void swapSlots(int a, int b);
    // from 자리를 빼서 to 자리에 끼운다(사이의 자리는 한 칸씩 당겨지거나 밀린다). 카드 끌기
    void moveSlot(int from, int to);
    void setMove(int slot, int index, int moveId); // 0 = 비우기
    void setMemo(int slot, const QString &memo);
    // 스쿼드를 통째로 바꾼다(불러오기) — 지금 세대 · 버전의 스쿼드로 저장된다
    void replaceSquad(const Squad &squad);
    // 되돌리기 — 세션이 커밋한 편집마다 한 단계(기록은 reload에서 비워진다: 스쿼드마다 따로)
    bool canUndo() const { return !m_undoStack.isEmpty(); }
    bool canRedo() const { return !m_redoStack.isEmpty(); }
    void undo();
    void redo();
    void clearAll(); // 멤버 6자리와 비전셔틀을 모두 비운다(이름은 남는다) — 되돌릴 수 있다
    void setShuttlePokemon(int pokemonId);      // 기술은 비운다
    void setShuttleMove(int index, int moveId); // 0 = 비우기
    void clearShuttle();
    void setAbility(int slot, int abilityId);
    void setNature(int slot, int natureId);
    void setItem(int slot, int itemId);

    QString defaultName() const; // 이름을 비워 두면 보이는 이름("소울실버 스쿼드")

signals:
    void changed();

public slots:
    void reload(); // 세대가 바뀌었다(또는 DB가 새로 생겼다) → 그 세대 스쿼드를 다시 읽는다

private:
    void resolve(int slot); // 포켓몬 · 기술을 그 세대 값으로
    void resolveAll(); // 여섯 자리 + 셔틀 전부(성격 · 물건 목록도 미리 읽는다)
    void resolveShuttle();
    // resolve의 몸통: member의 포켓몬 · 기술을 그 세대 값으로 detail · moves에 풀어 둔다
    void resolveMember(const SquadMember &member, PokemonDetail &detail,
                       std::array<std::optional<SlotMove>, 4> &moves);
    void commit(); // 저장소에 넘기고 분석을 다시 한다 → changed()
    void analyze();

    Repository *m_repository = nullptr;
    SquadStore *m_store = nullptr;
    AppState *m_state = nullptr;
    int m_generation = 0;
    QString m_version;
    QString m_versionGroup;
    bool m_seeding = false; // reload가 AppState에 처음 게임을 넣는 중
    Squad m_squad;
    std::array<PokemonDetail, kSquadSize> m_details;
    std::array<std::array<std::optional<SlotMove>, 4>, kSquadSize> m_moves;
    PokemonDetail m_shuttleDetail;
    std::array<std::optional<SlotMove>, 4> m_shuttleMoves;
    TypeChart m_chart;
    QList<GameInfo> m_games;
    QList<Nature> m_natures;
    QList<ItemRow> m_items;
    bool m_itemsLoaded = false;
    SquadAnalysis m_analysis;
    // 되돌리기 기록: commit이 "마지막으로 커밋한 스쿼드"와 달라질 때마다 직전 상태를 쌓는다
    Squad m_committed;
    QList<Squad> m_undoStack;
    QList<Squad> m_redoStack;
};
} // namespace com::yamada::studio
