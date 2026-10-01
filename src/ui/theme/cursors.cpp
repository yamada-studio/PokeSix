#include "ui/theme/cursors.h"

#include <QGuiApplication>
#include <QPainter>
#include <QPixmap>
#include <QSvgRenderer>

namespace {
constexpr int kSize = 32;          // 커서 크기(논리 px)
constexpr QPoint kHotSpot {13, 1}; // 검지 끝

// SVG → 커서. 화면 배율(devicePixelRatio)만큼 크게 그려서 고배율 화면에서도 선이 또렷하다.
QCursor fromSvg(const char *path)
{
    const qreal ratio = qApp ? qApp->devicePixelRatio() : 1.0;
    QPixmap pixmap(QSize(kSize, kSize) * ratio);
    pixmap.fill(Qt::transparent);
    QSvgRenderer renderer(QString::fromLatin1(path));
    QPainter painter(&pixmap);
    renderer.render(&painter);
    painter.end();
    pixmap.setDevicePixelRatio(ratio);
    return QCursor(pixmap, kHotSpot.x(), kHotSpot.y());
}
} // namespace

namespace com::yamada::studio::cursors {
const QCursor &pointer()
{
    // 함수 안 static: 처음 부를 때 한 번 만든다(QGuiApplication이 생긴 뒤에만 부른다 — 위젯 생성자
    // 등)
    static const QCursor cursor = fromSvg(":/icons/cursor/glove-point.svg");
    return cursor;
}

const QCursor &grab()
{
    static const QCursor cursor = fromSvg(":/icons/cursor/glove-grab.svg");
    return cursor;
}
} // namespace com::yamada::studio::cursors
