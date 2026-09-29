#pragma once

#include <QIcon>
#include <QPixmap>
#include <QString>

namespace com::yamada::studio::svgicon {
// 디자인 원본(*.dc.html)의 인라인 <svg> 문자열을 그대로 아이콘으로 쓴다.
// 크기는 논리 px. 고해상도 화면(devicePixelRatio 2 등)에서도 흐리지 않게 실제 픽셀로 그린다.
QPixmap pixmap(const QString &svg, int size, qreal devicePixelRatio);
QIcon icon(const QString &svg, int size, qreal devicePixelRatio);
} // namespace com::yamada::studio::svgicon
