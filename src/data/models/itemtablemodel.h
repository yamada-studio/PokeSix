#pragma once

#include "data/repository/repository.h"

#include <QAbstractTableModel>

namespace com::yamada::studio {
// 아이템 대백과 목록의 표 모델 (E3). SpeciesTableModel과 같은 구조다 — 값만 주고, 어떻게 그릴지는
// ui의 delegate가 정한다(QtCore만 쓴다).
//
// 칸: ▶ · 아이콘 · 이름 · 효과 · 가격. 목록에는 지금 세대 아이템만 나오므로(ItemFilterProxy)
// 디자인의 세대 존재 칸(1–9)은 두지 않는다. 기술머신은 담긴 기술(MachineMoveRole ·
// MachineTypeRole)도 준다.
class ItemTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum Column { CursorColumn, IconColumn, NameColumn, EffectColumn, PriceColumn, ColumnCount };

    enum Role {
        SortRole = Qt::UserRole, // 정렬용 값(이름은 한국어 가나다, 가격은 숫자)
        IdentifierRole,          // QString "fire-stone" — 아이콘 파일 이름
        CategoryRole,            // QString PokéAPI 분류 "evolution"
        PocketRole,              // QString 가방 주머니 "misc", "machines" …
        GenerationsRole,         // int 비트: bit (g − 1) = g세대에 있다
        InGenerationRole, // bool 지금 보는 세대에 있다(프록시가 이걸로 거른다)
        IntroGenerationRole, // int 처음 나온 세대
        SearchTextRole, // QString 이름 ko/en/ja + 효과 문구 + 기술머신의 기술 이름
        MachineMoveRole, // QString 기술머신에 담긴 기술(한국어, 없으면 영어). 아니면 빈 문자열
        MachineTypeRole, // QString 그 기술의 그 세대 타입 identifier("fighting")
    };

    explicit ItemTableModel(QObject *parent = nullptr);

    // 이름 · 문구를 보일 언어(AppState::language). 바꾸면 모든 칸이 바뀌었다고 알린다(dataChanged)
    // — 프록시가 이름순 정렬도 새 언어로 다시 한다.
    void setLanguage(Language language);
    Language language() const { return m_language; }

    // 통째로 바꾼다. generation = 지금 보는 세대(InGenerationRole의 기준).
    void setRows(QList<ItemRow> rows, int generation);
    int generation() const { return m_generation; }
    const ItemRow &rowAt(int row) const { return m_rows.at(row); }

    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

private:
    Language m_language = Language::Korean;
    QList<ItemRow> m_rows;
    int m_generation = 1;
};
} // namespace com::yamada::studio
