#include "ui/shell/markbutton.h"

#include "ui/theme/cursors.h"
#include "ui/theme/theme.h"
#include "ui/theme/tokens.h"
#include "ui/widgets/svgicon.h"

#include <QFocusEvent>
#include <QFontMetricsF>
#include <QPainter>
#include <QtMath>

namespace {
// Dex.dc.html 앱 막대의 마크: 32 규칙(광택 · 입 · 볼 없음) + 빨강 바탕용 흰 스티커 테(01b §6-2)
// 구분자를 svg로 둔 raw string: 내용 안의 translate(-50 -50)">에 )"가 있어 기본 R"(…)"로는 중간에
// 끝나 버린다.
const char kStickerMarkSvg[]
        = R"svg(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100"><g transform="translate(50 50) scale(1.071) rotate(-35) translate(-50 -50)"><rect x="10" y="25" width="80" height="50" rx="25" fill="#FFFFFF" stroke="#FFFFFF" stroke-width="16"/><path d="M50 25 L35 25 A25 25 0 0 0 35 75 L50 75 Z" fill="#D8322B"/><path d="M50 25 L65 25 A25 25 0 0 1 65 75 L50 75 Z" fill="#FFF8EC"/><line x1="50" y1="25" x2="50" y2="75" stroke="#2A2A33" stroke-width="6"/><rect x="10" y="25" width="80" height="50" rx="25" fill="none" stroke="#2A2A33" stroke-width="7"/><ellipse cx="61" cy="47" rx="4.8" ry="5.4" fill="#2A2A33"/><ellipse cx="77" cy="47" rx="4.8" ry="5.4" fill="#2A2A33"/></g></svg>)svg";
constexpr int kMarkSize = com::yamada::studio::tok::kSizeAppBarMark; // 36
constexpr int kGap = 10;        // 마크와 워드마크 사이
constexpr int kWordmarkPx = 20; // Silkscreen 700 · 20
// hover 테: padding 3px 8px 3px 3px 인 상자에 outline 2px dashed · offset 2
constexpr QMargins kPadding {3, 3, 8, 3};
constexpr int kOutline = 2;
constexpr int kOutlineOffset = 2;
constexpr int kOutlineRadius = 8;
constexpr int kDecoration = kOutline + kOutlineOffset; // 상자 바깥의 테가 차지하는 폭
} // namespace

namespace com::yamada::studio {
MarkButton::MarkButton(QWidget *parent)
    : QAbstractButton(parent)
    , m_font(theme::font(theme::kFamilyPixel, kWordmarkPx, QFont::Bold))
{
    m_mark = svgicon::pixmap(QString::fromLatin1(kStickerMarkSvg), kMarkSize, devicePixelRatioF());
    QAbstractButton::setCursor(cursors::pointer());
    QAbstractButton::setToolTip(tr("처음 화면으로"));
    QAbstractButton::setAccessibleName(tr("처음 화면으로"));
    QAbstractButton::setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

QSize MarkButton::sizeHint() const
{
    const QFontMetricsF metrics(m_font);
    const qreal content = kMarkSize + kGap + metrics.horizontalAdvance(QStringLiteral("POKESIX"));
    return {qCeil(content) + kPadding.left() + kPadding.right() + 2 * kDecoration,
            kMarkSize + kPadding.top() + kPadding.bottom() + 2 * kDecoration};
}

void MarkButton::enterEvent(QEnterEvent *event)
{
    QAbstractButton::enterEvent(event);
    QAbstractButton::update();
}

void MarkButton::leaveEvent(QEvent *event)
{
    QAbstractButton::leaveEvent(event);
    QAbstractButton::update();
}

void MarkButton::focusInEvent(QFocusEvent *event)
{
    QAbstractButton::focusInEvent(event);
    m_keyboardFocus
            = event->reason() == Qt::TabFocusReason || event->reason() == Qt::BacktabFocusReason;
    QAbstractButton::update();
}

void MarkButton::focusOutEvent(QFocusEvent *event)
{
    QAbstractButton::focusOutEvent(event);
    m_keyboardFocus = false;
    QAbstractButton::update();
}

void MarkButton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // 상자 = 위젯에서 바깥 테 자리를 뺀 곳
    const QRectF box
            = QRectF(rect()).adjusted(kDecoration, kDecoration, -kDecoration, -kDecoration);

    // hover · 키보드 포커스: 연노랑 점선 테
    if (QAbstractButton::underMouse() || m_keyboardFocus) {
        QPen pen(QColor(tok::kYellowSoft), kOutline, Qt::DashLine);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        const qreal half = kOutline / 2.0;
        painter.drawRoundedRect(box.adjusted(-kDecoration + half, -kDecoration + half,
                                             kDecoration - half, kDecoration - half),
                                kOutlineRadius, kOutlineRadius);
    }

    const QPointF markTopLeft(box.left() + kPadding.left(), box.top() + kPadding.top());
    painter.drawPixmap(markTopLeft, m_mark);

    // 워드마크: POKE 흰색 + SIX 연노랑(yellow.soft)
    painter.setFont(m_font);
    const QFontMetricsF metrics(m_font);
    const qreal baseline = box.center().y() + (metrics.ascent() - metrics.descent()) / 2.0;
    const qreal x = markTopLeft.x() + kMarkSize + kGap;
    const QString poke = QStringLiteral("POKE");
    painter.setPen(QColor(tok::kWhite));
    painter.drawText(QPointF(x, baseline), poke);
    painter.setPen(QColor(tok::kYellowSoft));
    painter.drawText(QPointF(x + metrics.horizontalAdvance(poke), baseline), QStringLiteral("SIX"));
}
} // namespace com::yamada::studio
