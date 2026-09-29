#include "ui/widgets/layoutguide.h"

#include <QPainter>
#include <QRect>
#include <QWidget>

namespace com::yamada::studio {
void paintLayoutGuide(QWidget &widget)
{
    const char *name = widget.metaObject()->className();

    QPainter painter(&widget);

    const QRect rect = widget.rect();

    QRect guideRect = rect.adjusted(0, 0, -1, -1);
    painter.drawText(guideRect, Qt::AlignCenter, QString::fromLatin1(name));
    painter.setPen(QPen(Qt::DashLine));
    painter.drawRect(guideRect);
}
} // namespace com::yamada::studio
