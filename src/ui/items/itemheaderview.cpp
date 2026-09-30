#include "ui/items/itemheaderview.h"

#include "data/models/itemtablemodel.h"
#include "ui/theme/theme.h"
#include "ui/theme/tokens.h"

#include <QFontMetricsF>
#include <QPainter>
#include <QPainterPath>

namespace {
constexpr int kHeight = 30; // 글자 줄 28 + 아래 먹선 2
constexpr int kBottomLine = 2;
constexpr qreal kPadding = 4;   // 데이터 칸과 같은 좌우 여백
constexpr QSizeF kArrow {7, 4}; // 정렬 화살표
constexpr qreal kArrowGap = 3;
} // namespace

namespace com::yamada::studio {
ItemHeaderView::ItemHeaderView(QWidget *parent)
    : QHeaderView(Qt::Horizontal, parent)
{
    QHeaderView::setFixedHeight(kHeight);
    QHeaderView::setSectionsClickable(true); // 머리 칸을 누르면 그 칸으로 정렬
    QHeaderView::setHighlightSections(false);
}

void ItemHeaderView::paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    const QRectF cell = rect;
    painter->fillRect(cell, QColor(tok::kWhite));
    painter->fillRect(
            QRectF(cell.left(), cell.bottom() - kBottomLine + 1, cell.width(), kBottomLine),
            QColor(tok::kInk));

    const QString text = model()->headerData(logicalIndex, orientation()).toString();
    const bool sorted = isSortIndicatorShown() && sortIndicatorSection() == logicalIndex;
    const bool right = logicalIndex == ItemTableModel::PriceColumn; // 숫자 칸은 오른쪽
    const QColor color(sorted ? tok::kRed : tok::kText2);
    const QRectF line = cell.adjusted(0, 0, 0, -kBottomLine);
    const QRectF content = line.adjusted(kPadding, 0, -kPadding, 0);
    const QFont font = theme::font(theme::kFamilyBody, 11, QFont::ExtraBold);
    painter->setFont(font);
    painter->setPen(color);
    painter->drawText(content, (right ? Qt::AlignRight : Qt::AlignLeft) | Qt::AlignVCenter, text);

    if (sorted && !text.isEmpty()) {
        const qreal textWidth = QFontMetricsF(font).horizontalAdvance(text);
        const qreal x = right ? content.right() - textWidth - kArrowGap - kArrow.width()
                              : content.left() + textWidth + kArrowGap;
        const qreal top = line.center().y() - kArrow.height() / 2;
        const bool ascending = sortIndicatorOrder() == Qt::AscendingOrder; // 위 삼각형
        QPainterPath arrow;
        arrow.moveTo(x, ascending ? top + kArrow.height() : top);
        arrow.lineTo(x + kArrow.width(), ascending ? top + kArrow.height() : top);
        arrow.lineTo(x + kArrow.width() / 2, ascending ? top : top + kArrow.height());
        arrow.closeSubpath();
        painter->fillPath(arrow, color);
    }
    painter->restore();
}
} // namespace com::yamada::studio
