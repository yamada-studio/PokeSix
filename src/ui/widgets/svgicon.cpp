#include "ui/widgets/svgicon.h"

#include <QPainter>
#include <QSvgRenderer>

namespace com::yamada::studio::svgicon {
QPixmap pixmap(const QString &svg, int size, qreal devicePixelRatio)
{
    QPixmap result(QSize(size, size) * devicePixelRatio);
    result.setDevicePixelRatio(devicePixelRatio);
    result.fill(Qt::transparent);
    QSvgRenderer renderer(svg.toUtf8());
    QPainter painter(&result);
    painter.setRenderHint(QPainter::Antialiasing, true);
    renderer.render(&painter, QRectF(0, 0, size, size)); // 논리 좌표로 그리면 DPR만큼 확대된다
    return result;
}

QIcon icon(const QString &svg, int size, qreal devicePixelRatio)
{
    return QIcon(pixmap(svg, size, devicePixelRatio));
}
} // namespace com::yamada::studio::svgicon
