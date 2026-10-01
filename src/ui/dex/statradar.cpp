#include "ui/dex/statradar.h"

#include "ui/theme/theme.h"
#include "ui/theme/tokens.h"

#include <QPainter>
#include <QPainterPath>
#include <QtMath>

namespace {
constexpr int kScaleMax = 200;
constexpr int kRings = 4;          // 50 · 100 · 150 · 200
constexpr int kLabelMargin = 44;   // 축 이름 · 값이 들어갈 바깥 여백
constexpr qreal kFillAlpha = 0.28; // 도형 채움(빨강 옅게)

// 그리는 순서(시계방향, 위에서부터) → Repository의 능력치 순서(HP 공격 방어 특공 특방 스피드)
constexpr int kAxisToStat[6] = {0, 1, 2, 5, 4, 3};
} // namespace

namespace com::yamada::studio {
StatRadar::StatRadar(QWidget *parent)
    : QWidget(parent)
{
    QWidget::setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void StatRadar::setStats(const std::array<int, 6> &stats)
{
    m_stats = stats;
    QWidget::update();
}

void StatRadar::setNature(int increased, int decreased)
{
    m_increased = increased == decreased ? -1 : increased; // 무보정 성격은 표시하지 않는다
    m_decreased = increased == decreased ? -1 : decreased;
    QWidget::update();
}

QSize StatRadar::sizeHint() const
{
    return {260, 260};
}

void StatRadar::paintEvent(QPaintEvent *)
{
    const QString names[6]
            = {tr("HP"), tr("공격"), tr("방어"), tr("특공"), tr("특방"), tr("스피드")};
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QPointF center = QRectF(rect()).center();
    const qreal radius = std::max(10.0, std::min(width(), height()) / 2.0 - kLabelMargin);
    // 축 i의 끝점(반지름 r). 0번 축이 위(−90°)이고 60°씩 시계방향.
    auto point = [&](int axis, qreal r) {
        const qreal angle = qDegreesToRadians(-90.0 + 60.0 * axis);
        return center + QPointF(std::cos(angle) * r, std::sin(angle) * r);
    };

    // 1) 눈금 육각형 · 축선
    painter.setBrush(Qt::NoBrush);
    for (int ring = 1; ring <= kRings; ++ring) {
        QPolygonF hexagon;
        for (int axis = 0; axis < 6; ++axis)
            hexagon << point(axis, radius * ring / kRings);
        painter.setPen(QPen(QColor(ring == kRings ? tok::kLineStrong : tok::kLine), 1));
        painter.drawPolygon(hexagon);
    }
    painter.setPen(QPen(QColor(tok::kLine), 1));
    for (int axis = 0; axis < 6; ++axis)
        painter.drawLine(center, point(axis, radius));

    // 2) 종족값 도형
    QPolygonF shape;
    for (int axis = 0; axis < 6; ++axis) {
        const int value = std::min(m_stats[kAxisToStat[axis]], kScaleMax);
        shape << point(axis, radius * value / kScaleMax);
    }
    QColor fill(tok::kRed);
    fill.setAlphaF(kFillAlpha);
    painter.setBrush(fill);
    painter.setPen(QPen(QColor(tok::kRed), 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPolygon(shape);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(tok::kRed));
    for (const QPointF &p : shape)
        painter.drawEllipse(p, 2.5, 2.5);

    // 3) 축 이름 + 값: 축 끝 바깥에. 위 · 아래 축은 가운데 정렬, 왼쪽 · 오른쪽 축은 바깥쪽으로
    // 붙인다.
    const QFont nameFont = theme::font(theme::kFamilyBody, 12, QFont::ExtraBold);
    const QFont valueFont = theme::font(theme::kFamilyData, 13, QFont::Bold);
    for (int axis = 0; axis < 6; ++axis) {
        const int stat = kAxisToStat[axis];
        const QPointF tip = point(axis, radius + 10);
        const bool top = axis == 0;
        const bool bottom = axis == 3;
        const bool right = axis == 1 || axis == 2;
        QRectF box(0, 0, 80, 34);
        if (top)
            box.moveCenter(tip - QPointF(0, 14));
        else if (bottom)
            box.moveCenter(tip + QPointF(0, 14));
        else if (right)
            box.moveTopLeft(tip - QPointF(2, 17));
        else
            box.moveTopRight(tip + QPointF(2, -17));
        const Qt::Alignment horizontal = top || bottom ? Qt::AlignHCenter
                                         : right       ? Qt::AlignLeft
                                                       : Qt::AlignRight;
        // 성격 보정: 오르는 능력치 "특공 ▲"(빨강) · 내리는 능력치 "공격 ▼"(파랑)
        QString name = names[stat];
        QColor nameColor(tok::kText2);
        if (stat == m_increased) {
            name += QStringLiteral(" ▲");
            nameColor = QColor(tok::kRed);
        } else if (stat == m_decreased) {
            name += QStringLiteral(" ▼");
            nameColor = QColor(tok::kBlueDeep);
        }
        painter.setFont(nameFont);
        painter.setPen(nameColor);
        painter.drawText(QRectF(box.left(), box.top(), box.width(), 16),
                         horizontal | Qt::AlignVCenter, name);
        painter.setFont(valueFont);
        painter.setPen(QColor(m_stats[stat] >= 100 ? tok::kText1 : tok::kText3));
        painter.drawText(QRectF(box.left(), box.top() + 16, box.width(), 18),
                         horizontal | Qt::AlignVCenter, QString::number(m_stats[stat]));
    }
}
} // namespace com::yamada::studio
