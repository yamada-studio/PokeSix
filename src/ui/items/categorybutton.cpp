#include "ui/items/categorybutton.h"

#include "ui/theme/cursors.h"
#include "ui/theme/theme.h"
#include "ui/theme/tokens.h"

#include <QFontMetricsF>
#include <QPainter>
#include <QPainterPath>
#include <QtMath>

namespace {
constexpr int kHeight = 44;
constexpr int kPaddingX = 10;
constexpr int kGap = 10;
constexpr int kRadius = 6;
constexpr int kBorder = 2;
constexpr QSizeF kCursor {9, 12}; // ▶ border-left 9 · 위아래 6
constexpr int kCursorSlot = 12; // ▶ 자리(선택 안 된 줄도 비워 둬서 견본이 한 줄로 선다)
constexpr int kSwatch = 16;
constexpr int kSwatchRadius = 4;
constexpr qreal kSwatchBorder = 1.5;
} // namespace

namespace com::yamada::studio {
CategoryButton::CategoryButton(const itemstyle::Group &group, QWidget *parent)
    : QAbstractButton(parent)
    , m_group(group)
    , m_font(theme::font(theme::kFamilyTitle, 18))
{
    QAbstractButton::setText(group.label.text(Language::Korean));
    QAbstractButton::setCheckable(true);
    QAbstractButton::setCursor(cursors::pointer());
    QAbstractButton::setFocusPolicy(Qt::TabFocus); // 마우스로 눌러도 검색 칸 포커스를 뺏지 않는다
    QAbstractButton::setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed); // 가로는 창 폭만큼
    QAbstractButton::setAttribute(Qt::WA_Hover); // enter/leave 때 다시 그려 hover 모양을 보인다
}

void CategoryButton::setLanguage(Language language)
{
    QAbstractButton::setText(
            m_group.label.text(language)); // setText가 다시 그리기 · 크기 알림을 한다
}

QSize CategoryButton::sizeHint() const
{
    const qreal width = 2 * kPaddingX + kCursorSlot + kGap + kSwatch + kGap
                        + QFontMetricsF(m_font).horizontalAdvance(text());
    return {qCeil(width), kHeight};
}

void CategoryButton::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    const bool on = isChecked();

    // 1) 바탕 · 테: 선택됨 = 노랑 옅은 바탕 + 먹선 2, hover = paper.alt, 평소 = 투명
    const qreal half = kBorder / 2.0;
    const QRectF box = QRectF(rect()).adjusted(half, half, -half, -half);
    if (on || underMouse() || hasFocus()) {
        const QColor ink(hasFocus() ? tok::kBlue : tok::kInk);
        painter.setPen(on || hasFocus() ? QPen(ink, kBorder) : Qt::NoPen);
        painter.setBrush(QColor(on ? tok::kYellowTint : tok::kPaperAlt));
        painter.drawRoundedRect(box, kRadius - half, kRadius - half);
    }

    // 2) ▶ (선택된 줄만)
    qreal x = kPaddingX;
    const qreal cy = height() / 2.0;
    if (on) {
        const qreal cx = x + kCursorSlot / 2.0;
        QPainterPath triangle;
        triangle.moveTo(cx - kCursor.width() / 2, cy - kCursor.height() / 2);
        triangle.lineTo(cx + kCursor.width() / 2, cy);
        triangle.lineTo(cx - kCursor.width() / 2, cy + kCursor.height() / 2);
        triangle.closeSubpath();
        painter.fillPath(triangle, QColor(tok::kInk));
    }
    x += kCursorSlot + kGap;

    // 3) 색 견본
    const qreal inset = kSwatchBorder / 2.0;
    painter.setPen(QPen(QColor(tok::kInk), kSwatchBorder));
    painter.setBrush(m_group.color);
    painter.drawRoundedRect(
            QRectF(x, cy - kSwatch / 2.0, kSwatch, kSwatch).adjusted(inset, inset, -inset, -inset),
            kSwatchRadius - inset, kSwatchRadius - inset);
    x += kSwatch + kGap;

    // 4) 이름
    painter.setFont(m_font);
    painter.setPen(QColor(tok::kText1));
    painter.drawText(QRectF(x, 0, width() - x - kPaddingX, height()),
                     Qt::AlignLeft | Qt::AlignVCenter, text());
}
} // namespace com::yamada::studio
