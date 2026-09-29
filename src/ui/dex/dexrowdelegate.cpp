#include "ui/dex/dexrowdelegate.h"

#include "data/models/speciestablemodel.h"
#include "ui/theme/theme.h"
#include "ui/theme/tokens.h"
#include "ui/widgets/typechip.h"

#include <QPainter>
#include <QPainterPath>

namespace {
using namespace com::yamada::studio;

constexpr int kRowHeight = tok::kSizeTableRow; // 34
constexpr qreal kCellPadding = 4;              // 열 사이 gap 8의 절반씩
constexpr QSizeF kCursor {9, 12};              // ▶ border-left 9 · 위아래 6

QColor statColor(int value)
{
    if (value >= 120)
        return QColor(tok::kBlueDeep);
    if (value <= 59)
        return QColor(tok::kRedDeep);
    return QColor(tok::kText1);
}
} // namespace

namespace com::yamada::studio {
QSize DexRowDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    return {QStyledItemDelegate::sizeHint(option, index).width(), kRowHeight};
}

void DexRowDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                           const QModelIndex &index) const
{
    painter->save();
    const QRectF cell = option.rect;
    const bool selected = option.state & QStyle::State_Selected;

    // 줄 바탕: 선택 > 짝수 줄 > 흰색. 행 번호는 프록시(보이는 순서) 기준이라 정렬 · 검색 뒤에도
    // 줄무늬가 맞다.
    QColor background = QColor(tok::kWhite);
    if (selected)
        background = QColor(tok::kYellowTint);
    else if (index.row() % 2 == 1)
        background = QColor(tok::kPaperAlt);
    painter->fillRect(cell, background);
    painter->fillRect(QRectF(cell.left(), cell.bottom(), cell.width(), 1),
                      QColor(tok::kLineSoft)); // 아래 선

    const QRectF content = cell.adjusted(kCellPadding, 0, -kCellPadding, 0);
    const int column = index.column();
    const QVariant value = index.data(Qt::DisplayRole);

    switch (column) {
    case SpeciesTableModel::CursorColumn:
        if (selected) {
            painter->setRenderHint(QPainter::Antialiasing, true);
            const QPointF c = cell.center();
            QPainterPath triangle;
            triangle.moveTo(c + QPointF(-kCursor.width() / 2, -kCursor.height() / 2));
            triangle.lineTo(c + QPointF(kCursor.width() / 2, 0));
            triangle.lineTo(c + QPointF(-kCursor.width() / 2, kCursor.height() / 2));
            triangle.closeSubpath();
            painter->fillPath(triangle, QColor(tok::kInk));
        }
        break;
    case SpeciesTableModel::NumberColumn:
        painter->setFont(theme::font(theme::kFamilyData, 12, QFont::Bold));
        painter->setPen(QColor(tok::kText3));
        painter->drawText(content, Qt::AlignLeft | Qt::AlignVCenter, value.toString());
        break;
    case SpeciesTableModel::NameColumn:
        painter->setFont(theme::font(theme::kFamilyBody, 13, QFont::ExtraBold));
        painter->setPen(QColor(tok::kText1));
        painter->drawText(content, Qt::AlignLeft | Qt::AlignVCenter, value.toString());
        break;
    case SpeciesTableModel::TypesColumn: {
        qreal x = content.left();
        const qreal y = cell.center().y() - typechip::kHeight / 2;
        for (const QString &identifier : index.data(SpeciesTableModel::TypesRole).toStringList()) {
            if (const tok::TypeColor *type = typechip::find(identifier))
                x += typechip::paint(*painter, QPointF(x, y), *type) + typechip::kGap;
        }
        break;
    }
    case SpeciesTableModel::TotalColumn:
        painter->setFont(theme::font(theme::kFamilyData, 13, QFont::Bold));
        painter->setPen(QColor(tok::kRed));
        painter->drawText(content, Qt::AlignRight | Qt::AlignVCenter, value.toString());
        break;
    default: { // 종족값 6칸
        const int stat = value.toInt();
        painter->setFont(
                theme::font(theme::kFamilyData, 12, stat >= 100 ? QFont::Bold : QFont::Normal));
        painter->setPen(statColor(stat));
        painter->drawText(content, Qt::AlignRight | Qt::AlignVCenter, value.toString());
        break;
    }
    }
    painter->restore();
}
} // namespace com::yamada::studio
