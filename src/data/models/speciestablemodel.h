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
        IconColumn, // 작은 아이콘(값은 PokemonIdRole — 그림은 delegate가 SpriteCache에서)
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
        PokemonIdRole,      // int: 기본 모습의 pokemon id(아이콘 파일 이름)
        LegendaryRole,      // bool: 전설 · 환상 (필터)
        FinalEvolutionRole, // bool: 그 세대 기준 최종 진화 (필터)
        TotalRole,          // int: 종족값 합계 (필터 — 어느 칸에 물어도 같다)
    };

    explicit SpeciesTableModel(QObject *parent = nullptr);

    // 이름 · 문구를 보일 언어(AppState::language). 바꾸면 모든 칸이 바뀌었다고 알린다(dataChanged)
    // — 프록시가 이름순 정렬도 새 언어로 다시 한다.
    void setLanguage(Language language);
    Language language() const { return m_language; }

    void setRows(QList<SpeciesRow> rows); // 통째로 바꾼다(세대가 바뀌면 다시 부른다)
    const SpeciesRow &rowAt(int row) const { return m_rows.at(row); }

    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

private:
    Language m_language = Language::Korean;
    QList<SpeciesRow> m_rows;
};
} // namespace com::yamada::studio
