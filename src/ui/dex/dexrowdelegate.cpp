#include "ui/dex/dexrowdelegate.h"

#include "data/models/speciestablemodel.h"
#include "data/sprites/spritecache.h"
#include "ui/theme/theme.h"
#include "ui/theme/tokens.h"
#include "ui/widgets/typechip.h"

#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPixmapCache>
#include <QtMath>

#include <algorithm>

namespace {
using namespace com::yamada::studio;

constexpr QSizeF kIcon {34, 28};  // 8세대 박스 아이콘 68×56의 절반
constexpr qreal kCellPadding = 4; // 열 사이 gap 8의 절반씩
constexpr QSizeF kCursor {9, 12}; // ▶ border-left 9 · 위아래 6

// 알파가 0이 아닌 픽셀을 모두 담는 가장 작은 사각형. 다 투명하면 전체.
QRect opaqueBounds(const QImage &source)
{
    const QImage image = source.convertToFormat(QImage::Format_ARGB32);
    int left = image.width(), top = image.height(), right = -1, bottom = -1;
    for (int y = 0; y < image.height(); ++y) {
        const QRgb *line = reinterpret_cast<const QRgb *>(image.constScanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            if (qAlpha(line[x]) == 0)
                continue;
            left = std::min(left, x);
            right = std::max(right, x);
            top = std::min(top, y);
            bottom = std::max(bottom, y);
        }
    }
    return right < 0 ? image.rect() : QRect(QPoint(left, top), QPoint(right, bottom));
}

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
DexRowDelegate::DexRowDelegate(SpriteCache *sprites, QObject *parent)
    : QStyledItemDelegate(parent)
    , m_sprites(sprites)
{
}

Qt::Alignment DexRowDelegate::alignment(int column)
{
    const bool numeric = column >= SpeciesTableModel::HpColumn;
    return (numeric ? Qt::AlignRight : Qt::AlignLeft) | Qt::AlignVCenter;
}

QRectF DexRowDelegate::contentRect(const QRectF &cell, int column)
{
    const qreal right = kCellPadding + (column == SpeciesTableModel::TotalColumn ? kEndGap : 0);
    return cell.adjusted(kCellPadding, 0, -right, 0);
}

int DexRowDelegate::typesColumnWidth()
{
    qreal widest = 0;
    qreal second = 0;
    for (const tok::TypeColor &type : tok::kTypes) {
        const qreal width = typechip::width(type);
        if (width > widest) {
            second = widest;
            widest = width;
        } else if (width > second) {
            second = width;
        }
    }
    return qCeil(2 * kCellPadding + widest + typechip::kGap + second);
}

void DexRowDelegate::paintIcon(QPainter *painter, const QRectF &cell, int pokemonId) const
{
    const QString id = QString::number(pokemonId);
    const QString file = m_sprites->path(id);
    if (file.isEmpty()) {
        m_sprites->request(id); // 받으면 ready → DexPage가 표를 다시 그린다
        return;
    }
    // 파일 → QPixmap 디코딩은 줄마다 매번 하면 스크롤이 버벅인다. QPixmapCache(앱 전역 LRU)에
    // 한 번 읽은 그림을 넣어 두고 꺼내 쓴다.
    const QString key = QStringLiteral("pokesix.sprite.%1").arg(pokemonId);
    QPixmap pixmap;
    if (!QPixmapCache::find(key, &pixmap)) {
        QImage image;
        if (!image.load(file))
            return;
        // 원본 PNG는 투명 여백이 넓다(68×56 안에 몸은 30px 남짓). 불투명한 부분만 잘라 칸을 채운다.
        pixmap = QPixmap::fromImage(image.copy(opaqueBounds(image)));
        QPixmapCache::insert(key, pixmap);
    }
    // 비율을 지키며 아이콘 칸(34×28)에 맞춘다. 96×96 대체 스프라이트도 같은 칸에 들어간다.
    const QSizeF size = QSizeF(pixmap.size()).scaled(kIcon, Qt::KeepAspectRatio);
    const QRectF target(cell.center() - QPointF(size.width() / 2, size.height() / 2), size);
    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter->drawPixmap(target, pixmap, pixmap.rect());
}

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

    const int column = index.column();
    const QRectF content = contentRect(cell, column);
    const Qt::Alignment align = alignment(column);
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
        painter->setFont(theme::font(theme::kFamilyData, 13, QFont::Bold));
        painter->setPen(QColor(tok::kText3));
        painter->drawText(content, align, value.toString());
        break;
    case SpeciesTableModel::IconColumn:
        paintIcon(painter, cell, index.data(SpeciesTableModel::PokemonIdRole).toInt());
        break;
    case SpeciesTableModel::NameColumn:
        painter->setFont(theme::font(theme::kFamilyBody, 14, QFont::ExtraBold));
        painter->setPen(QColor(tok::kText1));
        painter->drawText(content, align, value.toString());
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
        painter->setFont(theme::font(theme::kFamilyData, 14, QFont::Bold));
        painter->setPen(QColor(tok::kRed));
        painter->drawText(content, align, value.toString());
        break;
    default: { // 종족값 6칸
        const int stat = value.toInt();
        painter->setFont(
                theme::font(theme::kFamilyData, 13, stat >= 100 ? QFont::Bold : QFont::Normal));
        painter->setPen(statColor(stat));
        painter->drawText(content, align, value.toString());
        break;
    }
    }
    painter->restore();
}
} // namespace com::yamada::studio
