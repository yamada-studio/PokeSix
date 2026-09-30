#include "data/models/itemfilterproxy.h"

#include "data/models/itemtablemodel.h"

namespace com::yamada::studio {
ItemFilterProxy::ItemFilterProxy(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    QSortFilterProxyModel::setSortRole(ItemTableModel::SortRole);
    QSortFilterProxyModel::setSortLocaleAware(true); // 한글 이름을 가나다순으로
}

void ItemFilterProxy::setSearchText(const QString &text)
{
    const QString trimmed = text.trimmed();
    if (trimmed == m_search)
        return;
    m_search = trimmed;
    // 조건이 바뀌었다 → 모든 줄의 filterAcceptsRow를 다시 묻는다(Qt 6.8. 6.9부터는
    // begin/endFilterChange)
    QSortFilterProxyModel::invalidateFilter();
}

void ItemFilterProxy::setCategoryFilter(CategoryFilter filter)
{
    m_categoryFilter = std::move(filter);
    QSortFilterProxyModel::invalidateFilter();
}

void ItemFilterProxy::setOnlyInGeneration(bool only)
{
    if (only == m_onlyInGeneration)
        return;
    m_onlyInGeneration = only;
    QSortFilterProxyModel::invalidateFilter();
}

bool ItemFilterProxy::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    const QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
    if (m_onlyInGeneration && !index.data(ItemTableModel::InGenerationRole).toBool())
        return false;
    if (m_categoryFilter
        && !m_categoryFilter(index.data(ItemTableModel::CategoryRole).toString(),
                             index.data(ItemTableModel::PocketRole).toString()))
        return false;
    if (!m_search.isEmpty()
        && !index.data(ItemTableModel::SearchTextRole)
                    .toString()
                    .contains(m_search, Qt::CaseInsensitive))
        return false;
    return true;
}

} // namespace com::yamada::studio
