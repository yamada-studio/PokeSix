#include "ui/dex/dexheaderview.h"

#include "ui/dex/dexrowdelegate.h"
#include "ui/theme/theme.h"
#include "ui/theme/tokens.h"

#include <QFontMetricsF>
#include <QPainter>
#include <QPainterPath>

namespace {
constexpr int kHeight = 30;     // 글자 줄 28 + 아래 먹선 2
constexpr int kBottomLine = 2;  // border-bottom: 2px solid ink
constexpr QSizeF kArrow {7, 4}; // 정렬 화살표(아래 · 위 삼각형)
constexpr qreal kArrowGap = 3;  // 화살표와 글자 사이
} // namespace

namespace com::yamada::studio {
DexHeaderView::DexHeaderView(QWidget *parent)
    : QHeaderView(Qt::Horizontal, parent)
{
    QHeaderView::setFixedHeight(kHeight);
    QHeaderView::setSectionsClickable(true); // 머리 칸을 누르면 그 칸으로 정렬
    QHeaderView::setHighlightSections(false);
}

void DexHeaderView::paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const
{
    painter->save();
    const QRectF cell = rect;
    painter->fillRect(cell, QColor(tok::kWhite));
    painter->fillRect(
            QRectF(cell.left(), cell.bottom() - kBottomLine + 1, cell.width(), kBottomLine),
            QColor(tok::kInk));

    const QString text = model()->headerData(logicalIndex, orientation()).toString();
    const bool sorted = isSortIndicatorShown() && sortIndicatorSection() == logicalIndex;
    const QColor color(sorted ? tok::kRed : tok::kText2);

    // 글자 자리: 데이터 칸과 같은 함수. 세로는 먹선 위 글자 줄의 가운데.
    const QRectF line = cell.adjusted(0, 0, 0, -kBottomLine);
    const QRectF content = DexRowDelegate::contentRect(line, logicalIndex);
    const Qt::Alignment align = DexRowDelegate::alignment(logicalIndex);
    const QFont font = theme::font(theme::kFamilyBody, 11, QFont::ExtraBold);
    painter->setFont(font);
    painter->setPen(color);
    painter->drawText(content, align, text);

    if (sorted) {
        // 화살표는 글자의 정렬 반대쪽에: 왼쪽 정렬이면 글자 뒤, 오른쪽 정렬이면 글자 앞.
        const qreal textWidth = QFontMetricsF(font).horizontalAdvance(text);
        const qreal x = (align & Qt::AlignRight)
                                ? content.right() - textWidth - kArrowGap - kArrow.width()
                                : content.left() + textWidth + kArrowGap;
        const qreal top = line.center().y() - kArrow.height() / 2;
        // Qt 관례: AscendingOrder = 위 삼각형(작은 값이 위), DescendingOrder = 아래 삼각형.
        const bool ascending = sortIndicatorOrder() == Qt::AscendingOrder;
        QPainterPath arrow;
        if (ascending) {
            arrow.moveTo(x, top + kArrow.height());
            arrow.lineTo(x + kArrow.width(), top + kArrow.height());
            arrow.lineTo(x + kArrow.width() / 2, top);
        } else {
            arrow.moveTo(x, top);
            arrow.lineTo(x + kArrow.width(), top);
            arrow.lineTo(x + kArrow.width() / 2, top + kArrow.height());
        }
        arrow.closeSubpath();
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->fillPath(arrow, color);
    }
    painter->restore();
}
} // namespace com::yamada::studio
