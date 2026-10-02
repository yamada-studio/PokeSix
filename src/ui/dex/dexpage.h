#pragma once

#include "data/repository/repository.h"

#include <QWidget>

class QModelIndex;
class QStackedWidget;
class QTableView;
class QTimer;

namespace com::yamada::studio {
class PanelFrame;
class Repository;
class SearchField;
class SpriteCache;
class AppState;
class DexDetailPage;
class DexFilterPanel;
class DexPreview;
class DexRowDelegate;
class DexSelector;
class SpeciesFilterProxy;
class SpeciesTableModel;

// 도감 화면 (로드맵 D2 · E1). 목록 줄: [필터 | 목록 | 미리 보기] 세 창. 줄을 고르면(클릭 · ↑↓)
// 미리 보기가 바뀌고, 더블클릭 · Enter · [자세히 보기]가 전체 화면 상세로 간다.
//
//   Repository ──▶ SpeciesTableModel ──▶ SpeciesFilterProxy ──▶ QTableView (+ DexRowDelegate)
//      (DB 조회)        (표 모델)         (검색 · 필터 · 정렬)        (그리기)
//
// 처음 보일 때(showEvent) 한 번 읽는다. 앱을 켤 때 모든 화면을 미리 읽지 않고, 첫 실행에는 DB가
// 아직 없을 수도 있기 때문이다(설계서 03 §2 "페이지는 처음 열 때 생성(lazy)").
class DexPage : public QWidget
{
    Q_OBJECT
public:
    // repository · state는 소유하지 않는다(MainWindow가 소유). 세대는 state를 따라간다.
    explicit DexPage(Repository *repository, AppState *state, QWidget *parent = nullptr);

    void showList(); // 상세 → 목록
    // 다른 화면(스쿼드)에서 바로 그 포켓몬의 상세로. versionGroup = 그 화면이 고른 게임 — 상세의
    // 기준 게임이 되고, 목록도 그 게임의 도감으로 바꿔 둔다(← 목록으로 돌아와도 같은 게임)
    void openPokemon(int pokemonId, const QString &versionGroup);

protected:
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override; // 최대 폭: 넓으면 좌우 여백을 늘린다
    bool eventFilter(QObject *watched, QEvent *event) override; // 표 viewport의 크기 변화 → 칸 폭

private:
    void load();
    void onGenerationChanged();
    void applyLanguage();
    void
    openDetail(const QModelIndex &proxyIndex); // 목록의 줄 → 상세 화면             // AppState 언어
                                               // → 모델 · delegate · 도감 버튼 · 타입 칸 폭
    void showDex(int pokedexId); // 도감 선택 버튼 → 그 도감의 목록(DexSelector::kNational = 전국)
    void showPreview(const QModelIndex &proxyIndex); // 고른 줄 → 오른쪽 미리 보기
    void applyFilters();                             // 필터 창 → 프록시
    void updateTitle();
    void layoutColumns(); // 비율 칸(이름 · 종족값 · 합계)에 남는 폭을 가중치대로 나눈다

    Repository *m_repository = nullptr; // 소유하지 않는다(MainWindow가 소유)
    AppState *m_state = nullptr;        // 〃
    SpeciesTableModel *m_model = nullptr;
    SpeciesFilterProxy *m_proxy = nullptr;
    PanelFrame *m_panel = nullptr;
    QStackedWidget *m_views = nullptr; // [목록 창 │ 상세]
    DexDetailPage *m_detail = nullptr;
    SearchField *m_search = nullptr;
    DexFilterPanel *m_filter = nullptr;
    DexPreview *m_preview = nullptr;
    TypeChart m_chart; // 지금 세대의 상성표(미리 보기의 약점 계산)
    QTableView *m_table = nullptr;
    SpriteCache *m_sprites = nullptr;  // 이름 옆 아이콘 파일 캐시
    DexSelector *m_selector = nullptr; // 머리 띠 오른쪽 도감 선택
    DexRowDelegate *m_delegate = nullptr;
    QTimer *m_searchDelay = nullptr;
    bool m_loaded = false;
    int m_fixedWidth = 0; // 고정 칸(▶ · 번호 · 아이콘 · 타입) 폭의 합
};
} // namespace com::yamada::studio
