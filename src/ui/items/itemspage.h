#pragma once

#include <QWidget>

class QButtonGroup;
class QCheckBox;
class QTableView;
class QTimer;

namespace com::yamada::studio {
class AppState;
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
//   ┌ 분류(초록 머리) ┐ ┌ 아이템 대백과 · 진화 · 39개 (빨강 머리) ─────────────┐
//   │ 전체 · 회복 …   │ │ [검색]                                              │
//   │                │ │ ▶ 아이콘 이름 효과 [1 … 9 세대 칸]                    │
//   │ ☐ 4세대에만     │ │                                                     │
//   └────────────────┘ └─────────────────────────────────────────────────────┘
//
// 구조는 DexPage와 같다: Repository → ItemTableModel(원본) → ItemFilterProxy(검색 · 분류 · 세대) →
// QTableView + ItemRowDelegate. 분류 묶음은 itemstyle.json, 세대는 AppState를 따른다.
// 오른쪽 상세 창(세대별 존재 · 효과 · 진화 대상)은 다음 단계.
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
    void selectGroup(const QString &key);
    void updateTitle();

    Repository *m_repository = nullptr;
    AppState *m_state = nullptr;
    ItemTableModel *m_model = nullptr;
    ItemFilterProxy *m_proxy = nullptr;
    SpriteCache *m_sprites = nullptr;
    PanelFrame *m_categoryPanel = nullptr;
    PanelFrame *m_listPanel = nullptr;
    QButtonGroup *m_groups = nullptr;
    QCheckBox *m_onlyThisGeneration = nullptr;
    SearchField *m_search = nullptr;
    QTableView *m_table = nullptr;
    ItemHeaderView *m_header = nullptr;
    ItemRowDelegate *m_delegate = nullptr;
    QTimer *m_searchDelay = nullptr;
    QString m_groupKey;
    bool m_loaded = false;
};
} // namespace com::yamada::studio
