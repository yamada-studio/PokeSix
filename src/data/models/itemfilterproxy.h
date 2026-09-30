#pragma once

#include <QSortFilterProxyModel>

#include <functional>

namespace com::yamada::studio {
// 아이템 목록의 검색 · 분류 · 세대 필터와 정렬 (E3). SpeciesFilterProxy와 같은 방식이다.
//
// 분류 필터는 함수로 받는다: 화면의 묶음(회복 · 기술머신 …)이 PokéAPI 분류 55가지를 어떻게
// 모으는지는 ui(itemstyle.json)가 정하므로, 이 클래스(data)는 "분류 · 주머니를 받으면 통과인가"만
// 묻는다.
//   proxy->setCategoryFilter([](const QString &category, const QString &pocket) { … });
class ItemFilterProxy : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    using CategoryFilter = std::function<bool(const QString &category, const QString &pocket)>;

    explicit ItemFilterProxy(QObject *parent = nullptr);

    void setSearchText(const QString &text); // 이름 ko/en/ja · 효과 문구에 들어 있으면 통과
    void setCategoryFilter(CategoryFilter filter); // 비어 있으면(nullptr) 모든 분류 통과
    void setOnlyInGeneration(bool only);           // true면 지금 세대에 있는 아이템만

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    QString m_search;
    CategoryFilter m_categoryFilter;
    bool m_onlyInGeneration = false;
};
} // namespace com::yamada::studio
