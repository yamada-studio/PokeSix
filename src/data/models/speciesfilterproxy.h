#pragma once

#include <QSet>
#include <QSortFilterProxyModel>
#include <QString>

namespace com::yamada::studio {
// 도감 목록의 검색 · 필터 (D2 검색, E1 필터 창).
// 조건은 전부 AND: 검색 글자, 타입(고른 것 중 하나라도 가지면), 종족값 합계 범위,
// 전설 · 환상 제외, 최종 진화만. 값은 SpeciesTableModel의 역할(role)로 묻는다.
class SpeciesFilterProxy : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit SpeciesFilterProxy(QObject *parent = nullptr);

    void setSearchText(const QString &text);
    void setTypes(const QSet<QString> &types); // 비면 모든 타입
    void setTotalRange(int minimum, int maximum);
    void setExcludeLegendary(bool exclude);
    void setFinalEvolutionOnly(bool finalOnly);
    void clearFilters(); // 검색 글자는 그대로 두고 필터만 되돌린다

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    void invalidateIfChanged(bool changed);

    QString m_search;
    QSet<QString> m_types;
    int m_minTotal = 0;
    int m_maxTotal = kNoMaximum;
    bool m_excludeLegendary = false;
    bool m_finalOnly = false;

    static constexpr int kNoMaximum = 9999;
};
} // namespace com::yamada::studio
