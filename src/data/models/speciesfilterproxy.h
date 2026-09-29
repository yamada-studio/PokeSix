#pragma once

#include <QSortFilterProxyModel>

namespace com::yamada::studio {
// 도감 목록의 검색 · 정렬 (로드맵 D2). 원본 모델(SpeciesTableModel)은 그대로 두고,
// 그 위에 "보이는 줄 · 순서"만 바꿔 보여 주는 층이다. 뷰는 이 프록시를 모델로 쓴다.
//   정렬: 열 머리를 누르면 QTableView가 sort()를 부른다 → SortRole 값으로 비교
//   검색: setSearchText("한카") → 이름(한 · 영 · 일)이나 번호에 들어 있는 줄만 남긴다
class SpeciesFilterProxy : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit SpeciesFilterProxy(QObject *parent = nullptr);

    void setSearchText(const QString &text);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    QString m_search;
};
} // namespace com::yamada::studio
