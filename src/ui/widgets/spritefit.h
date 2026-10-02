#pragma once

#include <QRect>

class QPainter;
class QPixmap;

namespace com::yamada::studio::spritefit {
// 스프라이트를 칸 가운데에 그린다. 작은 도트 그림은 정수 배(기본 2배)로 또렷하게, 칸보다 커지면
// (9세대 256×256 렌더, 기본 그림 96×96의 2배) 비율을 지키며 칸에 맞게 줄이고 부드럽게 보간한다.
// 도트 확대는 이웃 픽셀 그대로, 축소는 SmoothPixmapTransform — 렌더 힌트는 그리고 나서 되돌린다.
void draw(QPainter &painter, const QRect &box, const QPixmap &sprite, int scale = 2);
} // namespace com::yamada::studio::spritefit
