#include "ui/items/itemrowdelegate.h"

#include "data/models/itemtablemodel.h"
#include "data/sprites/spritecache.h"
#include "ui/theme/theme.h"
#include "ui/theme/tokens.h"
#include "ui/widgets/rowhover.h"
#include "ui/widgets/typechip.h"

#include <QFontMetricsF>
#include <QLocale>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPixmapCache>
#include <QRegularExpression>

namespace {
using namespace com::yamada::studio;

constexpr qreal kCellPadding = 4; // 칸 좌우 여백(도감과 같다)
constexpr QSizeF kCursor {9, 12}; // ▶
constexpr qreal kPlaceholder = 26; // 아이콘이 없을 때의 원 지름(디자인의 아이콘 자리)
constexpr qreal kChipGap = 6;             // 타입 칩 ↔ 기술 이름 ↔ 설명
constexpr int kLastBwStyleGeneration = 5; // 여기까지는 5세대(BW) 그림
} // namespace

namespace com::yamada::studio {
QString ItemRowDelegate::iconKey(const QString &identifier, const QString &machineType,
                                 int generation)
{
    QString name = identifier;
    // 기술머신인데 타입을 모르면(2–4세대 저주의 ??? 타입, 데이터에만 있는 비전머신08) 노말 CD로
    static const QRegularExpression machine(QStringLiteral("^(tm|hm|tr)\\d"));
    const QString type = machineType.isEmpty() && machine.match(identifier).hasMatch()
                                 ? QStringLiteral("normal")
                                 : machineType;
    if (!type.isEmpty()) {
        // 기술머신은 아이템 하나("tm01")가 세대마다 다른 기술을 담는다 → 그림도 담긴 기술의 타입
        // CD. 비전머신은 hm-, 나머지(기술머신 · 기술레코드)는 tm- 그림.
        const bool hidden = identifier.startsWith(QLatin1String("hm"));
        name = (hidden ? QStringLiteral("hm-") : QStringLiteral("tm-")) + type;
    }
    return generation <= kLastBwStyleGeneration ? QStringLiteral("gen5/") + name : name;
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

    // 줄 바탕(선택 > 홀수 줄 > 흰색) + 아래 선
    QColor background(tok::kWhite);
    if (selected)
        background = QColor(tok::kYellowTint);
    else if (m_hover && m_hover->row() == index.row())
        background = QColor(tok::kYellowRowSel); // 호버: 선택보다 옅은 노랑
    else if (index.row() % 2 == 1)
        background = QColor(tok::kPaperAlt);
    painter->fillRect(cell, background);
    painter->fillRect(QRectF(cell.left(), cell.bottom(), cell.width(), 1), QColor(tok::kLineSoft));

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
            painter->fillPath(triangle, QColor(tok::kInk));
        }
        break;
    case ItemTableModel::IconColumn:
        paintIcon(painter, cell, index);
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
        paintEffect(painter, content, index);
        break;
    case ItemTableModel::PriceColumn: {
        const int cost = index.data().toInt();
        painter->setFont(theme::font(theme::kFamilyData, 13));
        painter->setPen(QColor(cost > 0 ? tok::kText1 : tok::kTextDisabled));
        // QLocale로 천 단위 쉼표("1,200"). 가격은 최신 게임 기준 하나뿐이다(PokéAPI)
        painter->drawText(content, Qt::AlignRight | Qt::AlignVCenter,
                          cost > 0 ? tr("%1원").arg(QLocale(QLocale::Korean).toString(cost))
                                   : QStringLiteral("—"));
        break;
    }
    default:
        break;
    }
    painter->restore();
}

void ItemRowDelegate::paintIcon(QPainter *painter, const QRectF &cell,
                                const QModelIndex &index) const
{
    const QString key
            = iconKey(index.data(ItemTableModel::IdentifierRole).toString(),
                      index.data(ItemTableModel::MachineTypeRole).toString(), m_generation);
    const QString file = m_sprites->path(key);
    QPixmap pixmap;
    if (!file.isEmpty()) {
        // 파일 → QPixmap 디코딩은 한 번만(QPixmapCache: 앱 전역 LRU). 도감 아이콘과 key가 겹치지
        // 않게
        const QString cacheKey = QStringLiteral("pokesix.item.") + key;
        if (!QPixmapCache::find(cacheKey, &pixmap) && pixmap.load(file))
            QPixmapCache::insert(cacheKey, pixmap);
    } else {
        m_sprites->request(key); // 받으면 ready → ItemsPage가 표를 다시 그린다
    }
    if (pixmap.isNull()) {
        painter->setPen(QPen(QColor(tok::kLineStrong), 1.5));
        painter->setBrush(QColor(tok::kPaperAlt));
        painter->drawEllipse(cell.center(), kPlaceholder / 2, kPlaceholder / 2);
        return;
    }
    // 도트를 그대로(정수 배가 아니게 늘리면 흐려진다). 칸 가운데에.
    const QPointF topLeft = cell.center() - QPointF(pixmap.width() / 2.0, pixmap.height() / 2.0);
    painter->drawPixmap(topLeft.toPoint(), pixmap);
}

void ItemRowDelegate::paintEffect(QPainter *painter, const QRectF &content,
                                  const QModelIndex &index) const
{
    qreal x = content.left();
    // 기술머신: [타입 칩] 기술 이름 — 이 세대에 담긴 기술(기술머신01: 4세대 힘껏펀치, 5세대
    // 손톱갈기)
    const QString move = index.data(ItemTableModel::MachineMoveRole).toString();
    if (!move.isEmpty()) {
        if (const tok::TypeColor *type
            = typechip::find(index.data(ItemTableModel::MachineTypeRole).toString())) {
            const QPointF topLeft(x, content.center().y() - typechip::kHeight / 2);
            x += typechip::paint(*painter, topLeft, *type, m_language) + kChipGap;
        }
        const QFont moveFont = theme::font(theme::kFamilyBody, 13, QFont::ExtraBold);
        painter->setFont(moveFont);
        painter->setPen(QColor(tok::kText1));
        const QRectF moveRect(x, content.top(), content.right() - x, content.height());
        const QString moveText
                = QFontMetricsF(moveFont).elidedText(move, Qt::ElideRight, moveRect.width());
        painter->drawText(moveRect, Qt::AlignLeft | Qt::AlignVCenter, moveText);
        x += QFontMetricsF(moveFont).horizontalAdvance(moveText) + kChipGap;
    }
    if (x >= content.right())
        return;
    const QFont font = theme::font(theme::kFamilyBody, 13);
    painter->setFont(font);
    painter->setPen(QColor(tok::kText2));
    const QRectF rest(x, content.top(), content.right() - x, content.height());
    painter->drawText(
            rest, Qt::AlignLeft | Qt::AlignVCenter,
            QFontMetricsF(font).elidedText(index.data().toString(), Qt::ElideRight, rest.width()));
}
} // namespace com::yamada::studio
