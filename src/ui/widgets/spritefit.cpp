#include "ui/widgets/spritefit.h"

#include <QPainter>
#include <QPixmap>

namespace com::yamada::studio::spritefit {
void draw(QPainter &painter, const QRect &box, const QPixmap &sprite, int scale)
{
    if (sprite.isNull() || box.isEmpty())
        return;
    QSize size = sprite.size() * scale;
    const bool shrink = size.width() > box.width() || size.height() > box.height();
    if (shrink)
        size = sprite.size().scaled(box.size(), Qt::KeepAspectRatio);
    const QRect target(box.center() - QPoint(size.width() / 2, size.height() / 2), size);
    const bool smooth = painter.testRenderHint(QPainter::SmoothPixmapTransform);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, shrink);
    painter.drawPixmap(target, sprite);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, smooth);
}
} // namespace com::yamada::studio::spritefit
