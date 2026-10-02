#pragma once

#include "ui/home/homecards.h"

#include <QHash>
#include <QPixmap>
#include <QWidget>

#include <array>

class QVariantAnimation;

namespace com::yamada::studio {
class AppState;
class SpriteCache;

// 홈(인트로)의 세대 선택: 카드 9장을 손에 쥔 패처럼 부채꼴로 펼친다.
//
// 카드 내용(마스코트 · 윗띠 색 · 지방 이름)은 homecards.json, 그림은 그 세대의 정면
// 스프라이트(SpriteCache, 실행 중에 사용자 캐시로 받는다). 세대는 AppState와 바로 잇는다:
// 카드를 누르면 setGeneration, 다른 곳(앱 막대)에서 바꿔도 generationChanged로 따라간다.
//
// 움직임(전부 이 위젯 안에서 끝난다):
//   펼침  — 보일 때마다 가운데에 모인 카드가 부채꼴로 펴진다 (0 → 1, OutCubic)
//   들림  — 마우스가 올라간 카드 · 고른 카드는 부채 바깥쪽으로 밀려 나온다 (카드마다 애니메이션)
//   흔들림 — 카드를 쥐고 끌면 부채 전체가 그 방향으로 돌고, 놓으면 튕기며 돌아온다 (OutBack)
// 카드들은 자식 위젯이 아니라 paintEvent에서 QTransform으로 직접 그린다 — 회전한 카드의
// 클릭 판정은 역변환으로 한다.
class CardFan : public QWidget
{
    Q_OBJECT
public:
    explicit CardFan(AppState *state, QWidget *parent = nullptr);

    void selectNeighbor(int direction); // 키보드 ← → : 이웃 세대로

protected:
    void paintEvent(QPaintEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    struct Layout // 지금 크기에서의 부채꼴 치수 (paint · 판정이 같은 값을 쓰도록 한곳에서 계산)
    {
        qreal cardWidth = 0;
        qreal cardHeight = 0;
        qreal radius = 0; // 회전 중심(pivot) ↔ 카드 윗변
        qreal step = 0;   // 이웃 카드 사이 각도(라디안)
        QPointF pivot;    // 위젯 아래 바깥의 회전 중심
    };
    Layout fanLayout() const;
    QTransform cardTransform(const Layout &fan, int index, bool withLift = true) const;
    QList<int> paintOrder() const; // 들린 카드가 위로 오게 정렬한 카드 번호
    int cardAt(const QPointF &pos, bool withLift) const;
    int hoverCardAt(const QPointF &pos) const; // hover용: 판정이 들림을 따라 흔들리지 않게
    void paintCard(QPainter &painter, const Layout &fan, int index) const;

    QPixmap mascotSprite(int index) const; // 여백을 잘라 낸 기본 그림(처음 쓸 때 캐시)
    void setHovered(int index);
    void animateLift(int index); // 그 카드의 목표 들림(고름 · hover)로 애니메이션을 다시 건다
    qreal liftTarget(int index) const;
    void select(int index);

    AppState *m_state = nullptr; // 소유하지 않는다
    QList<homecards::Card> m_cards;
    SpriteCache *m_fronts = nullptr;
    mutable QHash<int, QPixmap> m_mascots; // 포켓몬 → 여백 잘라 낸 그림

    qreal m_spread = 0; // 0 = 가운데에 모임, 1 = 다 펴짐
    qreal m_swing = 0;  // 끌기로 도는 각도(라디안). 놓으면 0으로 돌아간다
    QVariantAnimation *m_spreadAnimation = nullptr;
    QVariantAnimation *m_swingAnimation = nullptr;
    std::array<qreal, 9> m_lift {};
    std::array<QVariantAnimation *, 9> m_liftAnimations {};

    int m_hovered = -1;
    int m_pressed = -1;
    bool m_dragging = false;
    QPointF m_pressPos;
    qreal m_pressSwing = 0;
};
} // namespace com::yamada::studio
