#include "ui/squad/squadpaint.h"

#include "ui/theme/theme.h"
#include "ui/widgets/typechip.h"

#include <QCoreApplication>
#include <QPainter>
#include <QPainterPath>

namespace com::yamada::studio::squadpaint {
QString typeAbbr(const tok::TypeColor &type, Language language)
{
    if (language == Language::Korean)
        return QString::fromUtf16(type.abbr);
    const QString name = typechip::label(type, language);
    return language == Language::Japanese ? name.left(2) : name.left(3);
}

QString multiplierText(double multiplier)
{
    if (multiplier >= 4.0)
        return QStringLiteral("4");
    if (multiplier >= 2.0)
        return QStringLiteral("2");
    if (multiplier <= 0.0)
        return QStringLiteral("0");
    if (multiplier <= 0.25)
        return QStringLiteral("¼");
    if (multiplier <= 0.5)
        return QStringLiteral("½");
    return QString();
}

QString damageClassAbbr(int damageClass)
{
    switch (damageClass) {
    case 2:
        return QCoreApplication::translate("com::yamada::studio::squadpaint", "물");
    case 3:
        return QCoreApplication::translate("com::yamada::studio::squadpaint", "특");
    default:
        return QCoreApplication::translate("com::yamada::studio::squadpaint", "변");
    }
}

void paintTypeBox(QPainter &painter, const QRectF &rect, const tok::TypeColor &type,
                  const QString &text, int fontSize)
{
    painter.setPen(QPen(QColor(tok::kInk), 1.5));
    painter.setBrush(QColor(type.fill));
    painter.drawRoundedRect(rect.adjusted(0.75, 0.75, -0.75, -0.75), 3, 3);
    painter.setFont(theme::font(theme::kFamilyBody, fontSize, QFont::ExtraBold));
    painter.setPen(QColor(type.text));
    painter.drawText(rect, Qt::AlignCenter, text);
}

void paintDamageClass(QPainter &painter, const QRectF &rect, int damageClass)
{
    const QRgb fill = damageClass == 2   ? tok::kCatPhysical
                      : damageClass == 3 ? tok::kCatSpecial
                                         : tok::kCatStatus;
    painter.setPen(QPen(QColor(tok::kInk), 1.2));
    painter.setBrush(QColor(fill));
    painter.drawRoundedRect(rect.adjusted(0.6, 0.6, -0.6, -0.6), 3, 3);
    painter.setFont(theme::font(theme::kFamilyBody, 10, QFont::ExtraBold));
    painter.setPen(QColor(damageClass == 1 ? tok::kInk : tok::kWhite));
    painter.drawText(rect, Qt::AlignCenter, damageClassAbbr(damageClass));
}

void paintHatch(QPainter &painter, const QRectF &rect)
{
    painter.save();
    painter.setClipRect(rect);
    painter.fillRect(rect, QColor(tok::kPaperAlt));
    painter.setPen(QPen(QColor(tok::kLine), 1));
    for (qreal x = rect.left() - rect.height(); x < rect.right(); x += 5)
        painter.drawLine(QPointF(x, rect.bottom()), QPointF(x + rect.height(), rect.top()));
    painter.restore();
}
} // namespace com::yamada::studio::squadpaint
