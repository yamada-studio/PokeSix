#pragma once

#include <QList>
#include <QWidget>

class QBoxLayout;
class QGridLayout;
class QLabel;
class QLineEdit;
class QScrollArea;

namespace com::yamada::studio {
class AppState;
class DropdownButton;
class HeatmapView;
class PanelFrame;
class ProblemList;
class Repository;
class SlotCard;
class SplitBar;
class SpriteCache;
class SquadPips;
class SquadSession;
class SquadStore;

// SixSquad 편집 화면(02 SCR-04, E2). 지금 세대의 스쿼드 하나를 편집한다.
//
//   [이름 ✎] [4세대 규칙] [기라티나 (Pt) ▾] ■■■■■□ 5 / 6 ········· ✓ 자동 저장됨
//   ┌ 슬롯 카드 6 ┐ ┌ 실시간 분석 ─────────────── ⚠ 문제 3 ┐
//   │ 01 │ 02     │ │ 문제 목록 · 방어 상성 히트맵 · 물리/특수  │
//   │ …          │ └──────────────────────────────────────┘
// 넓으면(≥ kWideWidth) 카드 2열 | 분석 창, 좁으면 카드 3열 위 · 분석 창 아래(세로 스크롤).
//
// 고르는 창(포켓몬 · 기술 · 물건)은 모두 지금 세대(기술은 고른 게임)에 있는 것만 보여 준다.
class SquadPage : public QWidget
{
    Q_OBJECT
public:
    SquadPage(Repository *repository, AppState *state, QWidget *parent = nullptr);

    static constexpr int kWideWidth = 1240;

signals:
    void dexRequested(int pokemonId); // 슬롯 메뉴 "도감에서 보기"

protected:
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QWidget *buildTopBar();
    QWidget *buildAnalysis();
    void placeCards(bool wide);
    void refresh(); // 세션 · 언어가 바뀌었다 → 전부 다시
    void refreshAnalysis();

    void selectSlot(int slot);
    void showSlotMenu(int slot, const QPoint &globalPos);
    void showGameMenu();
    void pickPokemon(int slot);
    void pickMove(int slot, int index);
    void pickAbility(int slot, const QPoint &globalPos);
    void pickNature(int slot, const QPoint &globalPos);
    void pickItem(int slot);
    void onProblemHovered(int row);
    void onProblemClicked(int row);

    QString typeName(const QString &key) const;
    QString suggestion() const; // 빈 자리 제안 문구

    Repository *m_repository = nullptr;
    AppState *m_state = nullptr;
    SquadStore *m_store = nullptr;
    SquadSession *m_session = nullptr;
    SpriteCache *m_pokemonIcons = nullptr;
    SpriteCache *m_itemIcons = nullptr;

    QLineEdit *m_name = nullptr;
    QLabel *m_rule = nullptr;
    DropdownButton *m_game = nullptr;
    SquadPips *m_pips = nullptr;
    QLabel *m_count = nullptr;
    QLabel *m_saveStatus = nullptr;

    QScrollArea *m_scroll = nullptr;
    QBoxLayout *m_columns = nullptr;
    QWidget *m_cardArea = nullptr;
    QGridLayout *m_grid = nullptr;
    QList<SlotCard *> m_cards;
    bool m_wide = false;

    PanelFrame *m_analysis = nullptr;
    QLabel *m_problemPill = nullptr;
    QLabel *m_emptyAnalysis = nullptr;
    QWidget *m_analysisBody = nullptr;
    ProblemList *m_problems = nullptr;
    HeatmapView *m_heatmap = nullptr;
    QLabel *m_splitRule = nullptr;
    QLabel *m_moveCount = nullptr;
    SplitBar *m_split = nullptr;

    int m_selected = -1;
};
} // namespace com::yamada::studio
