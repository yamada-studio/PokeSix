#pragma once

#include "ui/squad/problemlist.h" // ProblemList::Row 를 멤버로 둔다

#include <QHash>
#include <QList>
#include <QWidget>

class QBoxLayout;
class QGridLayout;
class QLabel;
class QLineEdit;
class QPixmap;
class QPropertyAnimation;
class QPushButton;
class QScrollArea;

namespace com::yamada::studio {
class AppState;
class GameSelector;
class HeatmapView;
class PanelFrame;
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
//   ┌ 슬롯 카드 6 ┐ ┌ 스쿼드 분석 ─────────────── ⚠ 문제 3 ┐
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

    void reloadData(); // 게임 데이터 DB가 새로 생겼다(데이터 받기 끝) → 스쿼드를 다시 읽는다

signals:

protected:
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    // 스크롤 viewport의 크기 변화를 엿본다 — SquadPage::resizeEvent 시점에는 viewport가 아직
    // 옛 크기라서, 카드 높이를 거기서 재면 분석 창과 바닥이 어긋난다
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QWidget *buildTopBar();
    QWidget *buildAnalysis();
    void placeCards(bool wide);
    void fitCardHeights(); // 창 높이에 맞춰 카드 6장이 스크롤 없이 들어가게
    // 좁은 배치에서는 분석 창 최소 높이 = 내용 높이(안쪽 스크롤 없이 페이지가 스크롤),
    // 넓은 배치에서는 열 높이를 따른다(넘치면 분석 안에서만 스크롤)
    void syncAnalysisHeight();
    void refresh(); // 세션 · 언어가 바뀌었다 → 전부 다시
    void refreshAnalysis();
    void refreshResources(); // 분석 창의 "리소스 투자" 칸

    void selectSlot(int slot);
    void showMemberDetail(int slot); // 카드 머리의 [+] — 도감 상세를 모달로
    void pickPokemon(int slot);
    void pickMove(int slot, int index);
    void pickAbility(int slot, const QPoint &globalPos);
    void pickNature(int slot, const QPoint &globalPos);
    void pickItem(int slot);
    // 카드 끌기: 끄는 동안 레이아웃을 멈추고 카드를 직접 옮긴다(놓으면 순서를 저장하고 되돌린다)
    void onDragStarted(int slot, const QPoint &globalPos);
    void onDragMoved(int slot, const QPoint &globalPos);
    void onDragFinished(int slot);
    void finishDrag(int from, int to);
    void slideTo(QWidget *card, const QPoint &target, int durationMs);
    void onProblemHovered(int row);
    void onProblemClicked(int row);
    // 문제 목록은 자기 칸 안에서만 스크롤한다(히트맵 · 분포가 항상 보이게). 넓은 배치에서는 남는
    // 세로를 채우되 내용보다 커지지 않고, 좁은 배치에서는 kProblemVisibleRows 줄까지만 편다
    void syncProblemHeight();
    void showProblemDialog();    // "크게 보기" — 문제 전체를 큰 창에서
    void showShuttleDialog();    // 상단 막대의 [비전셔틀] — 7번째 멤버 창
    void refreshShuttleButton(); // [비전셔틀 · 잠만보] 글자 + 박스 아이콘
    // 상단 막대 오른쪽의 공유 묶음: 파일 내보내기 · 불러오기, 이미지 복사 · 저장
    void exportSquad();
    void importSquad();
    QPixmap squadImage(); // 카드 6장 + 분석 창을 제목 띠와 함께 한 장으로
    void copyImage();
    void saveImage();
    void flashStatus(const QString &text); // "✓ 이미지 복사됨" 같은 잠깐 알림(2.5초 뒤 복원)

    QString typeName(const QString &key) const;
    QString suggestion() const; // 빈 자리 제안 문구
    // slot 자리에 speciesId를 둘 때의 경고(같은 포켓몬이 이미 있다 · 스타팅이 이미 있다). 없으면 빈
    // 칸
    QString warningFor(int slot, int speciesId) const;

    Repository *m_repository = nullptr;
    AppState *m_state = nullptr;
    SquadStore *m_store = nullptr;
    SquadSession *m_session = nullptr;
    SpriteCache *m_pokemonIcons = nullptr;
    SpriteCache *m_itemIcons = nullptr;

    QLineEdit *m_name = nullptr;

    QString m_nameSquad; // 이름 칸이 보여 주는 스쿼드("4/soulsilver")
    QLabel *m_rule = nullptr;
    GameSelector *m_game = nullptr; // 게임 칩 [DP][Pt][HGSS] — 게임마다 스쿼드가 따로
    SquadPips *m_pips = nullptr;
    QLabel *m_count = nullptr;
    QPushButton *m_shuttleButton = nullptr; // [비전셔틀 · 잠만보] — 누르면 셔틀 창
    QLabel *m_saveStatus = nullptr;
    QWidget *m_topBar = nullptr; // 오른쪽 끝을 분석 창과 맞추려고 스크롤바 폭만큼 들여 쓴다

    QScrollArea *m_scroll = nullptr;
    QBoxLayout *m_columns = nullptr;
    QWidget *m_cardArea = nullptr;
    QGridLayout *m_grid = nullptr;
    QList<SlotCard *> m_cards;
    bool m_wide = false;

    PanelFrame *m_analysis = nullptr;
    QScrollArea *m_analysisScroll = nullptr; // 분석 몸통(넓은 배치에서 안쪽 스크롤)
    QLabel *m_problemPill = nullptr;
    QLabel *m_emptyAnalysis = nullptr;
    QWidget *m_analysisBody = nullptr;
    ProblemList *m_problems = nullptr;
    QScrollArea *m_problemScroll = nullptr; // 문제 목록만의 스크롤
    QLabel *m_problemCount = nullptr;       // 문제 칸 제목 옆 "13개"
    ProblemList *m_dialogProblems = nullptr; // 크게 보기 창이 열려 있으면 그 목록(같이 갱신)
    QList<ProblemList::Row> m_problemRows;
    QString m_problemEmptyText;
    HeatmapView *m_heatmap = nullptr;
    QLabel *m_moveCount = nullptr;
    SplitBar *m_split = nullptr;
    QWidget *m_resourceBox = nullptr; // 리소스 투자 칸(제목 + 줄들) — 집계가 비면 통째로 숨긴다
    QLabel *m_resourceTotal = nullptr; // 제목 오른쪽 합계("하트비늘 2개 · 160BP")
    QLabel *m_resources = nullptr;     // 기술머신 · 가르침 줄들

    int m_selected = -1;
    QHash<QString, int> m_pickerDex;
    mutable QHash<int, int> m_evolvesFrom; // 종 → 진화 전 종(경고용, 처음 쓸 때 읽는다)

    bool m_dragging = false;
    QList<QRect> m_cells; // 자리(위치 번호)마다 카드 칸 — 끌기를 시작할 때 레이아웃에서 잰다
    QList<int> m_order;  // 위치 → 카드(= 원래 슬롯 번호). 끄는 동안 바뀐다
    QPoint m_dragOffset; // 카드 왼쪽 위 ↔ 마우스
    QHash<QWidget *, class QPropertyAnimation *>
            m_slides; // 게임(버전 그룹) → 포켓몬 선택 창에서 마지막에 고른 도감
};
} // namespace com::yamada::studio
