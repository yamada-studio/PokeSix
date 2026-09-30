#include "ui/items/itemrowdelegate.h"

#include "data/models/itemtablemodel.h"
#include "data/sprites/spritecache.h"
#include "ui/theme/theme.h"
#include "ui/theme/tokens.h"

#include <QFontMetricsF>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPixmapCache>
#include <QtMath>

namespace {
using namespace com::yamada::studio;

constexpr qreal kCellPadding = 4; // 칸 좌우 여백(도감과 같다)
constexpr QSizeF kCursor {9, 12}; // ▶
constexpr qreal kPlaceholder = 26; // 아이콘이 없을 때의 원 지름(디자인의 아이콘 자리)
constexpr QSizeF kGenCell {18, 16};
constexpr qreal kGenGap = 2;
constexpr qreal kGenRadius = 3;
constexpr qreal kAbsentOpacity = 0.55;
constexpr qreal kTagGap = 6; // 효과 글자 ↔ "n세대 없음" 태그
constexpr qreal kTagPaddingX = 6;
constexpr qreal kTagHeight = 16;
} // namespace

namespace com::yamada::studio {
int ItemRowDelegate::generationsColumnWidth()
{
    return qCeil(kGenerationCount * kGenCell.width() + (kGenerationCount - 1) * kGenGap
                 + 2 * kCellPadding);
}

QRectF ItemRowDelegate::generationRect(const QRectF &cell, int generation)
{
    const qreal x = cell.left() + kCellPadding + (generation - 1) * (kGenCell.width() + kGenGap);
    return {x, cell.center().y() - kGenCell.height() / 2, kGenCell.width(), kGenCell.height()};
}

ItemRowDelegate::ItemRowDelegate(SpriteCache *sprites, QObject *parent)
    : QStyledItemDelegate(parent)
    , m_sprites(sprites)
{
}

QSize ItemRowDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    return {QStyledItemDelegate::sizeHint(option, index).width(), kRowHeight};
}

void ItemRowDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                            const QModelIndex &index) const
{
    painter->save();
    const QRectF cell = option.rect;
    const bool selected = option.state & QStyle::State_Selected;

    // 1) 줄 바탕(선택 > 홀수 줄 > 흰색) + 아래 선. 흐리게 하기 전에 칠한다 — 바탕까지 흐리면
    // 줄무늬가
    //    어긋나 보인다.
    QColor background(tok::kWhite);
    if (selected)
        background = QColor(tok::kYellowTint);
    else if (index.row() % 2 == 1)
        background = QColor(tok::kPaperAlt);
    painter->fillRect(cell, background);
    painter->fillRect(QRectF(cell.left(), cell.bottom(), cell.width(), 1), QColor(tok::kLineSoft));

    // 2) 지금 세대에 없는 아이템: 내용만 55%로
    const bool absent = !index.data(ItemTableModel::InGenerationRole).toBool();
    if (absent)
        painter->setOpacity(kAbsentOpacity);

    const QRectF content = cell.adjusted(kCellPadding, 0, -kCellPadding, 0);
    painter->setRenderHint(QPainter::Antialiasing, true);
    switch (index.column()) {
    case ItemTableModel::CursorColumn:
        if (selected) {
            const QPointF c = cell.center();
            QPainterPath triangle;
            triangle.moveTo(c + QPointF(-kCursor.width() / 2, -kCursor.height() / 2));
            triangle.lineTo(c + QPointF(kCursor.width() / 2, 0));
            triangle.lineTo(c + QPointF(-kCursor.width() / 2, kCursor.height() / 2));
            triangle.closeSubpath();
            painter->setOpacity(1); // ▶는 흐리지 않는다(어느 줄을 골랐는지는 늘 또렷하게)
            painter->fillPath(triangle, QColor(tok::kInk));
        }
        break;
    case ItemTableModel::IconColumn:
        paintIcon(painter, cell, index.data(ItemTableModel::IdentifierRole).toString());
        break;
    case ItemTableModel::NameColumn:
        painter->setFont(theme::font(theme::kFamilyBody, 14, QFont::ExtraBold));
        painter->setPen(QColor(tok::kText1));
        painter->drawText(
                content, Qt::AlignLeft | Qt::AlignVCenter,
                QFontMetricsF(painter->font())
                        .elidedText(index.data().toString(), Qt::ElideRight, content.width()));
        break;
    case ItemTableModel::EffectColumn:
        paintEffect(painter, content, index.data().toString(), absent);
        break;
    case ItemTableModel::GenerationsColumn:
        paintGenerations(painter, cell, index.data(ItemTableModel::GenerationsRole).toInt());
        break;
    default:
        break;
    }
    painter->restore();
}

void ItemRowDelegate::paintIcon(QPainter *painter, const QRectF &cell,
                                const QString &identifier) const
{
    const QString file = m_sprites->path(identifier);
    QPixmap pixmap;
    if (!file.isEmpty()) {
        // 파일 → QPixmap 디코딩은 한 번만(QPixmapCache: 앱 전역 LRU). 도감 아이콘과 key가 겹치지
        // 않게
        const QString key = QStringLiteral("pokesix.item.%1").arg(identifier);
        if (!QPixmapCache::find(key, &pixmap) && pixmap.load(file))
            QPixmapCache::insert(key, pixmap);
    } else {
        m_sprites->request(identifier); // 받으면 ready → ItemsPage가 표를 다시 그린다
    }
    if (pixmap.isNull()) {
        // 자리 표시: 원(기술머신은 아이콘 파일 이름이 타입별이라 아직 없다)
        painter->setPen(QPen(QColor(tok::kLineStrong), 1.5));
        painter->setBrush(QColor(tok::kPaperAlt));
        painter->drawEllipse(cell.center(), kPlaceholder / 2, kPlaceholder / 2);
        return;
    }
    // 30×30 도트를 그대로(정수 배가 아니게 늘리면 흐려진다). 칸 가운데에.
    const QPointF topLeft = cell.center() - QPointF(pixmap.width() / 2.0, pixmap.height() / 2.0);
    painter->drawPixmap(topLeft.toPoint(), pixmap);
}

void ItemRowDelegate::paintEffect(QPainter *painter, const QRectF &content, const QString &text,
                                  bool absent) const
{
    // "4세대 없음" 태그 폭을 먼저 잡고, 효과 글자는 남는 폭에서 말줄임한다.
    const QFont tagFont = theme::font(theme::kFamilyBody, 10, QFont::ExtraBold);
    const QString tag = tr("%1세대 없음").arg(m_generation);
    const qreal tagWidth
            = absent ? QFontMetricsF(tagFont).horizontalAdvance(tag) + 2 * kTagPaddingX : 0;

    const QFont font = theme::font(theme::kFamilyBody, 13);
    const QFontMetricsF metrics(font);
    const qreal available = content.width() - (absent ? tagWidth + kTagGap : 0);
    const QString elided = metrics.elidedText(text, Qt::ElideRight, available);
    painter->setFont(font);
    painter->setPen(QColor(tok::kText2));
    painter->drawText(content, Qt::AlignLeft | Qt::AlignVCenter, elided);
    if (!absent)
        return;

    const qreal x
            = content.left() + metrics.horizontalAdvance(elided) + (elided.isEmpty() ? 0 : kTagGap);
    const QRectF box(x, content.center().y() - kTagHeight / 2, tagWidth, kTagHeight);
    QPen dashed(QColor(tok::kTextDisabled), 1.5);
    dashed.setStyle(Qt::DashLine);
    painter->setPen(dashed);
    painter->setBrush(Qt::NoBrush);
    painter->drawRoundedRect(box.adjusted(0.75, 0.75, -0.75, -0.75), 3, 3);
    painter->setFont(tagFont);
    painter->setPen(QColor(tok::kText3));
    painter->drawText(box, Qt::AlignCenter, tag);
}

void ItemRowDelegate::paintGenerations(QPainter *painter, const QRectF &cell, int bits) const
{
    for (int g = 1; g <= kGenerationCount; ++g) {
        const QRectF box = generationRect(cell, g);
        const bool has = (bits >> (g - 1)) & 1;
        const bool current = g == m_generation;
        // 테 두께가 달라도 칸 바깥 크기는 같게: 펜 반 폭만큼 안쪽으로
        const qreal width = current ? 2.0 : 1.5;
        QPen pen(QColor(current ? tok::kYellow : (has ? tok::kInk : tok::kLineStrong)), width);
        if (!has && !current)
            pen.setStyle(Qt::DashLine);
        painter->setPen(pen);
        painter->setBrush(QColor(has ? tok::kGreen : tok::kWhite));
        const qreal half = width / 2;
        painter->drawRoundedRect(box.adjusted(half, half, -half, -half), kGenRadius - half,
                                 kGenRadius - half);
    }
}
} // namespace com::yamada::studio
