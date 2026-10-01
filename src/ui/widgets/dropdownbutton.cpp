#include "ui/widgets/dropdownbutton.h"

#include "ui/theme/cursors.h"
#include "ui/theme/theme.h"
#include "ui/theme/tokens.h"

#include <QFontMetricsF>
#include <QPainter>

namespace com::yamada::studio {
DropdownButton::DropdownButton(const QString &text, QWidget *parent)
    : QAbstractButton(parent)
{
    QAbstractButton::setCursor(cursors::pointer());
    QAbstractButton::setFocusPolicy(Qt::TabFocus);
    QAbstractButton::setText(text);
}

QSize DropdownButton::sizeHint() const
{
    const QFont font = theme::font(theme::kFamilyBody, 12, QFont::ExtraBold);
    return {int(QFontMetricsF(font).horizontalAdvance(text() + QStringLiteral("  ▾"))) + 20, 24};
}

void DropdownButton::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(QColor(hasFocus() ? tok::kBlue : tok::kInk), 1.5));
    painter.setBrush(QColor(underMouse() || isDown() ? tok::kYellowTint : tok::kWhite));
    painter.drawRoundedRect(QRectF(rect()).adjusted(0.75, 0.75, -0.75, -0.75), 5, 5);
    painter.setFont(theme::font(theme::kFamilyBody, 12, QFont::ExtraBold));
    painter.setPen(QColor(tok::kText1));
    painter.drawText(rect(), Qt::AlignCenter, text() + QStringLiteral("  ▾"));
}
} // namespace com::yamada::studio
