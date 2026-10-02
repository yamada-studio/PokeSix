#pragma once

#include "ui/home/homecards.h"

#include <QHash>
#include <QPixmap>
#include <QWidget>

class QVariantAnimation;

namespace com::yamada::studio {
class AppState;
class SpriteCache;

// 홈(인트로)의 세대 선택: 카드 9장이 둘러 붙은 원통(배럴)을 측면에서 본다.
// 정면 카드가 곧 선택이다 — 옆 카드들은 곡면을 따라 좁아지고(가로 압축 = cos), 뒤로 갈수록
// 작아지고 흐려지다가 모서리(90°)를 넘으면 사라진다.
//
//   돌리기 — 카드를 쥐고 끌면 배럴이 돌고, 놓으면 가장 가까운 카드가 정면에 걸리며 선택된다.
//            옆 카드를 누르면 그 카드가 정면으로 돌아온다. 휠 · ← → 도 한 칸씩 돌린다.
//   등장   — 보일 때마다 배럴이 반 바퀴쯤 돌면서 자리를 잡는다.
//   정면   — 노란 테 + 띠의 ▶ (앱의 선택 문법). 다른 곳(앱 막대)에서 세대를 바꾸면 따라 돈다.
//
// 카드는 자식 위젯이 아니라 paintEvent에서 QTransform으로 직접 그린다. 내용(마스코트 · 띠 색 ·
// 지방 이름)은 homecards.json, 그림은 기본 정면 스프라이트(여백을 잘라 Scale2x로 키워 캐시).
class CardBarrel : public QWidget
{
    Q_OBJECT
public:
    explicit CardBarrel(AppState *state, QWidget *parent = nullptr);

    void selectNeighbor(int direction); // 키보드 ← → : 한 칸 돌리기

protected:
    void paintEvent(QPaintEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    struct Layout // 지금 크기에서의 배럴 치수 (paint · 판정이 같은 값을 쓰도록 한곳에서 계산)
    {
        qreal cardWidth = 0;
        qreal cardHeight = 0;
        qreal radius = 0; // 원통 반지름(px) — 옆 카드 가운데의 가로 거리 = sin(각) × radius
        qreal step = 0; // 이웃 카드 사이 각도 = 2π / 9
    };
    Layout barrelLayout() const;
    // index 카드의 배럴 각도(0 = 정면). m_angle이 돌면 모두 같이 돈다.
    qreal angleOf(const Layout &barrel, int index) const;
    QTransform cardTransform(const Layout &barrel, int index) const;
    QList<int> paintOrder() const; // 뒤(깊은 쪽)부터 — 정면이 맨 위에 그려진다
    int cardAt(const QPointF &pos) const;
    void paintCard(QPainter &painter, const Layout &barrel, int index) const;

    QPixmap mascotSprite(int index) const; // 여백을 잘라 Scale2x로 키운 그림(처음 쓸 때 캐시)
    int frontIndex() const;                // 지금 각도에서 정면에 가장 가까운 카드
    void rotateTo(int index); // 그 카드가 정면에 오게 돌리고(최단 방향) 선택한다
    void snapToNearest();     // 끌다 놓았다 → 가장 가까운 카드로

    AppState *m_state = nullptr; // 소유하지 않는다
    QList<homecards::Card> m_cards;
    SpriteCache *m_fronts = nullptr;
    mutable QHash<int, QPixmap> m_mascots; // 포켓몬 → 여백 잘라 낸 그림

    qreal m_angle = 0; // 배럴의 회전(라디안). 카드 i의 각 = i × step + m_angle
    QVariantAnimation *m_turnAnimation = nullptr;

    bool m_pressed = false;
    bool m_dragging = false;
    QPointF m_pressPos;
    qreal m_pressAngle = 0;
};
} // namespace com::yamada::studio
