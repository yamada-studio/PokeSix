#include "ui/widgets/panelpainter.h"

#include <QColor>
#include <QPainter>
#include <QPainterPath>
#include <QRect>

namespace com::yamada::studio {
namespace {
// QPen은 경로의 "가운데"를 따라 선을 긋는다. 폭 3짜리 선을 상자 가장자리에 그대로 그리면
// 절반(1.5)이 상자 밖으로 나가 잘린다. 그래서 선 폭의 절반만큼 안쪽으로 들인 경로를 만든다.
// CSS의 border처럼 "선의 바깥 가장자리 = outer, 바깥 모서리 반경 = radius"가 되도록.
QPainterPath strokePath(const QRectF &outer, qreal radius, qreal pen)
{
    const qreal half = pen / 2.0;
    QPainterPath path;
    path.addRoundedRect(outer.adjusted(half, half, -half, -half), radius - half, radius - half);
    return path;
}
} // namespace

QMargins chromeMargins(const PanelStyle &style)
{
    const int side = style.outline + style.innerInset + style.innerLine;
    return {side, side, side, side + style.shadow};
}

void paintPanel(QPainter &painter, const QRect &rect, const PanelStyle &style)
{
    // QPainter는 펜 · 브러시 · 렌더 힌트를 "상태"로 들고 있다. 이 함수가 바꾼 상태가
    // 호출한 쪽의 다음 그리기에 새지 않도록 save()/restore()로 감싼다.
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true); // 둥근 모서리를 매끄럽게

    // 창 본체 = 위젯 rect에서 아래 그림자 몫을 뺀 상자
    const QRectF box = QRectF(rect).adjusted(0, 0, 0, -style.shadow);

    // 1) 그림자: 본체와 같은 모양을 아래로 옮겨 먹색으로 채운다. QGraphicsDropShadowEffect(블러)는
    // 쓰지 않는다.
    if (style.shadow > 0) {
        QPainterPath shadow;
        shadow.addRoundedRect(box.translated(0, style.shadow), style.radius, style.radius);
        painter.fillPath(shadow, QColor(style.ink));
    }

    // 2) 본체: 바탕(브러시) + 먹선(펜)을 한 번에. 그림자 위를 덮으므로 그림자는 아래쪽만 남는다.
    painter.setPen(QPen(QColor(style.ink), style.outline));
    painter.setBrush(QColor(style.fill));
    painter.drawPath(strokePath(box, style.radius, style.outline));

    // 3) 안쪽 선(이중 테): 먹선 + 간격만큼 들어간 곳에 가는 선을 하나 더
    if (style.innerLine > 0) {
        const qreal inset = style.outline + style.innerInset;
        const QRectF inner = box.adjusted(inset, inset, -inset, -inset);
        painter.setPen(QPen(QColor(style.innerColor), style.innerLine));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(strokePath(inner, style.innerRadius, style.innerLine));
    }

    painter.restore();
}
} // namespace com::yamada::studio
