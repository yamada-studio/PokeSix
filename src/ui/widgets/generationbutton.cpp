#include "ui/widgets/generationbutton.h"

#include "ui/theme/theme.h"
#include "ui/theme/tokens.h"

#include <QFontMetricsF>
#include <QPainter>
#include <QPainterPath>
#include <QtMath>

namespace {
// 크기별 수치. Large = Intro.dc.html의 세대 버튼, Compact = Dex.dc.html 등 앱 막대의 세대 버튼.
struct Metrics
{
    int height;    // 테두리 포함 높이
    int shadow;    // box-shadow: 0 Npx 0
    int paddingX;  // padding: 0 Npx
    int gap;       // 뱃지 · 지역명 · ▾ 사이
    int regionPx;  // 지역명: 도현
    int badgePx;   // 뱃지: Silkscreen
    int badgePadX; // 뱃지 padding: y x
    int badgePadY;
    int badgeRadius;
    int chevron; // ▾ 아이콘 크기 (viewBox 24)
};
constexpr Metrics kLarge {46, 3, 16, 10, 20, 13, 7, 3, 4, 16};
constexpr Metrics kCompact {36, 2, 12, 8, 16, 11, 5, 2, 3, 14};
constexpr int kBorder = 2;        // border: 2px solid ink
constexpr int kRadiusLarge = 8;   // border-radius: 인트로 8px
constexpr int kRadiusCompact = 6; // 〃 앱 막대 6px
constexpr int kPressedOffset = 2; // 눌림: 2px 내려앉고 그림자가 사라진다

const Metrics &metricsOf(com::yamada::studio::GenerationButton::Size size)
{
    return size == com::yamada::studio::GenerationButton::Size::Large ? kLarge : kCompact;
}
} // namespace

namespace com::yamada::studio {
GenerationButton::GenerationButton(Size size, QWidget *parent)
    : QAbstractButton(parent)
    , m_size(size)
    , m_badgeFont(theme::font(theme::kFamilyPixel, metricsOf(size).badgePx))
    , m_regionFont(theme::font(theme::kFamilyTitle, metricsOf(size).regionPx))
{
    QAbstractButton::setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    QAbstractButton::setCursor(Qt::PointingHandCursor);
}

void GenerationButton::setGeneration(int number, const QString &region)
{
    m_number = number;
    m_region = region;
    // 접근성: 화면 낭독기가 읽을 이름. 그림으로만 그린 글자는 스스로 알릴 수 없다.
    QAbstractButton::setAccessibleName(tr("세대 %1 %2").arg(number).arg(region));
    QAbstractButton::updateGeometry(); // 글자 폭이 바뀌면 sizeHint도 바뀐다 → 레이아웃에 다시 묻게
                                       // 한다
    QAbstractButton::update();
}

QString GenerationButton::badgeText() const
{
    return QStringLiteral("GEN %1").arg(m_number);
}

QSize GenerationButton::sizeHint() const
{
    const Metrics &m = metricsOf(m_size);
    const QFontMetricsF badge(m_badgeFont);
    const QFontMetricsF region(m_regionFont);
    const qreal badgeWidth = badge.horizontalAdvance(badgeText()) + 2 * m.badgePadX;
    const qreal width = 2 * kBorder + 2 * m.paddingX + badgeWidth + m.gap
                        + region.horizontalAdvance(m_region) + m.gap + m.chevron;
    return {qCeil(width), m.height + m.shadow};
}

void GenerationButton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    const Metrics &m = metricsOf(m_size);
    const int radius = m_size == Size::Large ? kRadiusLarge : kRadiusCompact;
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // isDown(): 마우스를 누르고 있는 동안 true (QAbstractButton이 관리).
    const bool down = QAbstractButton::isDown();
    const int offset = down ? kPressedOffset : 0;
    const QRectF box(0, offset, width(), m.height);

    // 1) 그림자 — 눌리면 버튼이 그림자 위로 내려앉은 것처럼 보이도록 그리지 않는다.
    if (!down) {
        QPainterPath shadow;
        shadow.addRoundedRect(box.translated(0, m.shadow), radius, radius);
        painter.fillPath(shadow, QColor(tok::kInk));
    }

    // 2) 본체: 흰 바탕(눌림은 노란 옅은 바탕) + 먹선 2. 펜은 선의 가운데를 따라 그리므로 반 폭
    // 안쪽으로.
    const qreal half = kBorder / 2.0;
    painter.setPen(QPen(QColor(tok::kInk), kBorder));
    painter.setBrush(QColor(down ? tok::kYellowTint : tok::kWhite));
    painter.drawRoundedRect(box.adjusted(half, half, -half, -half), radius - half, radius - half);

    // 3) 뱃지 "GEN n": 먹색 바탕 + 연노랑 글자
    const QFontMetricsF badgeMetrics(m_badgeFont);
    const QString badge = badgeText();
    const qreal badgeW = badgeMetrics.horizontalAdvance(badge) + 2 * m.badgePadX;
    const qreal badgeH = badgeMetrics.height() + 2 * m.badgePadY;
    qreal x = kBorder + m.paddingX;
    const QRectF badgeRect(x, box.center().y() - badgeH / 2.0, badgeW, badgeH);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(tok::kInk));
    painter.drawRoundedRect(badgeRect, m.badgeRadius, m.badgeRadius);
    painter.setFont(m_badgeFont);
    painter.setPen(QColor(tok::kYellowSoft));
    painter.drawText(badgeRect, Qt::AlignCenter, badge);

    // 4) 지역명 (도현 20)
    x += badgeW + m.gap;
    const QFontMetricsF regionMetrics(m_regionFont);
    const qreal regionW = regionMetrics.horizontalAdvance(m_region);
    painter.setFont(m_regionFont);
    painter.setPen(QColor(tok::kText1));
    painter.drawText(QRectF(x, box.top(), regionW, box.height()), Qt::AlignVCenter | Qt::AlignLeft,
                     m_region);

    // 5) ▾ — SVG path "M6 9 l6 6 6 -6"(viewBox 24)를 16px로 줄여 그린다.
    x += regionW + m.gap;
    const qreal scale = m.chevron / 24.0;
    const QPointF topLeft(x, box.center().y() - m.chevron / 2.0);
    QPainterPath chevron;
    chevron.moveTo(topLeft + QPointF(6, 9) * scale);
    chevron.lineTo(topLeft + QPointF(12, 15) * scale);
    chevron.lineTo(topLeft + QPointF(18, 9) * scale);
    QPen chevronPen(QColor(tok::kText1), 2.6 * scale);
    chevronPen.setCapStyle(Qt::FlatCap);    // SVG 기본값(stroke-linecap: butt)과 같게
    chevronPen.setJoinStyle(Qt::MiterJoin); // SVG 기본값(stroke-linejoin: miter)과 같게
    painter.setPen(chevronPen);
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(chevron);
}
} // namespace com::yamada::studio
