#pragma once

#include <QWidget>

class QTableView;
class QTimer;

namespace com::yamada::studio {
class PanelFrame;
class Repository;
class SearchField;
class SpriteCache;
class SpeciesFilterProxy;
class SpeciesTableModel;

// 도감 화면 — 지금은 목록 창 하나(로드맵 D2). 필터 창 · 상세 창은 Phase E1에서 붙인다.
//
//   Repository ──▶ SpeciesTableModel ──▶ SpeciesFilterProxy ──▶ QTableView (+ DexRowDelegate)
//      (DB 조회)        (표 모델)            (검색 · 정렬)            (그리기)
//
// 처음 보일 때(showEvent) 한 번 읽는다. 앱을 켤 때 모든 화면을 미리 읽지 않고, 첫 실행에는 DB가
// 아직 없을 수도 있기 때문이다(설계서 03 §2 "페이지는 처음 열 때 생성(lazy)").
class DexPage : public QWidget
{
    Q_OBJECT
public:
    explicit DexPage(Repository *repository, QWidget *parent = nullptr);

protected:
    void showEvent(QShowEvent *event) override;

private:
    void load();
    void updateTitle();

    Repository *m_repository = nullptr; // 소유하지 않는다(MainWindow가 소유)
    SpeciesTableModel *m_model = nullptr;
    SpeciesFilterProxy *m_proxy = nullptr;
    PanelFrame *m_panel = nullptr;
    SearchField *m_search = nullptr;
    QTableView *m_table = nullptr;
    SpriteCache *m_sprites = nullptr; // 이름 옆 아이콘 파일 캐시
    QTimer *m_searchDelay = nullptr;
    bool m_loaded = false;
};
} // namespace com::yamada::studio
