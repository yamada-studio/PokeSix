#include "data/models/itemtablemodel.h"

namespace com::yamada::studio {
ItemTableModel::ItemTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

void ItemTableModel::setRows(QList<ItemRow> rows, int generation)
{
    beginResetModel();
    m_rows = std::move(rows);
    m_generation = generation;
    endResetModel();
    // 머리 칸의 "현재 세대" 강조도 바뀌었다고 알린다(reset은 머리 칸 다시 그리기를 보장하지 않는다)
    emit headerDataChanged(Qt::Horizontal, 0, ColumnCount - 1);
}

int ItemTableModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_rows.size());
}

int ItemTableModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant ItemTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};
    const ItemRow &row = m_rows.at(index.row());
    // 9세대 아이템 일부는 PokéAPI에 한국어 이름이 아직 없다 → 영어로
    const QString name = row.nameKo.isEmpty() ? row.nameEn : row.nameKo;

    switch (role) {
    case Qt::DisplayRole:
        if (index.column() == NameColumn)
            return name;
        if (index.column() == EffectColumn)
            return row.effect;
        if (index.column() == PriceColumn)
            return row.cost;
        return {};
    case SortRole:
        if (index.column() == PriceColumn)
            return row.cost;
        if (index.column() == EffectColumn)
            return row.effect;
        return name;
    case IdentifierRole:
        return row.identifier;
    case CategoryRole:
        return row.category;
    case PocketRole:
        return row.pocket;
    case GenerationsRole:
        return int(row.generations);
    case InGenerationRole:
        return row.existsIn(m_generation);
    case IntroGenerationRole:
        return row.introGeneration();
    case SearchTextRole:
        return QStringLiteral("%1 %2 %3 %4 %5 %6")
                .arg(row.nameKo, row.nameEn, row.nameJa, row.effect, row.machineMoveKo,
                     row.machineMoveEn);
    case MachineMoveRole:
        return row.machineMoveKo.isEmpty() ? row.machineMoveEn : row.machineMoveKo;
    case MachineTypeRole:
        return row.machineType;
    default:
        return {};
    }
}

QVariant ItemTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};
    switch (section) {
    case NameColumn:
        return tr("이름");
    case EffectColumn:
        return tr("효과");
    case PriceColumn:
        return tr("가격");
    default:
        return {};
    }
}
} // namespace com::yamada::studio
