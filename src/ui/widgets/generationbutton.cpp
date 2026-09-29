#include "ui/widgets/generationbutton.h"

#include "ui/theme/theme.h"
#include "ui/theme/tokens.h"

#include <QFontMetricsF>
#include <QPainter>
#include <QPainterPath>
#include <QtMath>

namespace {
// 수치는 Intro.dc.html의 <button> 인라인 스타일 그대로다.
constexpr int kHeight = 46;   // height: 46px (테두리 포함)
constexpr int kShadow = 3;    // box-shadow: 0 3px 0
constexpr int kBorder = 2;    // border: 2px solid ink
constexpr int kRadius = 8;    // border-radius: 8px
constexpr int kPaddingX = 16; // padding: 0 16px
constexpr int kGap = 10;      // gap: 10px (뱃지 · 지역명 · ▾ 사이)
constexpr int kBadgePadX = 7; // 뱃지 padding: 3px 7px
constexpr int kBadgePadY = 3;
constexpr int kBadgeRadius = 4;
constexpr int kChevron = 16;      // ▾ 아이콘 16×16 (viewBox 24, 선 2.6)
constexpr int kPressedOffset = 2; // 눌림: 2px 내려앉고 그림자가 사라진다
} // namespace

namespace com::yamada::studio {
GenerationButton::GenerationButton(QWidget *parent)
    : QAbstractButton(parent)
    , m_badgeFont(theme::font(theme::kFamilyPixel, 13))
    , m_regionFont(theme::font(theme::kFamilyTitle, 20))
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
    const QFontMetricsF badge(m_badgeFont);
    const QFontMetricsF region(m_regionFont);
    const qreal badgeWidth = badge.horizontalAdvance(badgeText()) + 2 * kBadgePadX;
    const qreal width = 2 * kBorder + 2 * kPaddingX + badgeWidth + kGap
                        + region.horizontalAdvance(m_region) + kGap + kChevron;
    return {qCeil(width), kHeight + kShadow};
}

void GenerationButton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // isDown(): 마우스를 누르고 있는 동안 true (QAbstractButton이 관리).
    const bool down = QAbstractButton::isDown();
    const int offset = down ? kPressedOffset : 0;
    const QRectF box(0, offset, width(), kHeight);

    // 1) 그림자 — 눌리면 버튼이 그림자 위로 내려앉은 것처럼 보이도록 그리지 않는다.
    if (!down) {
        QPainterPath shadow;
        shadow.addRoundedRect(box.translated(0, kShadow), kRadius, kRadius);
        painter.fillPath(shadow, QColor(tok::kInk));
    }

    // 2) 본체: 흰 바탕(눌림은 노란 옅은 바탕) + 먹선 2. 펜은 선의 가운데를 따라 그리므로 반 폭
    // 안쪽으로.
    const qreal half = kBorder / 2.0;
    painter.setPen(QPen(QColor(tok::kInk), kBorder));
    painter.setBrush(QColor(down ? tok::kYellowTint : tok::kWhite));
    painter.drawRoundedRect(box.adjusted(half, half, -half, -half), kRadius - half, kRadius - half);

    // 3) 뱃지 "GEN n": 먹색 바탕 + 연노랑 글자
    const QFontMetricsF badgeMetrics(m_badgeFont);
    const QString badge = badgeText();
    const qreal badgeW = badgeMetrics.horizontalAdvance(badge) + 2 * kBadgePadX;
    const qreal badgeH = badgeMetrics.height() + 2 * kBadgePadY;
    qreal x = kBorder + kPaddingX;
    const QRectF badgeRect(x, box.center().y() - badgeH / 2.0, badgeW, badgeH);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(tok::kInk));
    painter.drawRoundedRect(badgeRect, kBadgeRadius, kBadgeRadius);
    painter.setFont(m_badgeFont);
    painter.setPen(QColor(tok::kYellowSoft));
    painter.drawText(badgeRect, Qt::AlignCenter, badge);

    // 4) 지역명 (도현 20)
    x += badgeW + kGap;
    const QFontMetricsF regionMetrics(m_regionFont);
    const qreal regionW = regionMetrics.horizontalAdvance(m_region);
    painter.setFont(m_regionFont);
    painter.setPen(QColor(tok::kText1));
    painter.drawText(QRectF(x, box.top(), regionW, box.height()), Qt::AlignVCenter | Qt::AlignLeft,
                     m_region);

    // 5) ▾ — SVG path "M6 9 l6 6 6 -6"(viewBox 24)를 16px로 줄여 그린다.
    x += regionW + kGap;
    const qreal scale = kChevron / 24.0;
    const QPointF topLeft(x, box.center().y() - kChevron / 2.0);
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
