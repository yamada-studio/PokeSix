#pragma once

#include "data/repository/repository.h"

#include <QAbstractTableModel>

namespace com::yamada::studio {
// 아이템 대백과 목록의 표 모델 (E3). SpeciesTableModel과 같은 구조다 — 값만 주고, 어떻게 그릴지는
// ui의 delegate가 정한다(QtCore만 쓴다).
//
// 칸: ▶ · 아이콘 · 이름 · 효과 · 세대(1–9 존재 칸을 한 칸에). 세대 칸의 값은
// GenerationsRole(비트)로 주고, delegate가 작은 네모 9개로 그린다(디자인 SCR-03: 존재 = 초록, 없음
// = 점선, 현재 세대 = 노랑 테).
class ItemTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum Column {
        CursorColumn,
        IconColumn,
        NameColumn,
        EffectColumn,
        GenerationsColumn,
        ColumnCount
    };

    enum Role {
        SortRole = Qt::UserRole, // 정렬용 값(이름은 한국어 가나다, 세대 칸은 처음 나온 세대)
        IdentifierRole,      // QString "fire-stone" — 아이콘 파일 이름
        CategoryRole,        // QString PokéAPI 분류 "evolution"
        PocketRole,          // QString 가방 주머니 "misc"
        GenerationsRole,     // int 비트: bit (g − 1) = g세대에 있다
        InGenerationRole,    // bool 지금 보는 세대에 있다(없으면 화면이 흐리게)
        IntroGenerationRole, // int 처음 나온 세대
        SearchTextRole,      // QString 이름 ko/en/ja + 효과 문구
    };

    explicit ItemTableModel(QObject *parent = nullptr);

    // 통째로 바꾼다. generation = 지금 보는 세대(InGenerationRole · 헤더 강조의 기준).
    void setRows(QList<ItemRow> rows, int generation);
    int generation() const { return m_generation; }
    const ItemRow &rowAt(int row) const { return m_rows.at(row); }

    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

private:
    QList<ItemRow> m_rows;
    int m_generation = 1;
};
} // namespace com::yamada::studio
