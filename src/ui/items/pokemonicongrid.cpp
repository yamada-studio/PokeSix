#include "ui/items/pokemonicongrid.h"

#include "data/sprites/spritecache.h"
#include "ui/dex/dexrowdelegate.h"

#include <QHelpEvent>
#include <QPainter>
#include <QToolTip>

#include <algorithm>

namespace {
constexpr int kCellWidth = 40;
constexpr int kCellHeight = 34;
constexpr qreal kDimmed = 0.35;
} // namespace

namespace com::yamada::studio {
PokemonIconGrid::PokemonIconGrid(SpriteCache *icons, QWidget *parent)
    : QWidget(parent)
    , m_icons(icons)
{
    QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    policy.setHeightForWidth(true);
    QWidget::setSizePolicy(policy);
    connect(m_icons, &SpriteCache::ready, this, qOverload<>(&QWidget::update));
}

void PokemonIconGrid::setEntries(const QList<Entry> &entries)
{
    m_entries = entries;
    QWidget::updateGeometry();
    QWidget::update();
}

int PokemonIconGrid::columns(int width) const
{
    return std::max(1, width / kCellWidth);
}

int PokemonIconGrid::heightForWidth(int width) const
{
    const int count = int(m_entries.size());
    const int perRow = columns(width);
    return ((count + perRow - 1) / perRow) * kCellHeight;
}

QSize PokemonIconGrid::sizeHint() const
{
    const int w = std::max(width(), kCellWidth * 6);
    return {w, heightForWidth(w)};
}

int PokemonIconGrid::entryAt(const QPoint &pos) const
{
    const int perRow = columns(width());
    const int column = pos.x() / kCellWidth;
    const int index = (pos.y() / kCellHeight) * perRow + column;
    return column < perRow && index >= 0 && index < m_entries.size() ? index : -1;
}

void PokemonIconGrid::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    const int perRow = columns(width());
    // 남는 폭은 칸 사이에 고르게(왼쪽에 몰리지 않게)
    const qreal step = qreal(width()) / perRow;
    for (qsizetype i = 0; i < m_entries.size(); ++i) {
        const Entry &entry = m_entries.at(i);
        const QRectF cell(int(i % perRow) * step, int(i / perRow) * kCellHeight, step, kCellHeight);
        painter.setOpacity(entry.note.isEmpty() ? 1.0 : kDimmed);
        DexRowDelegate::paintPokemonIcon(&painter, cell, entry.pokemonId, m_icons);
    }
}

bool PokemonIconGrid::event(QEvent *event)
{
    if (event->type() == QEvent::ToolTip) {
        const auto *help = static_cast<QHelpEvent *>(event);
        const int index = entryAt(help->pos());
        if (index < 0) {
            QToolTip::hideText();
        } else {
            const Entry &entry = m_entries.at(index);
            QToolTip::showText(help->globalPos(),
                               entry.note.isEmpty()
                                       ? entry.name
                                       : QStringLiteral("%1 (%2)").arg(entry.name, entry.note),
                               this);
        }
        return true;
    }
    return QWidget::event(event);
}
} // namespace com::yamada::studio
