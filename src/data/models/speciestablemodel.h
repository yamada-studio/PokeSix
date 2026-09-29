#pragma once

#include "data/repository/repository.h"

#include <QAbstractTableModel>

namespace com::yamada::studio {
// 도감 목록의 표 모델 (Qt 모델/뷰의 "모델", 로드맵 D2).
//
// 모델/뷰: 데이터(모델)와 그리기(뷰)를 나눈다. 뷰(QTableView)는 필요한 칸만 모델에 묻는다:
//   "몇 줄? 몇 칸? 3행 2열의 DisplayRole 값은?" → 모델은 값만 돌려주고, 어떻게 그릴지는 뷰 ·
//   delegate가 정한다.
// 그래서 이 파일은 QtCore만 쓴다(색 · 글꼴 없음) — 나중에 QML 화면에서도 그대로 쓸 수
// 있다(architecture §1). ROS 2로 치면 모델은 토픽의 메시지이고, 뷰는 그 메시지를 그리는 rviz
// 플러그인이다.
class SpeciesTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum Column {
        CursorColumn, // ▶ (선택된 줄 표시 — 값 없음)
        NumberColumn,
        NameColumn,
        TypesColumn,
        HpColumn,
        AttackColumn,
        DefenseColumn,
        SpAttackColumn,
        SpDefenseColumn,
        SpeedColumn,
        TotalColumn,
        ColumnCount
    };

    enum Role {
        SortRole = Qt::UserRole, // 정렬용 값(숫자는 숫자로 — 문자열로 정렬하면 "100" < "20"이 된다)
        TypesRole,      // QStringList: 타입 identifier(슬롯 순)
        SearchTextRole, // 검색에 쓰는 문자열(번호 · 한국어 · 영어 · 일본어 이름)
    };

    explicit SpeciesTableModel(QObject *parent = nullptr);

    void setRows(QList<SpeciesRow> rows); // 통째로 바꾼다(세대가 바뀌면 다시 부른다)
    const SpeciesRow &rowAt(int row) const { return m_rows.at(row); }

    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

private:
    QList<SpeciesRow> m_rows;
};
} // namespace com::yamada::studio
