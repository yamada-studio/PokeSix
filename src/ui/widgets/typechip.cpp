#include "ui/widgets/typechip.h"

#include "ui/theme/theme.h"

#include <QColor>
#include <QFontMetricsF>
#include <QPainter>

namespace {
constexpr qreal kPaddingX = 7;
constexpr qreal kRadius = 4;
constexpr qreal kBorder = 1.5;
} // namespace

namespace com::yamada::studio::typechip {
const tok::TypeColor *find(const QString &identifier)
{
    const QByteArray key = identifier.toLatin1();
    for (const tok::TypeColor &type : tok::kTypes) {
        if (key == type.key)
            return &type;
    }
    return nullptr;
}

QFont font()
{
    return theme::font(theme::kFamilyBody, 11, QFont::ExtraBold);
}

qreal width(const tok::TypeColor &type)
{
    return QFontMetricsF(font()).horizontalAdvance(QString::fromUtf16(type.ko)) + 2 * kPaddingX;
}

qreal paint(QPainter &painter, const QPointF &topLeft, const tok::TypeColor &type)
{
    const QString name = QString::fromUtf16(type.ko);
    const QRectF chip(topLeft, QSizeF(width(type), kHeight));
    const qreal half = kBorder / 2.0;
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(QColor(tok::kInk), kBorder));
    painter.setBrush(QColor(type.fill));
    painter.drawRoundedRect(chip.adjusted(half, half, -half, -half), kRadius - half,
                            kRadius - half);
    painter.setFont(font());
    painter.setPen(QColor(type.text));
    painter.drawText(chip, Qt::AlignCenter, name);
    painter.restore();
    return chip.width();
}
} // namespace com::yamada::studio::typechip
