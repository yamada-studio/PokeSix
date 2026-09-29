#include "data/models/speciestablemodel.h"

namespace com::yamada::studio {
SpeciesTableModel::SpeciesTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

void SpeciesTableModel::setRows(QList<SpeciesRow> rows)
{
    // beginResetModel / endResetModel: "모델이 통째로 바뀐다"를 뷰에 알린다. 그 사이에 데이터를
    // 바꾼다. (한 줄만 바뀌면 dataChanged, 줄이 늘면 beginInsertRows … 처럼 더 좁은 알림을 쓴다)
    beginResetModel();
    m_rows = std::move(rows);
    endResetModel();
}

int SpeciesTableModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_rows.size()); // 표는 자식이 없다
}

int SpeciesTableModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant SpeciesTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};
    const SpeciesRow &row = m_rows.at(index.row());
    const int column = index.column();
    const bool isStat = column >= HpColumn && column <= SpeedColumn;

    switch (role) {
    case Qt::DisplayRole:
        if (column == NumberColumn)
            return row.speciesId;
        if (column == NameColumn)
            return row.nameKo;
        if (isStat)
            return row.stats[column - HpColumn];
        if (column == TotalColumn)
            return row.total;
        return {};
    case SortRole:
        if (column == NameColumn)
            return row.nameKo;
        if (column == TypesColumn)
            return row.types.value(0);
        if (column == CursorColumn)
            return row.speciesId;
        return data(index, Qt::DisplayRole); // 숫자 칸은 숫자 그대로
    case TypesRole:
        return row.types;
    case SearchTextRole:
        return QStringLiteral("%1 %2 %3 %4")
                .arg(row.speciesId)
                .arg(row.nameKo, row.nameEn, row.nameJa);
    case Qt::TextAlignmentRole:
        if (isStat || column == TotalColumn)
            return QVariant::fromValue(Qt::AlignRight
                                       | Qt::AlignVCenter); // 숫자 열은 오른쪽 정렬(01 §3)
        return QVariant::fromValue(Qt::AlignLeft | Qt::AlignVCenter);
    default:
        return {};
    }
}

QVariant SpeciesTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal)
        return {};
    if (role == Qt::TextAlignmentRole) {
        const bool numeric = section >= HpColumn;
        return QVariant::fromValue((numeric ? Qt::AlignRight : Qt::AlignLeft) | Qt::AlignVCenter);
    }
    if (role != Qt::DisplayRole)
        return {};
    switch (section) {
    case NumberColumn:
        return tr("No.");
    case NameColumn:
        return tr("이름");
    case TypesColumn:
        return tr("타입");
    case HpColumn:
        return tr("HP");
    case AttackColumn:
        return tr("공격");
    case DefenseColumn:
        return tr("방어");
    case SpAttackColumn:
        return tr("특공");
    case SpDefenseColumn:
        return tr("특방");
    case SpeedColumn:
        return tr("스피드");
    case TotalColumn:
        return tr("합계");
    default:
        return {};
    }
}
} // namespace com::yamada::studio
