#pragma once

#include "data/text/localizedtext.h"

#include <QWidget>

class QLineEdit;

namespace com::yamada::studio {
class SpriteCache;
class SquadSession;

// 스쿼드 한 자리(디자인 시트 §5-6 SquadSlotCard / EmptySlotCard). 내용은 SquadSession에서 바로
// 읽어 그린다 — 카드는 "몇 번 자리"만 안다. 누를 수 있는 곳은 시그널로 알리고, 창 · 메뉴를 여는
// 건 SquadPage가 한다.
//
//   ┌ 01 [아이콘] 한카리아스 ·············· ⋯ ┐ ← 첫 타입 색 머리(누르면 선택)
//   │ [드래곤][땅]                              │
//   │ [역할 메모________________________]       │ ← QLineEdit(자식 위젯)
//   │ [특성 모래숨기 ▾][성격 고집 ▾][물건 ▾]    │ ← 그 세대에 있는 것만
//   │ [■ 지진    물][■ 역린     물]            │ ← 기술 칸(누르면 기술 선택)
//   │ [■ 스톤에지 물][■ 칼춤     변]           │
//   │ 약점 [얼×4][드]                           │
//   └───────────────────────────────────────────┘
class SlotCard : public QWidget
{
    Q_OBJECT
public:
    SlotCard(int slot, SquadSession *session, SpriteCache *pokemonIcons, SpriteCache *itemIcons,
             QWidget *parent = nullptr);

    static constexpr int kHeight = 258; // 내용이 꼭 맞는 기준 높이
    static constexpr int kMinimumHeight = 234; // 줄 사이 숨만 줄인 최소(창이 낮으면 여기까지)
    static constexpr int kMaximumHeight = 292; // 줄 사이가 넉넉해지는 최대(창이 높으면 여기까지)
    static constexpr int kMinimumWidth = 262;

    // 카드 높이를 [kMinimumHeight, kMaximumHeight]로 맞춘다 — 창 높이에 따라 6장이 아래 빈 공간
    // 없이 딱 들어가게(SquadPage가 계산한다). 차이는 줄 사이 간격에서만 더하고 뺀다(areas()).
    void setCardHeight(int height);

    void refresh(Language language); // 세션 값이 바뀌었다
    void setSelected(bool selected);
    void setAlert(bool alert); // 문제 행에 마우스가 올라왔다 → 빨강 테
    void setSuggestion(const QString &text); // 빈 자리의 제안 문구
    void setWarning(const QString &text); // 같은 포켓몬 · 스타팅 둘 → 머리에 "!" + 툴팁

    QSize sizeHint() const override { return {300, height()}; }
    QSize minimumSizeHint() const override { return {kMinimumWidth, height()}; }

signals:
    void selectRequested(int slot);
    void addRequested(int slot);
    void menuRequested(int slot, const QPoint &globalPos);
    void moveRequested(int slot, int index);
    void abilityRequested(int slot, const QPoint &globalPos);
    void natureRequested(int slot, const QPoint &globalPos);
    void itemRequested(int slot, const QPoint &globalPos);
    // 머리를 잡고 끈다(누른 채 조금 움직이면 시작). 자리 계산 · 애니메이션은 SquadPage가 한다
    void dragStarted(int slot, const QPoint &globalPos);
    void dragMoved(int slot, const QPoint &globalPos);
    void dragFinished(int slot);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    bool event(QEvent *event) override; // 기술 칸 툴팁

private:
    enum class Hit { None, Header, Menu, Add, Ability, Nature, Item, Move0, Move1, Move2, Move3 };
    struct Geometry
    {
        QRect card;   // 링(선택 · 경고) 자리를 뺀 카드
        QRect header; // 머리 띠
        QRect menu;   // ⋯
        QRect warn;   // ! (경고가 있을 때)
        QRect types;
        QRect memo;
        QRect traits[3]; // 특성 · 성격 · 물건
        QRect moves[4];
        QRect weak;
        QRect add; // 빈 자리의 [+ 개체 추가]
    };
    Geometry areas() const;
    Hit hitAt(const QPoint &pos) const;
    bool isEmptySlot() const;
    void paintFilled(QPainter &painter, const Geometry &g);
    void paintEmpty(QPainter &painter, const Geometry &g);
    void paintTrait(QPainter &painter, const QRect &rect, const QString &label,
                    const QString &value, bool hot, const QString &itemIcon = QString());
    void paintMove(QPainter &painter, const QRect &rect, int index, bool hot);

    int m_slot = 0;
    SquadSession *m_session = nullptr;
    SpriteCache *m_pokemonIcons = nullptr;
    SpriteCache *m_itemIcons = nullptr;
    QLineEdit *m_memo = nullptr;
    Language m_language = Language::Korean;
    bool m_selected = false;
    bool m_alert = false;
    QString m_suggestion;
    QString m_warning;
    Hit m_hot = Hit::None;
    bool m_headerPressed = false; // 머리를 눌렀다(놓으면 선택, 끌면 옮기기)
    bool m_dragging = false;
    QPoint m_pressPos;
};
} // namespace com::yamada::studio
