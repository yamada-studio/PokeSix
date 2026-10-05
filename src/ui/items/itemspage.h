#pragma once

#include <QWidget>

class QButtonGroup;
class QModelIndex;
class QTableView;
class QTimer;

namespace com::yamada::studio {
class AppState;
class GameSelector;
class ItemDetailPane;
class ItemFilterProxy;
class ItemHeaderView;
class ItemRowDelegate;
class ItemTableModel;
class PanelFrame;
class Repository;
class SearchField;
class SpriteCache;

// 아이템 대백과 화면 (02 SCR-03 · 13_items_1440, 로드맵 E3). 이번 단계는 분류 창 + 목록 창.
//
//   ┌ 분류(초록 머리) ┐ ┌ 아이템 대백과 · 진화 · 39개 (빨강 머리) ──┐ ┌ 상세(파랑 머리) ┐
//   │ 전체 · 회복 …   │ │ [검색]                                   │ │ 아이콘 · 이름    │
//   │                │ │ ▶ 아이콘 이름 효과 가격                    │ │ 세대 · 효과 · 진화│
//   └────────────────┘ └──────────────────────────────────────────┘ └────────────────┘
//
// 구조는 DexPage와 같다: Repository → ItemTableModel(원본) → ItemFilterProxy(검색 · 분류 · 세대) →
// QTableView + ItemRowDelegate. 분류 묶음은 itemstyle.json, 세대는 AppState를 따른다 — 목록에는 늘
// 지금 세대에 있는 아이템만 나오고, 세대를 바꾸면 다시 채운다(도감과 같다).
// 줄을 고르면(클릭 · ↑↓) 오른쪽 상세 창이 바뀐다(세대별 존재 · 효과 · 진화 대상 · 입수처).
// 목록 머리 띠의 게임 칩으로 같은 세대 안에서도 게임을 고른다: 기술머신에 담긴 기술은 그 게임의
// 표(PokéAPI machines)를, 아이템 존재 · 입수처는 그 게임의 입수 사전(acquisition/<게임>.json)을
// 따른다. 사전이 없는 게임은 세대 기준 목록 그대로다.
class ItemsPage : public QWidget
{
    Q_OBJECT
public:
    // repository · state는 소유하지 않는다(MainWindow가 소유).
    ItemsPage(Repository *repository, AppState *state, QWidget *parent = nullptr);

protected:
    void showEvent(QShowEvent *event) override; // 처음 보일 때 읽는다(lazy)

private:
    QWidget *buildCategoryBody();
    QWidget *buildListBody();
    void load();
    void onGenerationChanged();
    void applyLanguage(); // AppState 언어 → 모델 · delegate · 분류 이름 · 제목
    void selectGroup(const QString &key);
    void selectGame(const QString &versionGroup); // 게임 칩 → 그 게임의 기술머신 · 아이템으로
    void showDetail(const QModelIndex &proxyIndex);
    void updateTitle();

    Repository *m_repository = nullptr;
    AppState *m_state = nullptr;
    ItemTableModel *m_model = nullptr;
    ItemFilterProxy *m_proxy = nullptr;
    SpriteCache *m_sprites = nullptr;
    PanelFrame *m_categoryPanel = nullptr;
    PanelFrame *m_listPanel = nullptr;
    QButtonGroup *m_groups = nullptr;
    SearchField *m_search = nullptr;
    QTableView *m_table = nullptr;
    ItemHeaderView *m_header = nullptr;
    ItemRowDelegate *m_delegate = nullptr;
    QTimer *m_searchDelay = nullptr;
    ItemDetailPane *m_detail = nullptr;
    GameSelector *m_games = nullptr; // 게임 칩 [DP][Pt][HGSS] — 같은 세대라도 게임마다 다르다
    QString m_versionGroup; // 지금 게임(세대가 바뀌면 그 세대의 대표 게임으로)
    PanelFrame *m_detailPanel = nullptr;
    QString m_groupKey;
    bool m_loaded = false;
};
} // namespace com::yamada::studio
