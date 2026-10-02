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
    // Qt 6.9부터는 beginFilterChange()/endFilterChange()가 있지만, 6.8에서는 invalidateFilter()로
    // "거르는 조건이 바뀌었다"를 알린다. 프록시가 모든 줄을 다시 묻는다.
    const QString trimmed = text.trimmed();
    invalidateIfChanged(std::exchange(m_search, trimmed) != trimmed);
}

void SpeciesFilterProxy::setTypes(const QSet<QString> &types)
{
    invalidateIfChanged(std::exchange(m_types, types) != types);
}

void SpeciesFilterProxy::setTotalRange(int minimum, int maximum)
{
    const bool changed = m_minTotal != minimum || m_maxTotal != maximum;
    m_minTotal = minimum;
    m_maxTotal = maximum;
    invalidateIfChanged(changed);
}

void SpeciesFilterProxy::setExcludeLegendary(bool exclude)
{
    invalidateIfChanged(std::exchange(m_excludeLegendary, exclude) != exclude);
}

void SpeciesFilterProxy::setFinalEvolutionOnly(bool finalOnly)
{
    invalidateIfChanged(std::exchange(m_finalOnly, finalOnly) != finalOnly);
}

void SpeciesFilterProxy::clearFilters()
{
    const bool changed = !m_types.isEmpty() || m_minTotal != 0 || m_maxTotal != kNoMaximum
                         || m_excludeLegendary || m_finalOnly;
    m_types.clear();
    m_minTotal = 0;
    m_maxTotal = kNoMaximum;
    m_excludeLegendary = false;
    m_finalOnly = false;
    invalidateIfChanged(changed);
}

void SpeciesFilterProxy::invalidateIfChanged(bool changed)
{
    if (changed)
        QSortFilterProxyModel::invalidateFilter();
}

bool SpeciesFilterProxy::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    const QModelIndex index
            = sourceModel()->index(sourceRow, SpeciesTableModel::NameColumn, sourceParent);
    if (!m_search.isEmpty()
        && !index.data(SpeciesTableModel::SearchTextRole)
                    .toString()
                    .contains(m_search, Qt::CaseInsensitive))
        return false;
    if (!m_types.isEmpty()) {
        const QStringList types = index.data(SpeciesTableModel::TypesRole).toStringList();
        const bool any = std::any_of(types.cbegin(), types.cend(), [this](const QString &type) {
            return m_types.contains(type);
        });
        if (!any)
            return false;
    }
    const int total = index.data(SpeciesTableModel::TotalRole).toInt();
    if (total < m_minTotal || total > m_maxTotal)
        return false;
    if (m_excludeLegendary && index.data(SpeciesTableModel::LegendaryRole).toBool())
        return false;
    if (m_finalOnly && !index.data(SpeciesTableModel::FinalEvolutionRole).toBool())
        return false;
    return true;
}
} // namespace com::yamada::studio
