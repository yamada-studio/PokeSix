#include "data/models/speciesfilterproxy.h"

#include "data/models/speciestablemodel.h"

namespace com::yamada::studio {
SpeciesFilterProxy::SpeciesFilterProxy(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    QSortFilterProxyModel::setSortRole(SpeciesTableModel::SortRole);
    QSortFilterProxyModel::setSortLocaleAware(true); // 한글 이름을 가나다순으로
}

void SpeciesFilterProxy::setSearchText(const QString &text)
{
    const QString trimmed = text.trimmed();
    if (trimmed == m_search)
        return;
    // Qt 6.9부터는 beginFilterChange()/endFilterChange()가 있지만, 6.8에서는 invalidateFilter()로
    // "거르는 조건이 바뀌었다"를 알린다. 프록시가 모든 줄을 다시 묻는다.
    m_search = trimmed;
    QSortFilterProxyModel::invalidateFilter();
}

bool SpeciesFilterProxy::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    if (m_search.isEmpty())
        return true;
    const QModelIndex index
            = sourceModel()->index(sourceRow, SpeciesTableModel::NameColumn, sourceParent);
    const QString haystack = index.data(SpeciesTableModel::SearchTextRole).toString();
    return haystack.contains(m_search, Qt::CaseInsensitive);
}
} // namespace com::yamada::studio
